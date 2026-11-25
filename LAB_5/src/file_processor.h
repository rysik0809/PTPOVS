#pragma once

#include "exceptions.h"
#include "file_result.h"

#include <string>

/// Возвращает путь к файлу с полным результатом обработки данного файла.
std::string resultsPathFor(const std::string& inputPath);

// ─── Функции-обработчики исключительных ситуаций (задание №3, №4, №5) ───

/// Задание №1: файл слишком большой.
void handleFileTooLarge(const FileTooLargeException& e, ResultsStorage& results);

/// Задание №3: файл был удалён из указанного каталога.
void handleFileDeleted(const FileDeletedException& e, ResultsStorage& results);

/// Задание №5: файл не соответствует указанному формату.
void handleFormatMismatch(const FormatMismatchException& e, ResultsStorage& results);

/**
 * @brief Обрабатывает один файл функциями Лабораторной №2/№3.
 *
 * Перед обработкой проверяются:
 *   - существование файла (иначе FileDeletedException);
 *   - формат (иначе FormatMismatchException);
 *   - размер (иначе FileTooLargeException).
 * Любое исключение перехватывается, заносится в журнал и превращается
 * в соответствующую отметку в ResultsStorage.
 *
 * @return true, если файл обработан успешно.
 */
bool processOneFile(const std::string& inputPath, ResultsStorage& results);