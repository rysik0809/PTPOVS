#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

/// Максимальный размер анализируемого файла — 24 Мб.
constexpr std::uintmax_t MAX_FILE_SIZE_BYTES = 24ull * 1024ull * 1024ull;

/// Базовый класс для всех исключений лабораторной работы №5.
class Lab5Exception : public std::runtime_error {
public:
    explicit Lab5Exception(const std::string& msg) : std::runtime_error(msg) {}
};

/// Файл превышает допустимый размер.
class FileTooLargeException : public Lab5Exception {
public:
    FileTooLargeException(const std::string& path, std::uintmax_t actualSize);

    std::uintmax_t      actualSize() const noexcept { return actualSize_; }
    std::uintmax_t      maxSize()    const noexcept { return MAX_FILE_SIZE_BYTES; }
    const std::string&  path()       const noexcept { return path_; }

private:
    std::string    path_;
    std::uintmax_t actualSize_;
};

/// Файл был удалён из отслеживаемого каталога до/во время обработки.
class FileDeletedException : public Lab5Exception {
public:
    explicit FileDeletedException(const std::string& path);
    const std::string& path() const noexcept { return path_; }
private:
    std::string path_;
};

/// Нет прав на чтение каталога.
class DirectoryAccessException : public Lab5Exception {
public:
    DirectoryAccessException(const std::string& dir, const std::string& reason);
    const std::string& directory() const noexcept { return directory_; }
private:
    std::string directory_;
};

/// Файл не соответствует ожидаемому формату (Markdown).
class FormatMismatchException : public Lab5Exception {
public:
    FormatMismatchException(const std::string& path, const std::string& expected);
    const std::string& path()     const noexcept { return path_; }
    const std::string& expected() const noexcept { return expected_; }
private:
    std::string path_;
    std::string expected_;
};

/// Путь к журналу исключений (./results/exceptions.log).
std::string exceptionLogPath();

/// Записывает информацию об исключении в журнал. Потокобезопасна.
void logException(const std::string& context, const std::exception& e);

/// Проверяет, что расширение файла соответствует формату Markdown.
bool isSupportedMarkdownFile(const std::string& path);

/// Убеждается, что каталог существует и читается; при необходимости создаёт его.
/// Бросает DirectoryAccessException.
void ensureDirectoryReadable(const std::string& dir);