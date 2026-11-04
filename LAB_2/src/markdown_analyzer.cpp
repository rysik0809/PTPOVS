#include "markdown_analyzer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

namespace {

// Сбросить накопленный абзац в поток и обнулить состояние.
void flushParagraph(std::ostream& out,
                    AnalysisStats& stats,
                    bool& inParagraph,
                    std::string& buffer) {
    if (!inParagraph)
        return;
    out << "[P] ";
    for (char c : buffer)
        out << (c == '\n' ? ' ' : c);   // многострочный абзац — в одну строку
    out << "\n";
    ++stats.paragraphs;
    buffer.clear();
    inParagraph = false;
}

} // namespace

// ======================= ПРОВЕРКА ЯЗЫКА MARKDOWN ============================

bool isForeignMarkup(const std::string& line) {
    std::size_t i = 0;
    while (i < line.size() &&
           std::isspace(static_cast<unsigned char>(line[i])))
        ++i;
    if (i >= line.size())
        return false;

    char c = line[i];

    if (c == '<') {
        static const std::regex htmlTag(R"(<\s*/?\s*[a-zA-Z][^>]*>)");
        return std::regex_search(line, htmlTag);
    }

    if (c == '=') {
        static const std::regex asciidocHeading(R"(^\s*={1,6}\s+\S)");
        static const std::regex asciidocDelimiter(R"(^\s*={4,}\s*$)");
        return std::regex_search(line, asciidocHeading) ||
               std::regex_search(line, asciidocDelimiter);
    }

    if (c == '-' || c == '.') {
        static const std::regex asciidocDelimiter(R"(^\s*(-{4,}|\.{4,})\s*$)");
        return std::regex_search(line, asciidocDelimiter);
    }

    return false;
}

bool isValidMarkdownLine(const std::string& line) {
    if (line.empty() ||
        std::all_of(line.begin(), line.end(),
                    [](unsigned char ch) { return std::isspace(ch); }))
        return true;
    return !isForeignMarkup(line);
}

// ======================= ОБРАБОТКА ОДНОЙ СТРОКИ =============================

void processLine(const std::string& line,
                 std::ostream& out,
                 AnalysisStats& stats,
                 bool& inParagraph,
                 std::string& paragraphBuffer) {
    std::size_t i = 0;
    while (i < line.size() &&
           std::isspace(static_cast<unsigned char>(line[i])))
        ++i;

    bool mightBeHeading = false;
    bool mightBeList    = false;

    if (i < line.size()) {
        char c = line[i];
        if (c == '#') {
            mightBeHeading = true;
        } else if (c == '*' || c == '-' || c == '+') {
            mightBeList = true;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            mightBeList = true;
        }
    }

    std::smatch match;

    // ---------- Заголовки ----------
    if (mightBeHeading) {
        static const std::regex headingRe(R"(^\s*(#{1,6})\s+(.+?)\s*$)");
        if (std::regex_match(line, match, headingRe)) {
            flushParagraph(out, stats, inParagraph, paragraphBuffer);
            int level = static_cast<int>(match[1].str().size());
            out << "[H" << level << "] " << match[2].str() << "\n";
            ++stats.headings;
            return;
        }
    }

    // ---------- Списки ----------
    if (mightBeList) {
        static const std::regex unorderedListRe(R"(^\s*[*+-]\s+(.+)$)");
        static const std::regex orderedListRe(R"(^\s*\d+\.\s+(.+)$)");

        if (std::regex_match(line, match, unorderedListRe)) {
            flushParagraph(out, stats, inParagraph, paragraphBuffer);
            out << "[UL] " << match[1].str() << "\n";
            ++stats.listItems;
            return;
        }
        if (std::regex_match(line, match, orderedListRe)) {
            flushParagraph(out, stats, inParagraph, paragraphBuffer);
            out << "[OL] " << match[1].str() << "\n";
            ++stats.listItems;
            return;
        }
    }

    // ---------- Абзацы ----------
    if (line.empty() ||
        std::all_of(line.begin(), line.end(),
                    [](unsigned char ch) { return std::isspace(ch); })) {
        flushParagraph(out, stats, inParagraph, paragraphBuffer);
        return;
    }

    if (!inParagraph) {
        paragraphBuffer = line;
        inParagraph = true;
    } else {
        paragraphBuffer += "\n" + line;
    }
}

// ======================= АНАЛИЗ ФАЙЛА (ПОТОКОВЫЙ) ===========================

AnalysisStats analyzeMarkdownFile(const std::string& filename,
                                  std::ostream& out,
                                  std::size_t blockSize) {
    AnalysisStats stats;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << "\n";
        return stats;
    }
    if (blockSize == 0)
        blockSize = 1024 * 1024;

    out << "=== Результаты анализа Markdown ===\n";
    out << "Файл: " << filename << "\n\n";

    std::vector<char> buffer(blockSize);
    std::string data;
    data.reserve(blockSize + 64 * 1024);

    bool inParagraph = false;
    std::string paragraphBuffer;

    constexpr std::size_t kWarnLimit = 100;
    std::size_t warned = 0;

    while (file) {
        file.read(buffer.data(),
                  static_cast<std::streamsize>(buffer.size()));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0)
            break;

        data.append(buffer.data(), static_cast<std::size_t>(bytesRead));

        std::size_t start = 0;
        std::size_t pos;
        while ((pos = data.find('\n', start)) != std::string::npos) {
            std::string line = data.substr(start, pos - start);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            ++stats.totalLines;
            if (!isValidMarkdownLine(line)) {
                ++stats.foreignLines;
                if (warned < kWarnLimit) {
                    std::cerr << "Предупреждение: строка содержит чужую "
                                 "разметку: " << line << "\n";
                    ++warned;
                    if (warned == kWarnLimit) {
                        std::cerr << "... (дальнейшие предупреждения "
                                     "подавлены)\n";
                    }
                }
            }
            processLine(line, out, stats, inParagraph, paragraphBuffer);
            start = pos + 1;
        }

        if (start > 0)
            data.erase(0, start);
    }

    // Последняя строка без завершающего '\n'
    if (!data.empty()) {
        if (data.back() == '\r')
            data.pop_back();
        ++stats.totalLines;
        if (!isValidMarkdownLine(data)) {
            ++stats.foreignLines;
            if (warned < kWarnLimit) {
                std::cerr << "Предупреждение: строка содержит чужую "
                             "разметку: " << data << "\n";
            }
        }
        processLine(data, out, stats, inParagraph, paragraphBuffer);
    }

    // Закрыть последний абзац
    flushParagraph(out, stats, inParagraph, paragraphBuffer);

    // Итоговая сводка
    out << "\n=== Итоги ===\n";
    out << "Строк обработано:          " << stats.totalLines << "\n";
    out << "Заголовков:                " << stats.headings << "\n";
    out << "Абзацев:                   " << stats.paragraphs << "\n";
    out << "Элементов списков:         " << stats.listItems << "\n";
    out << "Строк с чужой разметкой:   " << stats.foreignLines << "\n";

    if (stats.foreignLines > 0) {
        std::cerr << "\nИтого: " << stats.foreignLines << " из "
                  << stats.totalLines << " строк содержат маркеры чужой "
                     "разметки";
        if (stats.totalLines > 0 &&
            stats.foreignLines * 100 / stats.totalLines >= 50) {
            std::cerr << ".\nОШИБКА: файл, похоже, не является Markdown "
                         "(AsciiDoc или HTML).\n";
        } else {
            std::cerr << ".\n";
        }
    }

    return stats;
}