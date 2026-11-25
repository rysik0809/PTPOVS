#include "file_processor.h"
#include "markdown_analyzer.h"

#include <filesystem>
#include <fstream>
#include <functional>

std::string resultsPathFor(const std::string& inputPath) {
    std::filesystem::path p(inputPath);
    std::string base = p.filename().string();
    std::size_t h    = std::hash<std::string>{}(inputPath);
    return (std::filesystem::path("./results")
            / (base + "_" + std::to_string(h) + ".txt")).string();
}

// ─── Обработчики исключительных ситуаций ────────────────────────────────

void handleFileTooLarge(const FileTooLargeException& e, ResultsStorage& results) {
    logException("FileTooLarge", e);
    results.markError(e.path(), e.what());
}

void handleFileDeleted(const FileDeletedException& e, ResultsStorage& results) {
    logException("FileDeleted", e);
    results.markError(e.path(), e.what());
}

void handleFormatMismatch(const FormatMismatchException& e, ResultsStorage& results) {
    logException("FormatMismatch", e);
    results.markError(e.path(), e.what());
}

// ─── Основная обработка ─────────────────────────────────────────────────

bool processOneFile(const std::string& inputPath, ResultsStorage& results) {
    results.markProcessing(inputPath);

    try {
        // 1. Проверка существования (файл мог быть удалён после постановки в очередь).
        std::error_code ec;
        if (!std::filesystem::exists(inputPath, ec) || ec) {
            throw FileDeletedException(inputPath);
        }

        // 2. Проверка формата.
        if (!isSupportedMarkdownFile(inputPath)) {
            throw FormatMismatchException(inputPath,
                ".md, .markdown, .mdown, .mkd");
        }

        // 3. Проверка размера.
        const auto fileSize = std::filesystem::file_size(inputPath, ec);
        if (ec) {
            throw Lab5Exception("Не удалось получить размер файла '" +
                                inputPath + "': " + ec.message());
        }
        if (fileSize > MAX_FILE_SIZE_BYTES) {
            throw FileTooLargeException(inputPath, fileSize);
        }

        // 4. Собственно обработка.
        std::filesystem::create_directories("./results", ec);
        const std::string outPath = resultsPathFor(inputPath);
        std::ofstream out(outPath);
        if (!out.is_open()) {
            throw Lab5Exception("Не удалось открыть файл результатов: " +
                                outPath);
        }

        AnalysisStats stats = analyzeMarkdownFile(inputPath, out);
        out.close();

        results.markDone(inputPath, stats, outPath);
        return true;
    }
    catch (const FileTooLargeException& e) {
        handleFileTooLarge(e, results);
        return false;
    }
    catch (const FileDeletedException& e) {
        handleFileDeleted(e, results);
        return false;
    }
    catch (const FormatMismatchException& e) {
        handleFormatMismatch(e, results);
        return false;
    }
    catch (const std::exception& e) {
        logException("processOneFile", e);
        results.markError(inputPath, e.what());
        return false;
    }
}