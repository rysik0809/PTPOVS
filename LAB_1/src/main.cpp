#include "text_utils.hpp"
#include "word_stats.hpp"
#include "algo_tasks.hpp"

#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace 
{

    constexpr std::size_t kMaxIndexFileSize = 1024ull * 1024 * 1024; // 1 ГБ

    /**
     * @brief Замеряет время работы вызываемого объекта.
     *
     * @tparam Fn Тип callable. Должен вызываться без аргументов.
     * @param fn  Что выполнить.
     * @return Время в миллисекундах.
     */
    template <class Fn>
    double timeMs(Fn&& fn)
    {
        const auto start = std::chrono::steady_clock::now();
        std::forward<Fn>(fn)();
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    /// Печатает время работы алгоритма в миллисекундах.
    void printTime(const char* label, double ms)
    {
        std::cout << "Время работы [" << label << "]: "
                  << std::fixed << std::setprecision(3) << ms << " мс\n";
    }

    std::string askLine(const std::string& prompt, const std::string& def = "")
    {
        while (true)
        {
            std::cout << prompt;
            if (!def.empty()) std::cout << " [" << def << "]";
            std::cout << ": ";

            std::string line;
            if (!std::getline(std::cin, line))
            { 
                std::cout << '\n'; return def;
            }
            if (line.empty() && !def.empty()) return def;

            if (line.empty())
            {  
                std::cout << "Пустой ввод, попробуйте снова.\n"; continue; 
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
            if (!std::getline(std::cin, line))
            { 
                std::cout << '\n'; return def;
            }
            if (line.empty()) return def;

            const char c = static_cast<char>(
                std::tolower(static_cast<unsigned char>(line[0])));

            if (c == 'y') return true;
            if (c == 'n') return false;

            std::cout << "Ответьте \"y\" или \"n\".\n";
        }
    }

    int askInt(const std::string& prompt, int def)
    {
        while (true)
        {
            std::cout << prompt << " [" << def << "]: ";
            std::string line;
            if (!std::getline(std::cin, line))
            { 
                std::cout << '\n'; return def;
            }
            if (line.empty()) return def;

            try 
            {
                std::size_t pos = 0;
                const int v = std::stoi(line, &pos);
                if (pos != line.size()) throw std::invalid_argument("tail");
                return v;
            } catch (...) 
            {
                std::cout << "Некорректное число, попробуйте снова.\n";
            }
        }
    }

    std::vector<int> askIntVector()
    {
        while (true)
        {
            std::cout << "Введите целые числа через пробел: ";
            std::string line;
            if (!std::getline(std::cin, line)) return {};

            if (line.empty())
            { 
                std::cout << "Пустой ввод.\n"; continue; 
            }

            std::istringstream is(line);
            std::vector<int> v;
            int x;
            while (is >> x) v.push_back(x);

            if (!is.eof())
            { 
                std::cout << "Некорректный ввод.\n"; continue; 
            }
            if (v.empty())
            { 
                std::cout << "Не введено ни одного числа.\n"; continue; 
            }
            return v;
        }
    }

    void printIntVector(std::ostream& os, const std::vector<int>& v)
    {
        os << '[';
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i) os << ", ";
            os << v[i];
        }
        os << ']';
    }

    void writeFrequencies(std::ostream& os, const wordFrequency& wf)
    {
        os << "Всего слов       : " << wf.totalCount()  << '\n';
        os << "Уникальных слов  : " << wf.uniqueCount() << '\n';
        os << '\n';
        for (const auto& e : wf.entries())
        {
            os << e.word << " - " << e.count << '\n';
        }          
    }

    void writeIndex(std::ostream& os, const wordIndex& wi)
    {
        os << "Уникальных слов  : " << wi.uniqueCount() << '\n';
        os << '\n';
        for (const auto& e : wi.entries())
        {
            os << e.word << " - ";
            for (std::size_t i = 0; i < e.positions.size(); ++i)
            {
                if (i) os << ", ";
                os << e.positions[i];
            }
            os << '\n';
        }
    }

    void runFrequency()
    {
        std::cout << "\n=== Подсчёт уникальных слов ===\n";

        const std::string inPath = askLine("Путь к файлу", "test.txt");
        fileBuffer file(inPath);
        std::cout << "Загружено " << file.size() << " байт.\n";

        const bool ignore = askYesNo("Игнорировать регистр?", true);
        const unsigned options = ignore ? wordFrequency::ignoreCase
                                        : wordFrequency::none;

        const std::string outPath = askLine("Путь к файлу результата",
                                            "result_freq.txt");

        const std::size_t threads = parallel::hardwareThreads();
        std::cout << "Потоков: " << threads << '\n';

        wordFrequency wf;
        const double ms = timeMs([&] {
            wf.process(file.view(), options, threads);
        });
        printTime("Задание 1 (подсчёт частот)", ms);

        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw std::runtime_error("Не удалось открыть: " + outPath);
        }
        writeFrequencies(out, wf);
        if (!out)
        {
            throw std::runtime_error("Ошибка записи: " + outPath);
        }
        std::cout << "Результат записан в " << outPath << '\n';
    }

    void runIndex()
    {
        std::cout << "\n=== Индексация позиций слов ===\n";

        const std::string inPath = askLine("Путь к файлу", "test.txt");
        fileBuffer file(inPath);
        std::cout << "Загружено " << file.size() << " байт.\n";

        if (file.size() > kMaxIndexFileSize)
        {
            std::ostringstream msg;
            msg << "Файл слишком большой для индексации позиций: "
                << file.size() << " байт ("
                << (file.size() >> 20) << " МБ). "
                << "Максимум — " << (kMaxIndexFileSize >> 20)
                << " МБ. Используйте задачу 1 (подсчёт частот) — "
                << "она умеет работать чанками.";
            throw std::runtime_error(msg.str());
        }

        const bool ignore = askYesNo("Игнорировать регистр?", true);
        const unsigned options = ignore ? wordIndex::ignoreCase
                                        : wordIndex::none;

        const std::size_t threads = parallel::hardwareThreads();
        std::cout << "Потоков: " << threads << '\n';

        wordIndex wi;

        const double ms = timeMs([&] {
            try
            {
                wi.process(file.view(), options, threads);
            }
            catch (const std::bad_alloc&)
            {
                throw std::runtime_error(
                    "Не хватило памяти для построения индекса. "
                    "Файл слишком большой для задачи 2 — "
                    "используйте задачу 1 (подсчёт частот).");
            }
        });
        printTime("Задание 2 (индексация)", ms);

        const std::string outPath = askLine("Путь к файлу результата",
                                            "result_index.txt");
        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw std::runtime_error("Не удалось открыть: " + outPath);
        }
        writeIndex(out, wi);
        if (!out)
        {
            throw std::runtime_error("Ошибка записи: " + outPath);
        }
        std::cout << "Результат записан в " + outPath << '\n';
    }

    void runStlTasks()
    {
        std::cout << "\n=== STL-задачи над vector<int> ===\n";

        const std::vector<int> source = askIntVector();

        std::cout << "Исходный вектор      : ";
        printIntVector(std::cout, source);
        std::cout << '\n';

        double msSquare = 0.0;
        std::vector<int> squared;
        msSquare = timeMs([&] {
            squared = algo_tasks::squarePrimes(source);
        });

        std::cout << "Простые в квадрат    : ";
        printIntVector(std::cout, squared);
        std::cout << '\n';

        std::vector<int> sorted = source;
        const double msSort = timeMs([&] {
            algo_tasks::sortOddAscEvenDesc(sorted);
        });

        std::cout << "Нечёт/чёт сортировка : ";
        printIntVector(std::cout, sorted);
        std::cout << '\n';

        const int lo = askInt("Нижняя граница диапазона", 0);
        const int hi = askInt("Верхняя граница диапазона", 10);

        std::vector<int> inRange;
        const double msRange = timeMs([&] {
            inRange = algo_tasks::uniqueInRange(source, lo, hi);
        });

        std::cout << "Уникальные в [" << lo << ", " << hi << "]: ";
        printIntVector(std::cout, inRange);
        std::cout << '\n';

        printTime("3a (простые в квадрат)",   msSquare);
        printTime("3b (нечёт/чёт сортировка)", msSort);
        printTime("3c (уникальные в диапазоне)", msRange);
        printTime("Задание 3 (всего)",          msSquare + msSort + msRange);

        if (askYesNo("Записать результат в файл?", false))
        {
            const std::string outPath = askLine("Путь к файлу результата",
                                                "result_stl.txt");
            std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
            if (!out)
            {
                throw std::runtime_error("Не удалось открыть: " + outPath);
            }
            out << "Исходный вектор      : "; printIntVector(out, source);  out << '\n';
            out << "Простые в квадрат    : "; printIntVector(out, squared); out << '\n';
            out << "Нечёт/чёт сортировка : "; printIntVector(out, sorted);  out << '\n';
            out << "Уникальные в [" << lo << ", " << hi << "]: ";
            printIntVector(out, inRange);
            out << '\n';

            if (!out)
            {
                throw std::runtime_error("Ошибка записи: " + outPath);
            }
            std::cout << "Результат записан в " + outPath << '\n';
        }
    }

} // namespace

int main()
{
    while (true)
    {
        std::cout << "\n=== Меню ===\n";
        std::cout << "1. Подсчёт уникальных слов\n";
        std::cout << "2. Индексация позиций слов\n";
        std::cout << "3. STL-задачи над vector<int>\n";
        std::cout << "0. Выход\n";

        const std::string choice = askLine("Выбор", "0");

        try
        {
            if (choice == "0") return 0;
            if (choice == "1") { runFrequency(); continue; }
            if (choice == "2") { runIndex();     continue; }
            if (choice == "3") { runStlTasks();  continue; }
            std::cout << "Неизвестный пункт меню.\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "Исключение: " << e.what() << '\n';
        }
    }
    return 0;
}