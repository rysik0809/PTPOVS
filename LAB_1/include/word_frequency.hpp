#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/**
 * @brief Подсчёт уникальных слов в тексте.
 *
 * Накапливает частоты слов. Результат отсортирован по убыванию частоты,
 * при равенстве — по алфавиту.
 */
class wordFrequency
{
public:
    enum options : unsigned
    {
        none        = 0,        ///< учитывать регистр
        ignoreCase  = 1u << 0,  ///< приводить слова к нижнему регистру
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
    std::size_t totalCount() const noexcept { return total_; }

    /// Результат, отсортированный по частоте.
    const std::vector<entry>& entries() const noexcept { return entries_; }

private:
    /// Пересобирает отсортированный вектор entries_ из freq_.
    void rebuildEntries();

    std::unordered_map<std::string, std::size_t> freq_;
    std::vector<entry> entries_;
    std::size_t total_ = 0;
};