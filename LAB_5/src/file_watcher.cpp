#include "file_watcher.h"

#include "exceptions.h"
#include "logger.h"

#include <filesystem>

FileWatcher::FileWatcher(ThreadSafeQueue<std::string>& queue)
    : queue_(queue) {}

FileWatcher::~FileWatcher() {
    stop();
}

void FileWatcher::start(const std::string& directory) {
    if (running_.exchange(true)) return;

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
    std::lock_guard<std::mutex> lock(mutex_);
    directory_ = directory;
    seen_.clear();
    ++generation_;
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
    std::string lastAccessError;

    while (running_.load()) {
        std::string dir;
        std::uint64_t gen = 0;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            dir = directory_;
            gen = generation_;
        }

        if (!dir.empty()) {
            std::error_code ec;
            std::filesystem::directory_iterator it(dir, ec);

            if (ec) {
                if (lastAccessError != ec.message()) {
                    DirectoryAccessException ex(dir, ec.message());
                    Logger::instance().logException(ex, "FileWatcher::run");
                    lastAccessError = ec.message();
                }
            } else {
                lastAccessError.clear();

                for (const auto& entry : it) {
                    std::error_code statEc;
                    if (!entry.is_regular_file(statEc) || statEc) continue;

                    const std::string path = entry.path().string();

                    std::lock_guard<std::mutex> lock(mutex_);
                    if (generation_ != gen) break;

                    if (seen_.insert(path).second) {
                        queue_.push(path);
                    }
                }
            }
        }

        std::this_thread::sleep_for(pollInterval_);
    }
}