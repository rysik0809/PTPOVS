#include "async_processor.h"
#include "file_processor.h"

#include <algorithm>
#include <stdexcept>

// Порядок инициализации в списке изменен на правильный: 
// сначала results_ (так как он объявлен первым в .h), затем numThreads_.
AsyncProcessor::AsyncProcessor(std::size_t numThreads)
    : results_(std::make_unique<ResultsStorage>()),
      numThreads_(numThreads ? numThreads
                             : std::max(1u, std::thread::hardware_concurrency())) {}

AsyncProcessor::~AsyncProcessor() {
    cancel();
    wait();
}

void AsyncProcessor::start(const std::vector<std::string>& files) {
    if (running_.exchange(true)) {
        throw std::runtime_error("AsyncProcessor already running");
    }
    if (thread_.joinable()) thread_.join();

    cancelled_ = false;
    finished_  = false;
    startTime_ = std::chrono::steady_clock::now();
    
    results_->clear();
    queue_     = std::make_unique<ThreadSafeQueue<std::string>>();
    remaining_ = files.size();

    if (files.empty()) {
        queue_->stop();
        running_ = false;
        finished_ = true;
        endTime_ = std::chrono::steady_clock::now();
        return;
    }

    for (const auto& f : files) {
        queue_->push(f);
        results_->markQueued(f);
    }
    thread_ = std::thread(&AsyncProcessor::runWorkers, this);
}

void AsyncProcessor::cancel() {
    cancelled_ = true;
    if (queue_) queue_->stop();
}

void AsyncProcessor::wait() {
    if (thread_.joinable()) thread_.join();
}

bool AsyncProcessor::isRunning() const {
    return running_.load();
}

std::vector<FileResult> AsyncProcessor::status() const {
    return results_->snapshot();
}

void AsyncProcessor::clearResults() { results_->clear(); }

std::chrono::milliseconds AsyncProcessor::elapsedTime() const {
    if (running_.load() || !finished_.load()) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime_);
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime_ - startTime_);
}

void AsyncProcessor::runWorkers() {
    std::vector<std::thread> workers;
    workers.reserve(numThreads_);

    for (std::size_t i = 0; i < numThreads_; ++i) {
        workers.emplace_back([this]() {
            std::string path;
            while (queue_->pop(path)) {
                if (cancelled_.load()) {
                    results_->markCancelled(path);
                } else {
                    processOneFile(path, *results_);
                }
                if (remaining_.fetch_sub(1) == 1) {
                    queue_->stop();            // последний — будим остальных
                }
            }
        });
    }

    for (auto& t : workers) t.join();
    
    running_  = false;
    endTime_  = std::chrono::steady_clock::now();
    finished_ = true;
}