#include "worker_pool.h"
#include "file_processor.h"

#include <algorithm>

WorkerPool::WorkerPool(ThreadSafeQueue<std::string>& queue,
                       ResultsStorage& results)
    : queue_(queue), results_(results) {}

WorkerPool::~WorkerPool() { stop(); }

void WorkerPool::start(std::size_t numThreads) {
    if (running_.exchange(true)) return;

    numThreads_ = numThreads ? numThreads
                             : std::max(1u, std::thread::hardware_concurrency());

    workers_.reserve(numThreads_);
    for (std::size_t i = 0; i < numThreads_; ++i) {
        workers_.emplace_back(&WorkerPool::workerLoop, this);
    }
}

void WorkerPool::stop() {
    if (!running_.exchange(false)) return;
    queue_.stop();                              // разбудить всех
    for (auto& t : workers_) if (t.joinable()) t.join();
    workers_.clear();
}

void WorkerPool::workerLoop() {
    std::string path;
    while (queue_.pop(path)) {
        if (!running_.load()) break;            // остановка — не обрабатываем
        processOneFile(path, results_);
    }
}