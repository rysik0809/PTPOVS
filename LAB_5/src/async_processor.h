#pragma once

#include "file_result.h"
#include "thread_safe_queue.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

/**
 * @brief Асинхронная пакетная обработка набора файлов.
 *
 * start(files) запускает фоновый поток, который создаёт свой пул
 * рабочих потоков (на базе того же ThreadSafeQueue) и обрабатывает
 * файлы функциями Лабораторной №2/№3. Управление сразу возвращается.
 *
 * cancel()      — сигнал остановки; оставшиеся файлы помечаются Cancelled.
 * status()      — снимок статуса по каждому файлу.
 * wait()        — дождаться завершения (для корректного shutdown).
 */
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

private:
    void runWorkers();

    std::unique_ptr<ThreadSafeQueue<std::string>> queue_;
    std::unique_ptr<ResultsStorage>               results_;
    std::thread                                   thread_;
    std::atomic<bool>                             running_{false};
    std::atomic<bool>                             cancelled_{false};
    std::atomic<std::size_t>                      remaining_{0};
    std::size_t                                   numThreads_ = 0;
};