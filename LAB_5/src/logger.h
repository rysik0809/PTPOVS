#pragma once

#include <exception>
#include <mutex>
#include <string>

/**
 * @brief Потокобезопасный журнал исключительных ситуаций.
 *
 * Все записи идут в один файл (по умолчанию exceptions.log).
 */
class Logger {
public:
    static Logger& instance();

    void setLogFile(const std::string& path);
    const std::string& logFile() const noexcept { return logPath_; }

    void logException(const std::exception& ex,
                      const std::string& context);

    void logMessage(const std::string& level,
                    const std::string& context,
                    const std::string& message);

private:
    Logger() = default;

    mutable std::mutex mutex_;
    std::string        logPath_ = "exceptions.log";
};