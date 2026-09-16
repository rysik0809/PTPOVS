#include "ui.hpp"

#include "file_io.hpp"
#include "word_frequency.hpp"
#include "word_index.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <cctype>
#include <stdexcept>

namespace ui {

namespace {

// ---------- ввод ----------

std::string askLine(const std::string& prompt, const std::string& def = "")
{
    while (true)
    {
        std::cout << prompt;
        if (!def.empty()) std::cout << " [" << def << "]";
        std::cout << ": ";

        std::string line;
        if (!std::getline(std::cin, line)) { std::cout << '\n'; return def; }
        if (line.empty() && !def.empty()) return def;
        if (line.empty()) { std::cout << "Пустой ввод, попробуйте снова.\n"; continue; }
        return line;
    }
}

bool askYesNo(const std::string& prompt, bool def)
{
    while (true)
    {
        std::cout << prompt << (def ? " [Y/n]: " : " [y/N]: ");

        std::string line;
        if (!std::getline(std::cin, line)) { std::cout << '\n'; return def; }
        if (line.empty()) return def;

        const char c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(line[0])));

        if (c == 'y' || c == 'д') return true;
        if (c == 'n' || c == 'н') return false;

        std::cout << "Ответьте \"y\" или \"n\".\n";
    }
}

// ---------- вывод ----------

void writeFrequencies(std::ostream& os, const wordFrequency& wf)
{
    os << "Всего слов       : " << wf.totalCount()  << '\n';
    os << "Уникальных слов  : " << wf.uniqueCount() << '\n';
    os << '\n';
    for (const auto& e : wf.entries()) {
        os << e.word << " - " << e.count << '\n';
    }
}

void writeIndex(std::ostream& os, const wordIndex& wi)
{
    os << "Уникальных слов  : " << wi.uniqueCount() << '\n';
    os << '\n';
    for (const auto& e : wi.entries()) {
        os << e.word << " - ";
        for (std::size_t i = 0; i < e.positions.size(); ++i) {
            if (i) os << ", ";
            os << e.positions[i];
        }
        os << '\n';
    }
}

// ---------- сценарии ----------

void runFrequency()
{
    std::cout << "\n=== Подсчёт уникальных слов ===\n";

    const std::string inPath = askLine("Путь к файлу", "test.txt");
    fileBuffer file(inPath);
    std::cout << "Загружено " << file.size() << " байт.\n";

    const bool ignore = askYesNo("Игнорировать регистр?", true);
    const unsigned options = ignore ? wordFrequency::ignoreCase
                                    : wordFrequency::none;

    wordFrequency wf;
    wf.process(file.view(), options);

    const std::string outPath = askLine("Путь к файлу результата",
                                        "result_freq.txt");
    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Не удалось открыть: " + outPath);

    writeFrequencies(out, wf);
    if (!out) throw std::runtime_error("Ошибка записи: " + outPath);

    std::cout << "Результат записан в " << outPath << '\n';

    if (askYesNo("Показать на экране?", false)) {
        std::cout << "\n--- Результат ---\n";
        writeFrequencies(std::cout, wf);
    }
}

void runIndex()
{
    std::cout << "\n=== Индексация позиций слов ===\n";

    const std::string inPath = askLine("Путь к файлу", "test.txt");
    fileBuffer file(inPath);
    std::cout << "Загружено " << file.size() << " байт.\n";

    const bool ignore = askYesNo("Игнорировать регистр?", true);
    const unsigned options = ignore ? wordIndex::ignoreCase
                                    : wordIndex::none;

    wordIndex wi;
    wi.process(file.view(), options);

    const std::string outPath = askLine("Путь к файлу результата",
                                        "result_index.txt");
    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Не удалось открыть: " + outPath);

    writeIndex(out, wi);
    if (!out) throw std::runtime_error("Ошибка записи: " + outPath);

    std::cout << "Результат записан в " << outPath << '\n';

    if (askYesNo("Показать на экране?", false)) {
        std::cout << "\n--- Результат ---\n";
        writeIndex(std::cout, wi);
    }
}

} // namespace

int run()
{
    while (true)
    {
        std::cout << "\n=== Меню ===\n";
        std::cout << "1. Подсчёт уникальных слов\n";
        std::cout << "2. Индексация позиций слов\n";
        std::cout << "0. Выход\n";

        const std::string choice = askLine("Выбор", "0");

        try
        {
            if (choice == "0") return 0;
            if (choice == "1") { runFrequency(); continue; }
            if (choice == "2") { runIndex();     continue; }
            std::cout << "Неизвестный пункт меню.\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "Исключение: " << e.what() << '\n';
        }
    }
}

} // namespace ui