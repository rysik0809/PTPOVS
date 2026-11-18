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

bool processOneFile(const std::string& inputPath, ResultsStorage& results) {
    results.markProcessing(inputPath);
    try {
        std::error_code ec;
        std::filesystem::create_directories("./results", ec);

        const std::string outPath = resultsPathFor(inputPath);
        std::ofstream out(outPath);
        if (!out.is_open()) {
            results.markError(inputPath,
                              "не удалось открыть файл результатов: " + outPath);
            return false;
        }

        AnalysisStats stats = analyzeMarkdownFile(inputPath, out);
        out.close();

        results.markDone(inputPath, stats, outPath);
        return true;
    } catch (const std::exception& e) {
        results.markError(inputPath, e.what());
        return false;
    }
}