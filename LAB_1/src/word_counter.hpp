#pragma once

#include <string_view>
#include <cstddef>
#include <string>

/**
 * @brief Счётчик вхождений слова в тексте.
 *
 * Три режима поиска:
 *   - none       — сырой поиск подстроки;
 *   - wholeWord  — только отдельные слова;
 *   - ignoreCase — регистронезависимо.
 */
class wordCounter
{
public:
    enum options : unsigned
    {
        none = 0,               ///< сырой поиск
        wholeWord = 1u << 0,    ///< строгий поиск
        ignoreCase = 1u << 1,   ///< учитывать регистр
    };

    wordCounter() = default;

    /**
     * @param text    текст для поиска.
     * @param word    искомое слово.
     * @param options комбинация флагов options.
     * @return число вхождений.
     */
    std::size_t count(std::string_view text,
                      std::string_view word,
                      unsigned options = none) const;

    // std::size_t countWholeWord(std::string_view text,
    //                   std::string_view word) const;

    // std::size_t countIgnoreCase(std::string_view text,
    //                             std::string_view word) const;

private:
    // static bool isWordByte(unsigned char c) noexcept;
    // static std::string toLower(std::string_view s);

    /**
     * @brief Ядро поиска: побайтовое сравнение подстроки.
     * @param text    текст для поиска.
     * @param word    искомое слово.
     */
    static std::size_t countRaw(std::string_view text,
                                std::string_view word) noexcept;

};