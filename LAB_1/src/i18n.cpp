#include "i18n.hpp"

#include <nlohmann/json.hpp>

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

#ifdef _WIN32
#  include <windows.h>
#endif

namespace i18n {

namespace {

/// Возвращает язык системы в виде строки: "ru_RU", "ru-RU", "C" и т.п.
std::string detectSystemLanguage()
{
#ifdef _WIN32
    wchar_t buf[LOCALE_NAME_MAX_LENGTH] = {};
    if (GetUserDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH))
    {
        const int need = WideCharToMultiByte(CP_UTF8, 0, buf, -1,
                                             nullptr, 0, nullptr, nullptr);
        if (need > 1)
        {
            std::string s(static_cast<std::size_t>(need - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf, -1,
                                s.data(), need, nullptr, nullptr);
            return s;  // например, "ru-RU"
        }
    }
    return "en-US";
#else
    for (const char* var : {"LC_ALL", "LC_MESSAGES", "LANG"})
    {
        if (const char* v = std::getenv(var); v && *v)
        {
            std::string s = v;
            if (auto dot = s.find('.'); dot != std::string::npos)
                s.resize(dot);          // "ru_RU.UTF-8" → "ru_RU"
            return s;
        }
    }
    return "C";
#endif
}

/// "ru_RU" / "ru-RU" / "ru" → "ru"; "C" / "POSIX" → "en".
std::string normalizeLang(const std::string& s)
{
    if (s.empty()) return "en";

    const std::size_t sep = s.find_first_of("_-");
    std::string lang = (sep == std::string::npos) ? s : s.substr(0, sep);

    for (char& c : lang)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lang == "c" || lang == "posix") return "en";
    return lang;
}

std::unordered_map<std::string, std::string> g_strings;
std::string g_lang = "en";

} // namespace

void init(const std::string& path)
{
    g_lang = normalizeLang(detectSystemLanguage());

    std::ifstream f(path);
    if (!f) return;

    auto j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded() || !j.is_object()) return;

    const nlohmann::json* langObj = nullptr;

    if (j.contains(g_lang) && j[g_lang].is_object())
        langObj = &j[g_lang];
    else if (j.contains("en") && j["en"].is_object())
        langObj = &j["en"];

    if (!langObj) return;

    for (auto it = langObj->begin(); it != langObj->end(); ++it)
    {
        if (it.value().is_string())
            g_strings[it.key()] = it.value().get<std::string>();
    }
}

const std::string& tr(const char* key)
{
    auto it = g_strings.find(key);
    if (it != g_strings.end()) return it->second;

    static std::unordered_map<std::string, std::string> fallback;
    auto [it2, inserted] = fallback.try_emplace(key, key);
    (void)inserted;
    return it2->second;
}

std::string_view currentLanguage() noexcept
{
    return g_lang;
}

} // namespace i18n