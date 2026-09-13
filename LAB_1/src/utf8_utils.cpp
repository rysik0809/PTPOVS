#include "utf8_utils.hpp"

namespace utf8 {

namespace {

bool decodeOne(std::string_view s, std::size_t& i, char32_t& cp) noexcept
{
    if (i >= s.size()) return false;

    const unsigned char c0 = static_cast<unsigned char>(s[i]);

    if (c0 < 0x80) { cp = c0; ++i; return true; }

    if ((c0 & 0xE0) == 0xC0) {
        if (i + 1 >= s.size()) return false;
        const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
        if ((c1 & 0xC0) != 0x80) return false;
        cp = ((c0 & 0x1Fu) << 6) | (c1 & 0x3Fu);
        i += 2;
        return true;
    }

    if ((c0 & 0xF0) == 0xE0) {
        if (i + 2 >= s.size()) return false;
        const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
        const unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return false;
        cp = ((c0 & 0x0Fu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu);
        i += 3;
        return true;
    }

    if ((c0 & 0xF8) == 0xF0) {
        if (i + 3 >= s.size()) return false;
        const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
        const unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
        const unsigned char c3 = static_cast<unsigned char>(s[i + 3]);
        if ((c1 & 0xC0) != 0x80 ||
            (c2 & 0xC0) != 0x80 ||
            (c3 & 0xC0) != 0x80) return false;
        cp = ((c0 & 0x07u) << 18) | ((c1 & 0x3Fu) << 12)
           | ((c2 & 0x3Fu) << 6)  | (c3 & 0x3Fu);
        i += 4;
        return true;
    }

    return false;
}

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
    if (cp >= U'A' && cp <= U'Z') return true;
    if (cp >= U'a' && cp <= U'z') return true;
    if (cp >= U'0' && cp <= U'9') return true;

    // Латиница с диакритикой
    if (cp >= 0x00C0 && cp <= 0x024F) return true;

    // Кириллица: основной блок + дополнение
    if (cp >= 0x0400 && cp <= 0x04FF) return true;
    if (cp >= 0x0500 && cp <= 0x052F) return true;

    return false;
}

char32_t toLowerCp(char32_t cp) noexcept
{
    if (cp >= U'A' && cp <= U'Z') return cp + 32;        // A-Z  -> a-z
    if (cp >= 0x0410 && cp <= 0x042F) return cp + 0x20;  // А-Я  -> а-я
    if (cp >= 0x0400 && cp <= 0x040F) return cp + 0x50;  // Ѐ-Џ  -> ѐ-џ
    return cp;
}

std::string toLower(std::string_view s)
{
    std::string out;
    out.reserve(s.size());

    std::size_t i = 0;
    while (i < s.size()) {
        const std::size_t start = i;
        char32_t cp = 0;
        if (!decodeOne(s, i, cp)) {
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
    while (i < text.size()) {
        std::size_t save = i;
        char32_t cp = 0;

        if (!decodeOne(text, i, cp)) {
            i = save + 1;   // некорректный байт — пропускаем
            continue;
        }
        if (!isWordCodepoint(cp)) {
            continue;
        }

        const std::size_t start = save;
        std::size_t end = i;

        while (i < text.size()) {
            std::size_t save2 = i;
            char32_t cp2 = 0;
            if (!decodeOne(text, i, cp2)) {
                i = save2 + 1;
                break;
            }
            if (!isWordCodepoint(cp2)) {
                break;
            }
            end = i;
        }

        words.push_back(text.substr(start, end - start));
    }

    return words;
}

} // namespace utf8