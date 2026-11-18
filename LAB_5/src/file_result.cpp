#include "file_result.h"

std::string statusToString(FileStatus s) {
    switch (s) {
        case FileStatus::Queued:     return "Queued";
        case FileStatus::Processing: return "Processing";
        case FileStatus::Done:       return "Done";
        case FileStatus::Cancelled:  return "Cancelled";
        case FileStatus::Error:      return "Error";
    }
    return "Unknown";
}

void ResultsStorage::markQueued(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    FileResult r;
    r.path      = path;
    r.status    = FileStatus::Queued;
    r.updatedAt = std::chrono::system_clock::now();
    results_[path] = std::move(r);
}

void ResultsStorage::markProcessing(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& r = results_[path];
    r.path      = path;
    r.status    = FileStatus::Processing;
    r.updatedAt = std::chrono::system_clock::now();
}

void ResultsStorage::markDone(const std::string& path,
                              const AnalysisStats& stats,
                              const std::string& resultsFile) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& r = results_[path];
    r.path        = path;
    r.status      = FileStatus::Done;
    r.stats       = stats;
    r.resultsFile = resultsFile;
    r.error.clear();
    r.updatedAt   = std::chrono::system_clock::now();
}

void ResultsStorage::markError(const std::string& path,
                               const std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& r = results_[path];
    r.path      = path;
    r.status    = FileStatus::Error;
    r.error     = error;
    r.updatedAt = std::chrono::system_clock::now();
}

void ResultsStorage::markCancelled(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& r = results_[path];
    r.path      = path;
    r.status    = FileStatus::Cancelled;
    r.updatedAt = std::chrono::system_clock::now();
}

void ResultsStorage::markAllQueuedAsCancelled() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [k, v] : results_) {
        if (v.status == FileStatus::Queued) {
            v.status    = FileStatus::Cancelled;
            v.updatedAt = std::chrono::system_clock::now();
        }
    }
}

std::optional<FileResult> ResultsStorage::get(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = results_.find(path);
    if (it == results_.end()) return std::nullopt;
    return it->second;
}

std::vector<FileResult> ResultsStorage::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<FileResult> out;
    out.reserve(results_.size());
    for (const auto& [k, v] : results_) out.push_back(v);
    return out;
}

std::size_t ResultsStorage::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return results_.size();
}

void ResultsStorage::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    results_.clear();
}