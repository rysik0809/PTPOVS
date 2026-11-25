#include "file_watcher.h"
#include "exceptions.h"

#include <filesystem>

FileWatcher::FileWatcher(ThreadSafeQueue<std::string>& queue)
    : queue_(queue) {}

FileWatcher::~FileWatcher() { stop(); }

void FileWatcher::start(const std::string& directory) {
    if (running_.exchange(true)) return;

    try {
        ensureDirectoryReadable(directory);
    }
    catch (...) {
        running_ = false;
        throw;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        directory_ = directory;
        seen_.clear();
        ++generation_;
    }
    thread_ = std::thread(&FileWatcher::run, this);
}

void FileWatcher::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

void FileWatcher::setDirectory(const std::string& directory) {
    ensureDirectoryReadable(directory); // может бросить DirectoryAccessException
    std::lock_guard<std::mutex> lock(mutex_);
    directory_ = directory;
    seen_.clear();
    ++generation_;
    loggedFailedGeneration_ = std::numeric_limits<std::uint64_t>::max();
}

std::string FileWatcher::getDirectory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return directory_;
}

void FileWatcher::resetSeen() {
    std::lock_guard<std::mutex> lock(mutex_);
    seen_.clear();
    ++generation_;
}

void FileWatcher::run() {
    while (running_.load()) {
        std::string   dir;
        std::uint64_t gen;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dir = directory_;
            gen = generation_;
        }

        if (!dir.empty()) {
            std::error_code ec;
            std::filesystem::directory_iterator it(dir, ec);
            if (ec) {
                // Ошибка доступа к каталогу — репортим один раз на поколение.
                if (loggedFailedGeneration_ != gen) {
                    try {
                        throw DirectoryAccessException(dir, ec.message());
                    }
                    catch (const std::exception& e) {
                        logException("FileWatcher::run", e);
                    }
                    loggedFailedGeneration_ = gen;
                }
            }
            else {
                loggedFailedGeneration_ =
                    std::numeric_limits<std::uint64_t>::max();

                for (const auto& entry : it) {
                    std::error_code statEc;
                    if (!entry.is_regular_file(statEc) || statEc) continue;

                    const std::string path = entry.path().string();

                    std::lock_guard<std::mutex> lock(mutex_);
                    if (generation_ != gen) break; // каталог сменился
                    if (seen_.insert(path).second) {
                        queue_.push(path);         // новый файл
                    }
                }
            }
        }
        std::this_thread::sleep_for(pollInterval_);
    }
}