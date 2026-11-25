#pragma once

#include "async_processor.h"
#include "file_result.h"
#include "file_watcher.h"
#include "thread_safe_queue.h"
#include "worker_pool.h"

#include <mutex>
#include <string>

class ConsoleUI {
public:
    ConsoleUI(FileWatcher&      watcher,
              WorkerPool&       pool,
              AsyncProcessor&   async,
              ResultsStorage&   results,
              ThreadSafeQueue<std::string>& queue);

    void run();

private:
    void printMenu() const;

    void handleQueueState() const;
    void handleFileResult() const;
    void handleChangeDirectory();
    void handleResetResults() const;
    void handleResetQueue();
    void handleAsyncStart();
    void handleAsyncCancel() const;
    void handleAsyncStatus() const;
    void handleShowDirectory() const;
    void handleShowExceptionsLog() const;   // ЛР №5, п.2

    std::string promptLine(const std::string& prompt) const;
    int         promptInt (const std::string& prompt) const;

    FileWatcher&                  watcher_;
    WorkerPool&                   pool_;
    AsyncProcessor&               async_;
    ResultsStorage&               results_;
    ThreadSafeQueue<std::string>& queue_;
    mutable std::mutex            ioMutex_;
};