#pragma once

#include "file_result.h"
#include "thread_safe_queue.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

class AsyncProcessor {
public:
    explicit AsyncProcessor(std::size_t numThreads = 0);
    ~AsyncProcessor();

    AsyncProcessor(const AsyncProcessor&)            = delete;
    AsyncProcessor& operator=(const AsyncProcessor&) = delete;

    void start(const std::vector<std::string>& files);
    void cancel();
    void wait();

    bool                     isRunning() const;
    std::vector<FileResult>  status() const;
    void                     clearResults();
    std::size_t              threadCount() const { return numThreads_; }

    std::chrono::milliseconds elapsedTime() const;

private:
    void runWorkers();

    // ВАЖНО: порядок соответствует .cpp-инициализатору.
    std::unique_ptr<ResultsStorage>               results_;
    std::unique_ptr<ThreadSafeQueue<std::string>> queue_;
    std::thread                                   thread_;
    std::atomic<bool>                             running_{false};
    std::atomic<bool>                             cancelled_{false};
    std::atomic<std::size_t>                      remaining_{0};
    std::size_t                                   numThreads_ = 0;

    std::chrono::steady_clock::time_point         startTime_;
    std::chrono::steady_clock::time_point         endTime_;
    std::atomic<bool>                             finished_{true};
};