#pragma once

#include <cstddef>
#include <ostream>
#include <string>

// ======================= СТАТИСТИКА АНАЛИЗА =================================

struct AnalysisStats {
    std::size_t totalLines    = 0;
    std::size_t headings      = 0;
    std::size_t paragraphs    = 0;
    std::size_t listItems     = 0;
    std::size_t foreignLines  = 0;
};

// ======================= ПРОВЕРКА ЯЗЫКА MARKDOWN ============================

bool isForeignMarkup(const std::string& line);
bool isValidMarkdownLine(const std::string& line);

// ======================= АНАЛИЗ ФАЙЛА (ПОТОКОВЫЙ) ===========================
//
// Читает файл блоками (по умолчанию 1 МБ), находит заголовки, абзацы и
// элементы списков и СРАЗУ пишет их в out. Память не зависит от размера
// файла — подходит для 6 ГБ и более.
//
// Предупреждения о чужой разметке выводятся в std::cerr (первые 100 строк).
// В out записываются только найденные элементы и итоговая сводка.

AnalysisStats analyzeMarkdownFile(const std::string& filename,
                                  std::ostream& out,
                                  std::size_t blockSize = 1024 * 1024);