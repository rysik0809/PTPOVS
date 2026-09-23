#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/**
 * @brief Считает частоты слов в тексте.
 *
 * Внутри две структуры:
 *   freq_    — хеш-карта «слово -> количество», быстрое обновление
 *   entries_ — кеш отсортированного результата, отдаётся наружу
 */
class wordFrequency
{
public:
    enum options : unsigned
    {
        none       = 0,
        ignoreCase = 1u << 0,
    };

    struct entry
    {
        std::string word;
        std::size_t count;
    };

    void process(std::string_view text, unsigned options = none,
                 std::size_t nThreads = 1);

    void reset() noexcept;

    std::size_t uniqueCount() const noexcept { return entries_.size(); }
    std::size_t totalCount()  const noexcept { return total_; }
    const std::vector<entry>& entries() const noexcept { return entries_; }

private:
    void rebuildEntries();

    std::unordered_map<std::string, std::size_t> freq_;
    std::vector<entry> entries_;
    std::size_t total_ = 0;
};

/**
 * @brief Для каждого уникального слова хранит позиции в тексте.
 */
class wordIndex
{
public:
    enum options : unsigned
    {
        none       = 0,
        ignoreCase = 1u << 0,
    };

    struct entry
    {
        std::string word;
        std::vector<std::size_t> positions;
    };

    void process(std::string_view text, unsigned options = none,
                 std::size_t nThreads = 1);

    void reset() noexcept;

    std::size_t uniqueCount() const noexcept { return entries_.size(); }
    const std::vector<entry>& entries() const noexcept { return entries_; }

private:
    std::vector<entry> entries_;
};