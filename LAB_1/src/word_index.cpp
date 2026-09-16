#include "word_index.hpp"
#include "utf8_utils.hpp"

#include <unordered_map>

void wordIndex::process(std::string_view text, unsigned options)
{
    const bool ignore = (options & ignoreCase) != 0;

    std::unordered_map<std::string, std::size_t> lookup; // word -> index в entries_
    std::size_t position = 0;

    for (auto w : utf8::splitWords(text))
    {
        std::string key = ignore ? utf8::toLower(w) : std::string(w);

        auto it = lookup.find(key);
        if (it == lookup.end())
        {
            lookup.emplace(key, entries_.size());
            entries_.push_back({std::move(key), {position}});
        }
        else
        {
            entries_[it->second].positions.push_back(position);
        }

        ++position;
    }
}

void wordIndex::reset() noexcept
{
    entries_.clear();
}