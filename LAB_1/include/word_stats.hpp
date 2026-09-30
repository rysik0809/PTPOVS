#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/**
 * @brief Считает частоты слов в тексте.
 */
class wordFrequency
{
public:
    /**
     * @brief Опции обработки текста.
     */
    enum options : unsigned
    {
        none       = 0,        ///< Без нормализации — слова как есть.
        ignoreCase = 1u << 0,  ///< Приводить слова к нижнему регистру.
    };

    /**
     * @brief Одна строка результата: слово и его частота.
     */
    struct entry
    {
        std::string word;    ///< Слово (в нижнем регистре, если ignoreCase).
        std::size_t count;   ///< Сколько раз встретилось.
    };

    /**
     * @brief Считает частоты в памяти (для небольших файлов).
     * @param text     Исходный текст в UTF-8.
     * @param options  Опции обработки (см. options).
     * @param nThreads Число потоков, ≥ 1.
     * 
     * @note После возврата результат доступен через entries(),
     *       uniqueCount() и totalCount().
     */
    void process(std::string_view text, unsigned options = none,
                 std::size_t nThreads = 1);

    /**
     * @brief Считает частоты через temp-файлы (для больших файлов).
     *
     * @param text             Исходный текст в UTF-8.
     * @param tmpDir           Каталог для temp-файлов.
     * @param outPath          Путь к выходному текстовому отчёту.
     * @param options          Опции обработки.
     * @param targetChunkBytes Желаемый размер чанка в байтах.
     * 
     * @warning Требуется достаточно места на диске под temp-файлы.
     */
    void processChunked(std::string_view text,
                        const std::string& tmpDir,
                        const std::string& outPath,
                        unsigned options = none,
                        std::size_t targetChunkBytes = 512ull * 1024 * 1024);

    /**
     * @brief Сбрасывает всё состояние к исходному.
     */
    void reset() noexcept;

    /**
     * @brief Число уникальных слов в последнем результате.
     * @return Размер entries().
     */
    std::size_t uniqueCount() const noexcept { return entries_.size(); }

    /**
     * @brief Общее число слов, обработанных в последнем вызове.
     * @return Сумма всех count по всем словам.
     */
    std::size_t totalCount() const noexcept { return total_; }

    /**
     * @brief Отсортированный список слов с частотами.
     * @return Константная ссылка на внутренний вектор.
     */
    const std::vector<entry>& entries() const noexcept { return entries_; }

private:
    /**
     * @brief Пересобирает @c entries_ из @c freq_ и сортирует.
     */
    void rebuildEntries();

    std::unordered_map<std::string, std::size_t> freq_;  ///< «слово -> количество».
    std::vector<entry> entries_;                         ///< Отсортированный кеш.
    std::size_t total_ = 0;                              ///< Общее число слов.
};


/**
 * @brief Для каждого уникального слова хранит позиции в тексте.
 * 
 * @warning Хранит все позиции всех вхождений — на больших файлах
 *          легко упирается в RAM. Для файлов свыше ~1 ГБ не
 *          предназначен; используйте wordFrequency.
 */
class wordIndex
{
public:
    /**
     * @brief Опции обработки текста.
     */
    enum options : unsigned
    {
        none       = 0,        ///< Без нормализации — слова как есть.
        ignoreCase = 1u << 0,  ///< Приводить слова к нижнему регистру.
    };

    /**
     * @brief Одна запись: слово и все его позиции в тексте.
     */
    struct entry
    {
        std::string word;                     ///< Слово.
        std::vector<std::size_t> positions;   ///< Позиции (0-based, возрастают).
    };

    /**
     * @brief Строит индекс позиций.
     *
     * @param text     Исходный текст в UTF-8.
     * @param options  Опции обработки (см. options).
     * @param nThreads Число потоков, ≥ 1.
     *
     * @note После возврата результат доступен через entries()
     *       и uniqueCount().
     */
    void process(std::string_view text, unsigned options = none,
                 std::size_t nThreads = 1);

    /**
     * @brief Сбрасывает индекс к исходному состоянию.
     */
    void reset() noexcept;

    /**
     * @brief Число уникальных слов в индексе.
     * @return Размер entries().
     */
    std::size_t uniqueCount() const noexcept { return entries_.size(); }

    /**
     * @brief Все записи индекса в порядке первого появления.
     *
     * @return Константная ссылка на внутренний вектор.
     * @warning Ссылка инвалидируется после вызова process()
     *          или reset().
     */
    const std::vector<entry>& entries() const noexcept { return entries_; }

private:
    std::vector<entry> entries_;  ///< Слова и их позиции.
};