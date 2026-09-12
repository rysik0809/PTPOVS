#include "word_counter.hpp"

#include <cctype>
#include <cstring>
#include <string>
#include <algorithm>
#include <iostream>

std::size_t wordCounter::countRaw(std::string_view text,
                                     std::string_view word) noexcept
{
    if(word.empty() || word.size() > text.size())
    {
        std::cerr << "Ошибка 1";
        return 0;
    }

    const char first = word[0];
    const std::size_t m = word.size();
    const std::size_t limit = text.size() - m;

    std::size_t count = 0;
    for(std::size_t i = 0; i <= limit; ++i)
    {
        if(text[i] != first)
        {
            continue;
        }

        if(std::memcmp(text.data() + i, word.data(), m) == 0)
        {
            ++count;
            i += m - 1;
        }
    }
    return count;
}

std::size_t wordCounter::count(std::string_view text,
                                std::string_view word,
                                unsigned options) const
{
    //const bool wholeWord = options & wholeWord;
    //const bool ignoreCase = options & ignoreCase;

    //std::string textCopy;
    //std::string wordCopy;

    std::string_view t = text;
    std::string_view w = word;

    return countRaw(t, w);
}