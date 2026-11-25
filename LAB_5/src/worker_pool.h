#pragma once

#include "file_result.h"
#include "thread_safe_queue.h"

#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

class WorkerPool {
public:
    WorkerPool(ThreadSafeQueue<std::string>& queue,
               ResultsStorage& results);
    ~WorkerPool();

    WorkerPool(const WorkerPool&)            = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    void start(std::size_t numThreads = 0);
    void stop();

    std::size_t threadCount() const { return numThreads_; }

private:
    void workerLoop();

    ThreadSafeQueue<std::string>& queue_;
    ResultsStorage&               results_;
    std::vector<std::thread>      workers_;
    std::atomic<bool>             running_{false};
    std::size_t                   numThreads_ = 0;
};