#include "word_frequency.hpp"
#include "utf8_utils.hpp"

#include <unordered_map>
#include <algorithm>

void wordFrequency::process(std::string_view text, unsigned options)
{
    const bool ignore = (options & ignoreCase) != 0;

    std::unordered_map<std::string, std::size_t> freq;
    for (auto w : utf8::splitWords(text)) {
        std::string key = ignore ? utf8::toLower(w) : std::string(w);
        ++freq[std::move(key)];
    }

    // Переносим в вектор и сортируем.
    for (auto& kv : freq) {
        total_ += kv.second;
        entries_.push_back({std::move(kv.first), kv.second});
    }

    std::sort(entries_.begin(), entries_.end(),
              [](const entry& a, const entry& b) {
                  if (a.count != b.count) return a.count > b.count;
                  return a.word < b.word;
              });
}

void wordFrequency::reset() noexcept
{
    entries_.clear();
    total_ = 0;
}