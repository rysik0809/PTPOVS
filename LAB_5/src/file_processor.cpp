#include "file_processor.h"

#include "exceptions.h"
#include "logger.h"
#include "markdown_analyzer.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>

namespace {

std::string lowerExtension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return ext;
}

} // namespace

std::string resultsPathFor(const std::string& inputPath) {
    std::filesystem::path p(inputPath);
    const std::string base = p.filename().string();
    const std::size_t h = std::hash<std::string>{}(inputPath);
    return (std::filesystem::path("./results") /
            (base + "_" + std::to_string(h) + ".txt")).string();
}

void ensureDirectoryReadable(const std::string& directory) {
    std::error_code ec;

    if (!std::filesystem::exists(directory, ec) || ec) {
        throw DirectoryAccessException(
            directory, "каталог не существует или недоступен");
    }

    if (!std::filesystem::is_directory(directory, ec) || ec) {
        throw DirectoryAccessException(
            directory, "путь не является каталогом");
    }

    std::filesystem::directory_iterator test(directory, ec);
    if (ec) {
        throw DirectoryAccessException(directory, ec.message());
    }
}

void ensureFileReadyForProcessing(const std::string& path) {
    std::error_code ec;

    if (!std::filesystem::exists(path, ec) || ec) {
        throw FileRemovedException(path);
    }

    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        throw FileRemovedException(path);
    }

    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    if (ec) {
        throw Lab5Exception("Не удалось получить размер файла " + path +
                            ": " + ec.message());
    }

    if (size > FileTooLargeException::MaxFileSize) {
        throw FileTooLargeException(path, size);
    }

    if (lowerExtension(path) != ".md") {
        throw UnsupportedFileFormatException(
            path, "ожидается расширение .md");
    }

    std::ifstream test(path, std::ios::binary);
    if (!test.is_open()) {
        throw Lab5Exception("Нет прав на чтение файла: " + path);
    }
}

bool processOneFile(const std::string& inputPath,
                    ResultsStorage& results) {
    results.markProcessing(inputPath);

    try {
        ensureFileReadyForProcessing(inputPath);

        std::error_code ec;
        std::filesystem::create_directories("./results", ec);

        const std::string outPath = resultsPathFor(inputPath);
        std::ofstream out(outPath);
        if (!out.is_open()) {
            throw Lab5Exception(
                "Не удалось открыть файл результатов: " + outPath);
        }

        const AnalysisStats stats = analyzeMarkdownFile(inputPath, out);
        out.close();

        if (stats.totalLines > 0 &&
            stats.foreignLines * 100 / stats.totalLines >= 50) {
            throw UnsupportedFileFormatException(
                inputPath,
                "более половины строк содержат чужую разметку");
        }

        results.markDone(inputPath, stats, outPath);
        return true;
    } catch (const Lab5Exception& e) {
        Logger::instance().logException(e, "processOneFile");
        results.markError(inputPath, e.what());
        return false;
    } catch (const std::exception& e) {
        Logger::instance().logException(e, "processOneFile");
        results.markError(inputPath, e.what());
        return false;
    }
}