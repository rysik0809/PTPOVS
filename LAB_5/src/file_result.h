#pragma once

#include "markdown_analyzer.h"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

enum class FileStatus {
    Queued,       ///< Файл стоит в очереди.
    Processing,   ///< Сейчас обрабатывается рабочим потоком.
    Done,         ///< Обработан успешно.
    Cancelled,    ///< Отменён.
    Error         ///< Ошибка обработки (в т.ч. исключительная ситуация).
};

std::string statusToString(FileStatus s);

struct FileResult {
    std::string path;
    FileStatus  status = FileStatus::Queued;
    AnalysisStats stats;
    std::string error;
    std::string resultsFile;
    std::chrono::system_clock::time_point updatedAt =
        std::chrono::system_clock::now();
};

class ResultsStorage {
public:
    void markQueued    (const std::string& path);
    void markProcessing(const std::string& path);
    void markDone      (const std::string& path,
                        const AnalysisStats& stats,
                        const std::string& resultsFile);
    void markError     (const std::string& path,
                        const std::string& error);
    void markCancelled (const std::string& path);
    void markAllQueuedAsCancelled();

    std::optional<FileResult>     get     (const std::string& path) const;
    std::vector<FileResult>       snapshot() const;
    std::size_t                   size    () const;
    void                          clear   ();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, FileResult> results_;
};