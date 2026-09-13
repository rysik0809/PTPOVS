#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstddef>

/**
 * @brief Подсчёт уникальных слов в тексте.
 *
 * Собирает все слова, считает их частоты и хранит результат,
 * отсортированный по убыванию частоты (при равенстве — по алфавиту).
 */
class wordFrequency
{
public:
    enum options : unsigned
    {
        none        = 0,            ///< учитывать регистр
        ignoreCase  = 1u << 0,      ///< приводить слова к нижнему регистру
    };

    struct entry
    {
        std::string word;
        std::size_t count;
    };

    wordFrequency() = default;

    /// Обработать текст, добавив слова к накопленной статистике.
    void process(std::string_view text, unsigned options = none);

    /// Сбросить накопленный результат.
    void reset() noexcept;

    /// Количество уникальных слов.
    std::size_t uniqueCount() const noexcept { return entries_.size(); }

    /// Общее количество слов (с повторами).
    std::size_t totalCount()  const noexcept { return total_; }

    /// Результат, отсортированный по частоте.
    const std::vector<entry>& entries() const noexcept { return entries_; }

private:
    std::vector<entry> entries_;
    std::size_t total_ = 0;
};