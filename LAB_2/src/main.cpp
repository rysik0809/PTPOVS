#include "markdown_analyzer.h"
#include "person.h"
#include "random_generator.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <locale>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace {

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n\"'";
    std::size_t first = s.find_first_not_of(ws);
    if (first == std::string::npos)
        return "";
    std::size_t last = s.find_last_not_of(ws);
    return s.substr(first, last - first + 1);
}

} // namespace

int main() {
    std::locale::global(std::locale(""));

    std::cout << "=== Лабораторная работа №2. Вариант 2: Markdown ===\n\n";

    // ---------- 1. Путь к входному файлу ----------
    std::cout << "Введите путь к Markdown-файлу для анализа\n"
                 "(например, example.md; Enter — example.md): ";
    std::string inputPath;
    std::getline(std::cin, inputPath);
    inputPath = trim(inputPath);
    if (inputPath.empty())
        inputPath = "example.md";

    std::error_code ec;
    if (!fs::exists(inputPath, ec)) {
        std::cerr << "Ошибка: файл не найден: " << inputPath << "\n";
        return 1;
    }

    // ---------- 2. Путь для сохранения файлов ----------
    std::cout << "\nВведите путь к папке, куда будут сохранены результаты\n"
                 "(results.txt и anketa.txt; Enter — текущая папка): ";
    std::string outputDir;
    std::getline(std::cin, outputDir);
    outputDir = trim(outputDir);
    if (outputDir.empty())
        outputDir = ".";

    fs::create_directories(outputDir, ec);
    if (ec) {
        std::cerr << "Не удалось создать папку " << outputDir
                  << ": " << ec.message() << "\n";
        return 1;
    }

    fs::path resultsPath  = fs::path(outputDir) / "results.txt";
    fs::path anketaPath   = fs::path(outputDir) / "anketa.txt";

    std::cout << "\nВходной файл:    " << inputPath           << "\n";
    std::cout << "Файл результатов: " << resultsPath.string() << "\n";
    std::cout << "Файл анкеты:      " << anketaPath.string()  << "\n\n";

    // ---------- 3. Анализ Markdown + запись results.txt ----------
    AnalysisStats stats;
    {
        std::ofstream resultsOut(resultsPath);
        if (!resultsOut.is_open()) {
            std::cerr << "Не удалось открыть файл для записи: "
                      << resultsPath << "\n";
            return 1;
        }
        stats = analyzeMarkdownFile(inputPath, resultsOut, 1024 * 1024);
        resultsOut.close();
    }

    std::cout << "\nАнализ завершён.\n";
    std::cout << "  Строк:              " << stats.totalLines   << "\n";
    std::cout << "  Заголовков:         " << stats.headings     << "\n";
    std::cout << "  Абзацев:            " << stats.paragraphs   << "\n";
    std::cout << "  Элементов списков:  " << stats.listItems    << "\n";
    std::cout << "  Строк с чужой разметкой: " << stats.foreignLines << "\n";
    std::cout << "Результат записан в " << resultsPath.string() << "\n";

    // ---------- 4. Анкета ----------
    std::cout << "\n=== Проверка анкеты ===\n";
    std::cout << "1. Ввод вручную\n";
    std::cout << "2. Генерация случайных данных\n";
    std::cout << "Выберите режим (1/2): ";
    int mode = 1;
    std::cin >> mode;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    Person raw;
    if (mode == 1) {
        raw = inputPerson();
    } else {
        RandomPersonGenerator gen;
        raw = gen.generate();
        std::cout << "Сгенерированные данные (до нормализации):\n";
        printPerson(raw);
    }

    Person normalized = raw;
    bool valid = validateAndNormalize(normalized);

    // ---------- 5. Сохранение anketa.txt ----------
    {
        std::ofstream anketaOut(anketaPath);
        if (!anketaOut.is_open()) {
            std::cerr << "Не удалось открыть файл для записи: "
                      << anketaPath << "\n";
            return 1;
        }
        writePerson(anketaOut, raw, normalized, valid);
        anketaOut.close();
    }

    std::cout << "\nНормализованные данные:\n";
    printPerson(normalized);
    std::cout << (valid ? "Статус: OK\n" : "Статус: ОШИБКА ВАЛИДАЦИИ\n");
    std::cout << "Анкета записана в " << anketaPath.string() << "\n";

    return 0;
}