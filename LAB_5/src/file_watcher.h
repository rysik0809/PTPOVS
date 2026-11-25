#pragma once

#include "thread_safe_queue.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <mutex>
#include <set>
#include <string>
#include <thread>

/**
 * @brief Поток-наблюдатель, опрашивающий каталог и добавляющий
 *        в очередь новые (ещё не виденные) файлы.
 *
 * При недоступности каталога (нет прав / удалён) бросается
 * DirectoryAccessException, которая пишется в журнал один раз
 * на «поколение» (без спама каждые 500 мс).
 */
class FileWatcher {
public:
    explicit FileWatcher(ThreadSafeQueue<std::string>& queue);
    ~FileWatcher();

    FileWatcher(const FileWatcher&)            = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    void        start(const std::string& directory);
    void        stop();
    void        setDirectory(const std::string& directory);
    std::string getDirectory() const;
    void        resetSeen();

private:
    void run();

    ThreadSafeQueue<std::string>& queue_;
    std::thread                   thread_;
    mutable std::mutex            mutex_;
    std::string                   directory_;
    std::set<std::string>         seen_;
    std::uint64_t                 generation_ = 0;
    std::atomic<bool>             running_{false};
    std::chrono::milliseconds     pollInterval_{500};

    /// Поколение, для которого мы уже сообщили об ошибке доступа.
    std::uint64_t loggedFailedGeneration_ = std::numeric_limits<std::uint64_t>::max();
};