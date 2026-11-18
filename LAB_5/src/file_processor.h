#pragma once

#include "file_result.h"

#include <string>

/// Путь к файлу с полным результатом обработки данного входного файла.
std::string resultsPathFor(const std::string& inputPath);

/// Проверяет, что каталог существует и доступен для чтения.
/// Бросает DirectoryAccessException.
void ensureDirectoryReadable(const std::string& directory);

/// Проверяет файл перед обработкой:
///  - существует (иначе FileRemovedException);
///  - размер <= 24 МБ (иначе FileTooLargeException);
///  - расширение .md (иначе UnsupportedFileFormatException);
///  - доступен для чтения.
void ensureFileReadyForProcessing(const std::string& path);

/**
 * @brief Обрабатывает один файл функциями Лабораторной №2/№3.
 *
 * Все исключения перехватываются, логируются через Logger
 * и превращаются в markError().
 *
 * @return true, если файл обработан успешно.
 */
bool processOneFile(const std::string& inputPath,
                    ResultsStorage& results);