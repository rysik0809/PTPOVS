#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace utf8 
{

/// Является ли кодовая точка «словесным» символом (буква/цифра).
bool isWordCodepoint(char32_t cp) noexcept;

/// Приведение кодовой точки к нижнему регистру (ASCII + кириллица).
char32_t toLowerCp(char32_t cp) noexcept;

/// Приведение UTF-8-строки к нижнему регистру.
std::string toLower(std::string_view s);

/// Разбиение текста на «слова» (виды на исходную строку).
std::vector<std::string_view> splitWords(std::string_view text);

} // namespace utf8