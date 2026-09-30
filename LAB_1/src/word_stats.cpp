#include "word_stats.hpp"
#include "text_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

    void writeFreqPartial(std::ostream& out, std::string_view word, std::size_t count)
    {
        std::uint32_t len = static_cast<std::uint32_t>(word.size());
        std::uint64_t c   = static_cast<std::uint64_t>(count);
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(word.data(), len);
        out.write(reinterpret_cast<const char*>(&c), sizeof(c));
    }

    bool readFreqPartial(std::istream& in, std::string& word, std::size_t& count)
    {
        std::uint32_t len = 0;
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) return false;
        word.resize(len);
        if (!in.read(word.data(), len)) return false;
        std::uint64_t c = 0;
        if (!in.read(reinterpret_cast<char*>(&c), sizeof(c))) return false;
        count = static_cast<std::size_t>(c);
        return true;
    }

    std::size_t chunksForBytes(std::size_t textSize, std::size_t targetBytes)
    {
        if (targetBytes == 0) targetBytes = 1;
        std::size_t n = (textSize + targetBytes - 1) / targetBytes;
        if (n < 2) n = 2;
        return n;
    }

    // Не больше 8 потоков — иначе упираемся в диск.
    std::size_t workerCount(std::size_t nChunks)
    {
        std::size_t n = parallel::hardwareThreads();
        if (n > 8)       n = 8;
        if (n > nChunks) n = nChunks;
        if (n == 0)      n = 1;
        return n;
    }

} // namespace

void wordFrequency::process(std::string_view text, unsigned options,
                            std::size_t nThreads)
{
    if (nThreads <= 1)
    {
        utf8::forEachWord(text, [&](std::string_view w) 
        {
            std::string key = (options & ignoreCase) ? utf8::toLower(w)
                                                     : std::string(w);
            ++freq_[std::move(key)];
            ++total_;
        });
        rebuildEntries();
        return;
    }

    const auto chunks = parallel::splitChunks(text, nThreads);
    const std::size_t n = chunks.size();

    std::vector<std::unordered_map<std::string, std::size_t>> maps(n);
    std::vector<std::size_t> totals(n, 0);
    std::vector<std::thread> threads;
    threads.reserve(n);

    for (std::size_t t = 0; t < n; ++t)
    {
        threads.emplace_back([&, t] {
            const auto [b, e] = chunks[t];
            utf8::forEachWord(text.substr(b, e - b), [&](std::string_view w) 
            {
                std::string key = (options & ignoreCase) ? utf8::toLower(w)
                                                         : std::string(w);
                ++maps[t][std::move(key)];
                ++totals[t];
            });
        });
    }
    for (auto& th : threads) th.join();

    // Слияние локальных карт в одну
    for (std::size_t t = 0; t < n; ++t)
    {
        total_ += totals[t];
        for (auto& kv : maps[t])
        {
            freq_[std::move(kv.first)] += kv.second;
        }
    }

    rebuildEntries();
}

void wordFrequency::processChunked(std::string_view text,
                                   const std::string& tmpDir,
                                   const std::string& outPath,
                                   unsigned options,
                                   std::size_t targetChunkBytes)
{
    const std::size_t nChunks = chunksForBytes(text.size(), targetChunkBytes);
    const auto chunks = parallel::splitChunks(text, nChunks);

    std::vector<std::string> partPaths(chunks.size());
    std::vector<std::size_t> totals(chunks.size(), 0);

    const std::size_t nThreads = workerCount(chunks.size());
    std::atomic<std::size_t> nextChunk{0};

    std::exception_ptr workerError;
    std::mutex         errorMutex;

    auto workerFn = [&] {
        try 
        {
            while (true) 
            {
                const std::size_t ci =
                    nextChunk.fetch_add(1, std::memory_order_relaxed);

                if (ci >= chunks.size()) break;

                const auto [b, e] = chunks[ci];
                std::string_view slice = text.substr(b, e - b);

                std::unordered_map<std::string, std::size_t> local;
                local.reserve(slice.size() / 64);

                std::size_t count = 0;
                utf8::forEachWord(slice, [&](std::string_view w) 
                {
                    std::string key = (options & ignoreCase)
                        ? utf8::toLower(w) : std::string(w);
                    ++local[std::move(key)];
                    ++count;
                });
                totals[ci] = count;

                struct Ref { const std::string* w; std::size_t c; };
                std::vector<Ref> sorted;
                sorted.reserve(local.size());
                for (const auto& kv : local)
                {
                    sorted.push_back({&kv.first, kv.second});
                }
                    
                std::sort(sorted.begin(), sorted.end(),
                          [](const Ref& a, const Ref& b) { return *a.w < *b.w; });

                std::string p = tmpDir + "/wordstats_freq_part_"
                              + std::to_string(ci) + ".bin";

                std::ofstream out(p, std::ios::binary | std::ios::trunc);

                if (!out)
                {
                    throw std::runtime_error("Не создать temp: " + p);
                }

                for (const auto& r : sorted)
                {
                    writeFreqPartial(out, *r.w, r.c);
                }
                    
                out.close();
                if (!out)
                {
                    throw std::runtime_error("Ошибка записи temp: " + p);
                }
                partPaths[ci] = std::move(p);
            }
        } catch (...) {
            std::lock_guard<std::mutex> lk(errorMutex);
            if (!workerError)
            {
                workerError = std::current_exception();
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(nThreads);

    for (std::size_t t = 0; t < nThreads; ++t)
    {
        workers.emplace_back(workerFn);
    }

    for (auto& w : workers)
    {
        w.join();
    }

    if (workerError) 
    {
        std::rethrow_exception(workerError);
    }
    std::size_t totalWords = 0;

    for (auto c : totals)
    {
        totalWords += c;
    }

    struct HeapEntry
    {
        std::string word;
        std::size_t count;
        std::size_t part;
    };

    auto cmp = [](const HeapEntry& a, const HeapEntry& b) 
    {
        if (a.word != b.word) return a.word > b.word;
        return a.part > b.part;
    };
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, decltype(cmp)> pq(cmp);

    std::vector<std::unique_ptr<std::ifstream>> ins;
    ins.reserve(partPaths.size());
    for (std::size_t i = 0; i < partPaths.size(); ++i)
    {
        auto in = std::make_unique<std::ifstream>(partPaths[i], std::ios::binary);

        if (!in->good())
        {
            throw std::runtime_error("Не открыть temp: " + partPaths[i]);
        }      

        HeapEntry e;
        if (readFreqPartial(*in, e.word, e.count)) 
        {
            e.part = i;
            pq.push(std::move(e));
        }
        ins.push_back(std::move(in));
    }

    std::vector<entry> merged;
    std::string curWord;
    std::size_t curCount = 0;
    bool haveCur = false;

    while (!pq.empty())
    {
        HeapEntry e = pq.top();
        pq.pop();

        if (!haveCur) 
        {
            curWord  = std::move(e.word);
            curCount = e.count;
            haveCur  = true;
        } else if (e.word == curWord) {
            curCount += e.count;
        } else {
            merged.push_back({std::move(curWord), curCount});
            curWord  = std::move(e.word);
            curCount = e.count;
        }

        HeapEntry ne;
        if (readFreqPartial(*ins[e.part], ne.word, ne.count)) 
        {
            ne.part = e.part;
            pq.push(std::move(ne));
        }
    }

    if (haveCur)
    {
        merged.push_back({std::move(curWord), curCount});
    }
        
    std::sort(merged.begin(), merged.end(),
              [](const entry& a, const entry& b) {
                  if (a.count != b.count) return a.count > b.count;
                  return a.word < b.word;
              });

    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        throw std::runtime_error("Не создать: " + outPath);
    }
    out << "Всего слов       : " << totalWords        << '\n';
    out << "Уникальных слов  : " << merged.size()     << '\n';
    out << '\n';
    for (const auto& e : merged)
    {
        out << e.word << " - " << e.count << '\n';
    }
    out.close();

    for (const auto& p : partPaths)
    {
        std::remove(p.c_str());
    }    
}

void wordFrequency::reset() noexcept
{
    freq_.clear();
    entries_.clear();
    total_ = 0;
}

void wordFrequency::rebuildEntries()
{
    entries_.clear();
    entries_.reserve(freq_.size());

    for (const auto& kv : freq_)
    {
        entries_.push_back({kv.first, kv.second});
    }
        
    std::sort(entries_.begin(), entries_.end(),
              [](const entry& a, const entry& b) {
                  if (a.count != b.count) return a.count > b.count;
                  return a.word < b.word;
              });
}

void wordIndex::process(std::string_view text, unsigned options,
                        std::size_t nThreads)
{
    if (nThreads <= 1)
    {
        std::unordered_map<std::string, std::size_t> lookup;
        std::size_t position = 0;

        utf8::forEachWord(text, [&](std::string_view w) 
        {
            std::string key = (options & ignoreCase) ? utf8::toLower(w)
                                                     : std::string(w);
            auto it = lookup.find(key);
            if (it == lookup.end())
            {
                lookup.emplace(key, entries_.size());
                entries_.push_back({std::move(key), {position}});
            } else {
                entries_[it->second].positions.push_back(position);
            }
            ++position;
        });
        return;
    }

    const auto chunks = parallel::splitChunks(text, nThreads);
    const std::size_t n = chunks.size();

    struct Local
    {
        std::unordered_map<std::string, std::size_t> lookup;
        std::vector<entry> entries;
        std::size_t wordCount = 0;
    };
    std::vector<Local> locals(n);
    std::vector<std::thread> threads;
    threads.reserve(n);

    // каждая нить обрабатывает свой чанк и пишет позиции
    // относительно начала чанка
    for (std::size_t t = 0; t < n; ++t)
    {
        threads.emplace_back([&, t] {
            const auto [b, e] = chunks[t];
            auto& L = locals[t];
            std::size_t pos = 0;

            utf8::forEachWord(text.substr(b, e - b), [&](std::string_view w) 
            {
                std::string key = (options & ignoreCase) ? utf8::toLower(w)
                                                         : std::string(w);
                auto it = L.lookup.find(key);
                if (it == L.lookup.end())
                {
                    L.lookup.emplace(key, L.entries.size());
                    L.entries.push_back({std::move(key), {pos}});
                } else {
                    L.entries[it->second].positions.push_back(pos);
                }
                ++pos;
            });
            L.wordCount = pos;
        });
    }
    for (auto& th : threads)
    {
        th.join();
    }
    // сдвиги — сколько слов было до каждого чанка
    std::vector<std::size_t> offset(n, 0);
    for (std::size_t t = 1; t < n; ++t)
    {
        offset[t] = offset[t - 1] + locals[t - 1].wordCount;
    }

    // сливаем чанки слева направо
    std::unordered_map<std::string, std::size_t> globalIndex;
    for (std::size_t t = 0; t < n; ++t)
    {
        const std::size_t base = offset[t];
        for (auto& e : locals[t].entries)
        {
            auto it = globalIndex.find(e.word);
            if (it == globalIndex.end())
            {
                for (auto& p : e.positions) p += base;
                globalIndex.emplace(e.word, entries_.size());
                entries_.push_back(std::move(e));
            } else {
                auto& dst = entries_[it->second].positions;
                for (auto p : e.positions) dst.push_back(p + base);
            }
        }
    }
}

void wordIndex::reset() noexcept
{
    entries_.clear();
}