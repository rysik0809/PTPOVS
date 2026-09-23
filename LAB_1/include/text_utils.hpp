#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/**
 * @brief Владелец всего содержимого файла в памяти
 *
 * После успешного конструирования text_ содержит
 * все байты файла
 *
 * - Копирование запрещено
 * - Перемещение разрешено
 */
class fileBuffer
{
public:
    /// @throws std::runtime_error — файл не открылся или чтение упало
    explicit fileBuffer(const std::string& path);

    /// Невладеющий вид на содержимое. Валиден, пока жив *this*
    /// Zero-copy: копируется только указатель + длина (16 байт)
    std::string_view view() const noexcept { return text_; }
    
    /// Ссылка на владеющую строку
    const std::string& str() const noexcept { return text_; }
    
    std::size_t size() const noexcept { return text_.size(); }
    bool empty() const noexcept { return text_.empty(); }

    fileBuffer(const fileBuffer&) = delete;
    fileBuffer& operator=(const fileBuffer&) = delete;
    fileBuffer(fileBuffer&&) noexcept = default;
    fileBuffer& operator=(fileBuffer&&) noexcept = default;

private:
    std::string text_;
};


namespace utf8 {

/**
 * @brief Декодирует одну кодовую точку UTF-8.
 *
 * @param s   исходная строка
 * @param i   [in/out] позиция начала; при успехе сдвигается вперёд
 *            на длину последовательности 
 * @param cp  [out] кодпоинт
 * @return    true при успехе; false — битый байт или обрыв
 */
bool decodeOne(std::string_view s, std::size_t& i, char32_t& cp) noexcept;

/// Буква или цифра?
bool isWordCodepoint(char32_t cp) noexcept;

/// Привести кодпоинт к нижнему регистру
char32_t toLowerCp(char32_t cp) noexcept;

/// Привести UTF-8-строку к нижнему регистру
std::string toLower(std::string_view s);


/**
 * @brief Обход слов текста через callback.
 *
 * Callback получает string_view — «окно» в исходный text
 * Вид валиден, пока жив text
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

        // Тянем слово до первого не-словесного кодпоинта
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


namespace parallel {

/// Полуинтервал [begin, end) — границы чанка в исходном тексте
using Chunk = std::pair<std::size_t, std::size_t>; 

/// Разбить текст на n частей по границам слов
std::vector<Chunk> splitChunks(std::string_view text, std::size_t n);

/// Число потоков (минимум 1)
std::size_t hardwareThreads();

} // namespace parallel