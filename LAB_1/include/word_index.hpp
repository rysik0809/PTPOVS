#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstddef>

/**
 * @brief Индексация позиций слов в тексте.
 *
 * Для каждого уникального слова хранит список позиций (0-based)
 * в порядке их появления в тексте.
 */
class wordIndex
{
public:
    enum options : unsigned
    {
        none        = 0,
        ignoreCase  = 1u << 0,   ///< приводить слова к нижнему регистру
    };

    struct entry
    {
        std::string word;
        std::vector<std::size_t> positions;
    };

    wordIndex() = default;

    /// Разобрать текст, добавив слова к накопленному индексу.
    void process(std::string_view text, unsigned options = none);

    /// Сбросить накопленный индекс.
    void reset() noexcept;

    /// Количество уникальных слов.
    std::size_t uniqueCount() const noexcept { return entries_.size(); }

    /// Записи в порядке первого появления слова.
    const std::vector<entry>& entries() const noexcept { return entries_; }

private:
    std::vector<entry> entries_;
};