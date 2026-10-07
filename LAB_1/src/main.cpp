#include "i18n.hpp"
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
#include <string_view>
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
    void printTime(std::string_view label, double ms)
    {
        std::cout << i18n::tr("time.label") << label << i18n::tr("time.labelEnd")
                  << std::fixed << std::setprecision(3) << ms
                  << i18n::tr("time.unit") << '\n';
    }

    std::string askLine(std::string_view prompt, std::string_view def = "")
    {
        while (true)
        {
            std::cout << prompt;
            if (!def.empty()) std::cout << " [" << def << "]";
            std::cout << ": ";

            std::string line;
            if (!std::getline(std::cin, line))
            {
                std::cout << '\n';
                return std::string(def);
            }
            if (!line.empty()) return line;
            if (!def.empty())  return std::string(def);

            std::cout << i18n::tr("common.emptyInput") << '\n';
        }
    }

    bool askYesNo(std::string_view prompt, bool def)
    {
        while (true)
        {
            std::cout << prompt << (def ? " [Y/n]: " : " [y/N]: ");

            std::string line;
            if (!std::getline(std::cin, line))
            {
                std::cout << '\n';
                return def;
            }
            if (line.empty()) return def;

            const char c = static_cast<char>(
                std::tolower(static_cast<unsigned char>(line[0])));

            if (c == 'y') return true;
            if (c == 'n') return false;

            std::cout << i18n::tr("common.yesNo") << '\n';
        }
    }

    int askInt(std::string_view prompt, int def)
    {
        while (true)
        {
            std::cout << prompt << " [" << def << "]: ";
            std::string line;
            if (!std::getline(std::cin, line))
            {
                std::cout << '\n';
                return def;
            }
            if (line.empty()) return def;

            try
            {
                std::size_t pos = 0;
                const int v = std::stoi(line, &pos);
                if (pos != line.size()) throw std::invalid_argument("tail");
                return v;
            }
            catch (...)
            {
                std::cout << i18n::tr("common.badNumber") << '\n';
            }
        }
    }

    std::vector<int> askIntVector()
    {
        while (true)
        {
            std::cout << i18n::tr("stl.promptVector");
            std::string line;
            if (!std::getline(std::cin, line)) return {};

            if (line.empty())
            {
                std::cout << i18n::tr("stl.emptyInput") << '\n';
                continue;
            }

            std::istringstream is(line);
            std::vector<int> v;
            int x;
            while (is >> x) v.push_back(x);

            if (!is.eof())
            {
                std::cout << i18n::tr("stl.invalidInput") << '\n';
                continue;
            }
            if (v.empty())
            {
                std::cout << i18n::tr("stl.noNumbers") << '\n';
                continue;
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
        os << i18n::tr("freq.headerTotal")  << wf.totalCount()  << '\n';
        os << i18n::tr("freq.headerUnique") << wf.uniqueCount() << '\n';
        os << '\n';
        for (const auto& e : wf.entries())
        {
            os << e.word << " - " << e.count << '\n';
        }
    }

    void writeIndex(std::ostream& os, const wordIndex& wi)
    {
        os << i18n::tr("index.headerUnique") << wi.uniqueCount() << '\n';
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
        std::cout << '\n' << i18n::tr("freq.title") << '\n';

        const std::string inPath = askLine(i18n::tr("freq.promptFile"), "test.txt");
        fileBuffer file(inPath);
        std::cout << i18n::tr("common.loaded") << file.size()
                  << i18n::tr("common.bytes") << '\n';

        const bool ignore = askYesNo(i18n::tr("freq.promptIgnoreCase"), true);
        const unsigned options = ignore ? wordFrequency::ignoreCase
                                        : wordFrequency::none;

        const std::string outPath =
            askLine(i18n::tr("freq.promptOutput"), "result_freq.txt");

        const std::size_t threads = parallel::hardwareThreads();
        std::cout << i18n::tr("common.threads") << threads << '\n';

        wordFrequency wf;
        const double ms = timeMs([&] {
            wf.process(file.view(), options, threads);
        });
        printTime(i18n::tr("freq.label"), ms);

        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw std::runtime_error(i18n::tr("common.openError") + outPath);
        }
        writeFrequencies(out, wf);
        if (!out)
        {
            throw std::runtime_error(i18n::tr("common.writeError") + outPath);
        }
        std::cout << i18n::tr("common.resultWritten") << outPath << '\n';
    }

    void runIndex()
    {
        std::cout << '\n' << i18n::tr("index.title") << '\n';

        const std::string inPath = askLine(i18n::tr("freq.promptFile"), "test.txt");
        fileBuffer file(inPath);
        std::cout << i18n::tr("common.loaded") << file.size()
                  << i18n::tr("common.bytes") << '\n';

        if (file.size() > kMaxIndexFileSize)
        {
            std::ostringstream msg;
            msg << i18n::tr("index.tooLarge")
                << file.size() << " ("
                << (file.size() >> 20) << " MB). "
                << i18n::tr("index.maxSize")
                << (kMaxIndexFileSize >> 20)
                << i18n::tr("index.useTask1");
            throw std::runtime_error(msg.str());
        }

        const bool ignore = askYesNo(i18n::tr("freq.promptIgnoreCase"), true);
        const unsigned options = ignore ? wordIndex::ignoreCase
                                        : wordIndex::none;

        const std::size_t threads = parallel::hardwareThreads();
        std::cout << i18n::tr("common.threads") << threads << '\n';

        wordIndex wi;

        const double ms = timeMs([&] {
            try
            {
                wi.process(file.view(), options, threads);
            }
            catch (const std::bad_alloc&)
            {
                throw std::runtime_error(i18n::tr("index.noMemory"));
            }
        });
        printTime(i18n::tr("index.label"), ms);

        const std::string outPath =
            askLine(i18n::tr("freq.promptOutput"), "result_index.txt");
        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw std::runtime_error(i18n::tr("common.openError") + outPath);
        }
        writeIndex(out, wi);
        if (!out)
        {
            throw std::runtime_error(i18n::tr("common.writeError") + outPath);
        }
        std::cout << i18n::tr("common.resultWritten") << outPath << '\n';
    }

    void runStlTasks()
    {
        std::cout << '\n' << i18n::tr("stl.title") << '\n';

        const std::vector<int> source = askIntVector();

        std::cout << i18n::tr("stl.source");
        printIntVector(std::cout, source);
        std::cout << '\n';

        double msSquare = 0.0;
        std::vector<int> squared;
        msSquare = timeMs([&] {
            squared = algo_tasks::squarePrimes(source);
        });

        std::cout << i18n::tr("stl.squared");
        printIntVector(std::cout, squared);
        std::cout << '\n';

        std::vector<int> sorted = source;
        const double msSort = timeMs([&] {
            algo_tasks::sortOddAscEvenDesc(sorted);
        });

        std::cout << i18n::tr("stl.sorted");
        printIntVector(std::cout, sorted);
        std::cout << '\n';

        const int lo = askInt(i18n::tr("stl.promptLo"), 0);
        const int hi = askInt(i18n::tr("stl.promptHi"), 10);

        std::vector<int> inRange;
        const double msRange = timeMs([&] {
            inRange = algo_tasks::uniqueInRange(source, lo, hi);
        });

        std::cout << i18n::tr("stl.uniquePrefix") << lo << ", " << hi
                  << i18n::tr("stl.uniqueSuffix");
        printIntVector(std::cout, inRange);
        std::cout << '\n';

        printTime(i18n::tr("stl.label3a"), msSquare);
        printTime(i18n::tr("stl.label3b"), msSort);
        printTime(i18n::tr("stl.label3c"), msRange);
        printTime(i18n::tr("stl.label3Total"), msSquare + msSort + msRange);

        if (askYesNo(i18n::tr("stl.promptWriteFile"), false))
        {
            const std::string outPath =
                askLine(i18n::tr("stl.promptOutput"), "result_stl.txt");
            std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
            if (!out)
            {
                throw std::runtime_error(i18n::tr("common.openError") + outPath);
            }

            out << i18n::tr("stl.source");
            printIntVector(out, source);
            out << '\n';

            out << i18n::tr("stl.squared");
            printIntVector(out, squared);
            out << '\n';

            out << i18n::tr("stl.sorted");
            printIntVector(out, sorted);
            out << '\n';

            out << i18n::tr("stl.uniquePrefix") << lo << ", " << hi
                << i18n::tr("stl.uniqueSuffix");
            printIntVector(out, inRange);
            out << '\n';

            if (!out)
            {
                throw std::runtime_error(i18n::tr("common.writeError") + outPath);
            }
            std::cout << i18n::tr("common.resultWritten") << outPath << '\n';
        }
    }

} // namespace

int main()
{
    i18n::init();

    std::cout << i18n::tr("common.language")
              << i18n::currentLanguage() << '\n';

    while (true)
    {
        std::cout << '\n' << i18n::tr("menu.title") << '\n';
        std::cout << i18n::tr("menu.freq")  << '\n';
        std::cout << i18n::tr("menu.index") << '\n';
        std::cout << i18n::tr("menu.stl")   << '\n';
        std::cout << i18n::tr("menu.exit")  << '\n';

        const std::string choice = askLine(i18n::tr("menu.prompt"), "0");

        try
        {
            if (choice == "0") return 0;
            if (choice == "1") { runFrequency(); continue; }
            if (choice == "2") { runIndex();     continue; }
            if (choice == "3") { runStlTasks();  continue; }
            std::cout << i18n::tr("menu.unknown") << '\n';
        }
        catch (const std::exception& e)
        {
            std::cerr << i18n::tr("common.exception") << e.what() << '\n';
        }
    }
    return 0;
}