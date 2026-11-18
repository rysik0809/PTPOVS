#include "logger.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

std::string currentTimestamp() {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return os.str();
}

} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    logPath_ = path;
}

void Logger::logException(const std::exception& ex,
                          const std::string& context) {
    logMessage("EXCEPTION", context, ex.what());
}

void Logger::logMessage(const std::string& level,
                        const std::string& context,
                        const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ofstream out(logPath_, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Не удалось открыть журнал: " << logPath_ << "\n";
        return;
    }

    out << "[" << currentTimestamp() << "] "
        << "[" << level << "] "
        << context << ": " << message << "\n";
}