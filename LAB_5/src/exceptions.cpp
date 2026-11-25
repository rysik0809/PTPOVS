#include "exceptions.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>

namespace {
std::mutex g_logMutex;

std::string formatMb(std::uintmax_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2)
        << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " Мб";
    return oss.str();
}
} // namespace

std::string exceptionLogPath() {
    return (std::filesystem::path("./results") / "exceptions.log").string();
}

FileTooLargeException::FileTooLargeException(const std::string& path,
                                             std::uintmax_t actualSize)
    : Lab5Exception("Файл '" + path + "' слишком большой: " +
                    formatMb(actualSize) + ", максимум " +
                    formatMb(MAX_FILE_SIZE_BYTES)),
      path_(path), actualSize_(actualSize) {}

FileDeletedException::FileDeletedException(const std::string& path)
    : Lab5Exception("Файл '" + path +
                    "' был удалён из отслеживаемого каталога"),
      path_(path) {}

DirectoryAccessException::DirectoryAccessException(const std::string& dir,
                                                   const std::string& reason)
    : Lab5Exception("Нет доступа к каталогу '" + dir + "': " + reason),
      directory_(dir) {}

FormatMismatchException::FormatMismatchException(const std::string& path,
                                                 const std::string& expected)
    : Lab5Exception("Файл '" + path +
                    "' не соответствует ожидаемому формату (" + expected + ")"),
      path_(path), expected_(expected) {}

bool isSupportedMarkdownFile(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".md" || ext == ".markdown" ||
           ext == ".mdown" || ext == ".mkd";
}

void ensureDirectoryReadable(const std::string& dir) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) {
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            throw DirectoryAccessException(
                dir, "не удалось создать каталог: " + ec.message());
        }
    }
    // Пробный обход каталога — самый портируемый способ проверить права.
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) {
        throw DirectoryAccessException(dir, ec.message());
    }
}

void logException(const std::string& context, const std::exception& e) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    std::error_code ec;
    std::filesystem::create_directories("./results", ec);

    const std::string logPath = exceptionLogPath();
    std::ofstream log(logPath, std::ios::app);
    if (!log.is_open()) {
        std::cerr << "[" << context << "] " << e.what()
                  << " (не удалось открыть журнал " << logPath << ")\n";
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto tt  = std::chrono::system_clock::to_time_t(now);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &tt);
#else
    localtime_r(&tt, &tmv);
#endif

    log << "[" << std::put_time(&tmv, "%Y-%m-%d %H:%M:%S") << "] "
        << context << ": " << e.what() << "\n";
}