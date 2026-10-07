#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/**
 * @brief Владелец mmap-отображения файла в память.
 * @note Деструктор автоматически закрывает отображение.
 * @warning std::string_view, возвращённый view(), валиден только
 *          пока жив объект fileBuffer.
 */
class fileBuffer
{
public:
    /**
     * @brief Открывает файл и отображает его в память через mmap().
     * @param path Путь к файлу на диске.
     * @throws std::runtime_error Если файл не открылся (open),
     *         не удалось узнать размер (fstat) или отображение
     *         не создалось (mmap вернул MAP_FAILED).
     */
    explicit fileBuffer(const std::string& path);

    /// @brief Закрывает отображение и файловый дескриптор.
    ~fileBuffer();

    /**
     * @brief Невладеющий вид на содержимое файла.
     * @return std::string_view на всё содержимое файла.
     * @warning Результат валиден только пока жив *this.
     */
    std::string_view view() const noexcept { return {data_, size_}; }

    /// @brief Размер файла в байтах.
    std::size_t size() const noexcept { return size_; }

    /// @brief Копирование запрещено — объект владеет ресурсом.
    fileBuffer(const fileBuffer&) = delete;

    /// @brief Копирующее присваивание запрещено — объект владеет ресурсом.
    fileBuffer& operator=(const fileBuffer&) = delete;

    /**
     * @brief Перемещающий конструктор.
     * @param other Источник; после перемещения пуст.
     */
    fileBuffer(fileBuffer&& other) noexcept;

    /**
     * @brief Перемещающее присваивание.
     * @param other Источник; после перемещения пуст.
     * @return Ссылка на *this.
     */
    fileBuffer& operator=(fileBuffer&& other) noexcept;

private:
    /// @brief Закрывает отображение и дескриптор.
    void close() noexcept;

    const char* data_ = nullptr;  ///< Начало mmap-отображения или nullptr.
    std::size_t size_ = 0;        ///< Размер файла в байтах.
    int         fd_   = -1;       ///< Файловый дескриптор или -1.
};


/**
 * @brief Утилиты для работы с UTF-8.
 */
namespace utf8 {

/**
 * @brief Декодирует одну кодовую точку UTF-8.
 * @param[in]     s   Исходная строка.
 * @param[in,out] i   Позиция начала кодпоинта.
 * @param[out]    cp  Декодированный кодпоинт.
 * @return true при успехе; false — битый байт или обрыв.
 * @note Не валидирует overlong-последовательности и суррогаты —
 *       для задач подсчёта слов это не критично.
 */
bool decodeOne(std::string_view s, std::size_t& i, char32_t& cp) noexcept;

/**
 * @brief Проверяет, считается ли кодпоинт «словесным».
 *
 * @param cp Кодпоинт.
 *
 * @return true, если кодпоинт — часть слова.
 */
bool isWordCodepoint(char32_t cp) noexcept;

/**
 * @brief Приводит UTF-8-строку к нижнему регистру.
 * @param s Исходная строка в UTF-8.
 * @return Новая строка в нижнем регистре.
 * @note Битые байты копируются как есть — функция не бросает.
 */
std::string toLower(std::string_view s);

/**
 * @brief Обходит все слова текста, вызывая @p fn на каждом.
 * @tparam Fn Тип callback'а. Должен принимать std::string_view.
 * @param text Исходный текст в UTF-8.
 * @param fn   Callback. Получает string_view — «окно» в исходный
 *             текст без копирования.
 */
template <class Fn>
void forEachWord(std::string_view text, Fn&& fn)
{
    std::size_t i = 0;
    while (i < text.size())
    {
        const std::size_t start = i;
        char32_t cp = 0;

        // Пропускаем всё, что не является началом слова:
        // разделители и битые байты
        if (!decodeOne(text, i, cp)) { ++i; continue; }
        if (!isWordCodepoint(cp))    continue;

        // Тянем слово до первого не-словесного кодпоинта.
        // Если встретили битый байт — откатываемся на save, чтобы
        // он не попал внутрь слова
        while (i < text.size())
        {
            const std::size_t save = i;
            if (!decodeOne(text, i, cp) || !isWordCodepoint(cp))
            {
                i = save;
                break;
            }
        }
        fn(text.substr(start, i - start));
    }
}

} // namespace utf8

/**
 * @brief Вспомогательные утилиты для многопоточной обработки.
 */
namespace parallel {

/**
 * @brief Полуинтервал [begin, end) — границы чанка в исходном тексте.
 */
using Chunk = std::pair<std::size_t, std::size_t>;

/**
 * @brief Разбивает текст на @p n чанков по границам слов.
 * @param text Исходный текст.
 * @param n    Желаемое число чанков (минимум 1).
 * @return Вектор непересекающихся полуинтервалов, покрывающих
 *         весь текст.
 */
std::vector<Chunk> splitChunks(std::string_view text, std::size_t n);

/**
 * @brief Возвращает число аппаратных потоков.
 * @return Количество логических ядер, минимум 1.
 */
std::size_t hardwareThreads();

} // namespace parallel