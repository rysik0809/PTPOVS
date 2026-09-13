#include "utf8_utils.hpp"

namespace utf8 {

namespace {

/**
 * @brief Декодирует одну кодовую точку UTF-8.
 *
 * Поддерживает корректные последовательности 1–4 байт.
 * Отклоняет overlong-кодировки, surrogate-диапазон и значения > U+10FFFF.
 *
 * @param s исходная строка
 * @param i индекс начала последовательности; при успехе сдвигается вперёд
 * @param cp результат — кодовая точка
 * @return true при успешном декодировании
 */
bool decodeOne(std::string_view s, std::size_t& i, char32_t& cp) noexcept
{
    if (i >= s.size()) return false;

    const auto c0 = static_cast<unsigned char>(s[i]);

    if (c0 < 0x80) 
    {
        cp = c0;
        ++i;
        return true;
    }

    std::size_t len = 0;
    char32_t minCp = 0;

    if ((c0 & 0xE0) == 0xC0) {
        len = 2;
        cp = c0 & 0x1F;
        minCp = 0x80;
    } else if ((c0 & 0xF0) == 0xE0) {
        len = 3;
        cp = c0 & 0x0F;
        minCp = 0x800;
    } else if ((c0 & 0xF8) == 0xF0) {
        len = 4;
        cp = c0 & 0x07;
        minCp = 0x10000;
    } else {
        return false;
    }

    if (i + len > s.size()) return false;

    for (std::size_t k = 1; k < len; ++k) 
    {
        const auto c = static_cast<unsigned char>(s[i + k]);
        if ((c & 0xC0) != 0x80) return false;
        cp = (cp << 6) | (c & 0x3F);
    }

    // Отсекаем некорректные кодовые точки.
    if (cp < minCp) return false;
    if (cp >= 0xD800 && cp <= 0xDFFF) return false;
    if (cp > 0x10FFFF) return false;

    i += len;
    return true;
}

/// Кодирует кодовую точку обратно в UTF-8 и дописывает в строку.
void appendUtf8(std::string& out, char32_t cp)
{
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

} // namespace

bool isWordCodepoint(char32_t cp) noexcept
{
    // ASCII: буквы и цифры.
    if (cp >= U'A' && cp <= U'Z') return true;
    if (cp >= U'a' && cp <= U'z') return true;
    if (cp >= U'0' && cp <= U'9') return true;

    // Латиница с диакритикой.
    if (cp >= 0x00C0 && cp <= 0x024F) return true;

    // Кириллица: основной блок и дополнение.
    if (cp >= 0x0400 && cp <= 0x04FF) return true;
    if (cp >= 0x0500 && cp <= 0x052F) return true;

    return false;
}

char32_t toLowerCp(char32_t cp) noexcept
{
    if (cp >= U'A' && cp <= U'Z') return cp + 32;

    if (cp >= 0x0410 && cp <= 0x042F) return cp + 0x20;

    if (cp >= 0x0400 && cp <= 0x040F) return cp + 0x50;

    return cp;
}

std::string toLower(std::string_view s)
{
    std::string out;
    out.reserve(s.size());

    std::size_t i = 0;
    while (i < s.size()) 
    {
        const std::size_t start = i;
        char32_t cp = 0;

        if (!decodeOne(s, i, cp)) 
        {
            // Некорректный байт оставляем как есть, чтобы не терять данные.
            out.push_back(s[start]);
            i = start + 1;
            continue;
        }

        appendUtf8(out, toLowerCp(cp));
    }

    return out;
}

std::vector<std::string_view> splitWords(std::string_view text)
{
    std::vector<std::string_view> words;

    std::size_t i = 0;
    while (i < text.size()) 
    {
        const std::size_t start = i;
        char32_t cp = 0;

        if (!decodeOne(text, i, cp)) 
        {
            ++i; // пропускаем некорректный байт
            continue;
        }
        if (!isWordCodepoint(cp)) 
        {
            continue;
        }

        std::size_t end = i;
        while (i < text.size()) 
        {
            const std::size_t save = i;

            if (!decodeOne(text, i, cp) || !isWordCodepoint(cp)) 
            {
                i = save; // возвращаемся к разделителю или ошибке
                break;
            }
            end = i;
        }

        words.push_back(text.substr(start, end - start));
    }

    return words;
}

} // namespace utf8