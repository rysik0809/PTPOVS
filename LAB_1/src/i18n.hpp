#pragma once

#include <string>
#include <string_view>

/**
 * @brief Мини-модуль локализации интерфейса.
 */
namespace i18n {

/**
 * @brief Инициализация: определить язык и загрузить переводы.
 * @param path Путь к JSON-файлу с переводами.
 *
 * @note Вызывать один раз в начале main() до любого tr().
 *       Повторные вызовы безопасны.
 */
void init(const std::string& path = "locales/strings.json");

/**
 * @brief Перевод по ключу.
 * @param key Ключ в файле переводов (например, "menu.freq").
 * @return Ссылка на строку перевода или на сам @p key.
 */
const std::string& tr(const char* key);

/**
 * @brief Текущий выбранный язык (например, "ru" или "en").
 */
std::string_view currentLanguage() noexcept;

} // namespace i18n