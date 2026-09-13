#include "word_frequency.hpp"
#include "utf8_utils.hpp"

#include <algorithm>
#include <utility>

void wordFrequency::process(std::string_view text, unsigned options)
{
    const bool ignore = (options & ignoreCase) != 0;

    for (auto w : utf8::splitWords(text)) 
    {
        std::string key = ignore ? utf8::toLower(w) : std::string(w);

        ++freq_[std::move(key)];
        ++total_;
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

    for (const auto& kv : freq_) {
        entries_.push_back({kv.first, kv.second});
    }

    std::sort(entries_.begin(), entries_.end(),
              [](const entry& a, const entry& b) {
                  if (a.count != b.count) return a.count > b.count;
                    return a.word < b.word;
              });
}