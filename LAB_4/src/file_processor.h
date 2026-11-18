#pragma once

#include "file_result.h"
#include <string>

/// Возвращает путь к файлу с полным результатом обработки данного файла.
std::string resultsPathFor(const std::string& inputPath);

/**
 * @brief Обрабатывает один файл функциями Лабораторной №2/№3
 *        и записывает статус в ResultsStorage.
 *
 * Все исключения перехватываются внутри и превращаются в markError().
 * @return true, если файл обработан успешно.
 */
bool processOneFile(const std::string& inputPath, ResultsStorage& results);