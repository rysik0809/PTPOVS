#include "word_stats.hpp"
#include "text_utils.hpp"

#include <algorithm>
#include <thread>
#include <utility>

void wordFrequency::process(std::string_view text, unsigned options,
                            std::size_t nThreads)
{
    if (nThreads <= 1)
    {
        utf8::forEachWord(text, [&](std::string_view w) {
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
            utf8::forEachWord(text.substr(b, e - b), [&](std::string_view w) {
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
            freq_[std::move(kv.first)] += kv.second;
    }

    rebuildEntries();
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
        entries_.push_back({kv.first, kv.second});

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

        utf8::forEachWord(text, [&](std::string_view w) {
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

            utf8::forEachWord(text.substr(b, e - b), [&](std::string_view w) {
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
    for (auto& th : threads) th.join();

    // сдвиги — сколько слов было до каждого чанка
    std::vector<std::size_t> offset(n, 0);
    for (std::size_t t = 1; t < n; ++t)
        offset[t] = offset[t - 1] + locals[t - 1].wordCount;

    // сливаем чанки слева направо
    std::unordered_map<std::string, std::size_t> globalIndex;
    for (std::size_t t = 0; t < n; ++t)
    {
        const std::size_t base = offset[t];
        for (auto& e : locals[t].entries)
        {
            auto it = globalIndex.find(e.word);
            if (it == globalIndex.end()) {
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