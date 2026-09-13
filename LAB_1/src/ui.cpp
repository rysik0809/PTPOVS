#include "ui.hpp"

#include "file_io.hpp"
#include "word_frequency.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <cctype>
#include <stdexcept>

namespace ui {

namespace {

std::string askLine(const std::string& prompt, const std::string& def = "")
{
    while (true)
    {
        std::cout << prompt;
        if (!def.empty()) std::cout << " [" << def << "]";
        std::cout << ": ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            return def;
        }
        if (line.empty() && !def.empty()) return def;
        if (line.empty()) {
            std::cout << "Пустой ввод, попробуйте снова.\n";
            continue;
        }
        return line;
    }
}

bool askYesNo(const std::string& prompt, bool def)
{
    while (true)
    {
        std::cout << prompt << (def ? " [Y/n]: " : " [y/N]: ");

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            return def;
        }
        if (line.empty()) return def;

        const char c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(line[0])));

        if (c == 'y' || c == 'д') return true;
        if (c == 'n' || c == 'н') return false;

        std::cout << "Пожалуйста, ответьте \"y\" или \"n\".\n";
    }
}

// Пишет результат в поток в формате "слово - счётчик".
void writeResult(std::ostream& os,
                 const std::string& path,
                 const std::string& modeName,
                 const wordFrequency& wf)
{
    os << "Файл             : " << path << '\n';
    os << "Режим            : " << modeName << '\n';
    os << "Всего слов       : " << wf.totalCount()  << '\n';
    os << "Уникальных слов  : " << wf.uniqueCount() << '\n';
    os << '\n';
    os << "слово - счётчик\n";
    os << "----------------\n";

    for (const auto& e : wf.entries()) {
        os << e.word << " - " << e.count << '\n';
    }
}

} // namespace

int run()
{
    try
    {
        std::cout << "=== Подсчёт уникальных слов в файле ===\n\n";

        // 1) Входной файл
        const std::string inPath = askLine("Путь к файлу", "test.txt");
        fileBuffer file(inPath);

        std::cout << "Загружено " << file.size() << " байт.\n\n";

        // 2) Опции
        const bool ignoreCase = askYesNo(
            "Игнорировать регистр (регистронезависимо)?", true);

        const unsigned options = ignoreCase
            ? wordFrequency::ignoreCase
            : wordFrequency::none;

        const std::string modeName = ignoreCase
            ? "без учёта регистра"
            : "с учётом регистра";

        // 3) Подсчёт
        wordFrequency wf;
        wf.process(file.view(), options);

        // 4) Куда писать результат
        const std::string outPath = askLine(
            "Путь к файлу результата", "result.txt");

        {
            std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
            if (!out) {
                throw std::runtime_error(
                    "Не удалось открыть на запись: " + outPath);
            }

            writeResult(out, inPath, modeName, wf);

            if (!out) {
                throw std::runtime_error(
                    "Ошибка записи в файл: " + outPath);
            }
        }

        std::cout << "\nРезультат записан в: " << outPath << '\n';

        // 5) По желанию — продублировать на экран
        if (askYesNo("Показать результат также на экране?", false)) {
            std::cout << "\n--- Результат ---\n";
            writeResult(std::cout, inPath, modeName, wf);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Исключение: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

} // namespace ui