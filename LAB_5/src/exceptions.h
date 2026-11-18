#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

/// Базовый класс для пользовательских исключений Лабораторной №5.
class Lab5Exception : public std::runtime_error {
public:
    explicit Lab5Exception(const std::string& message);
};

/// Файл превышает максимально допустимый размер (24 МБ).
class FileTooLargeException : public Lab5Exception {
public:
    static constexpr std::uintmax_t MaxFileSize = 24ull * 1024 * 1024;

    FileTooLargeException(const std::string& path, std::uintmax_t size);

    const std::string& path() const noexcept { return path_; }
    std::uintmax_t     size() const noexcept { return size_; }

private:
    std::string    path_;
    std::uintmax_t size_ = 0;
};

/// Файл был удалён из каталога или стал недоступен.
class FileRemovedException : public Lab5Exception {
public:
    explicit FileRemovedException(const std::string& path);
    const std::string& path() const noexcept { return path_; }

private:
    std::string path_;
};

/// Нет прав на чтение указанного каталога.
class DirectoryAccessException : public Lab5Exception {
public:
    DirectoryAccessException(const std::string& path,
                             const std::string& reason);

    const std::string& path()   const noexcept { return path_; }
    const std::string& reason() const noexcept { return reason_; }

private:
    std::string path_;
    std::string reason_;
};

/// Файл не соответствует формату Markdown.
class UnsupportedFileFormatException : public Lab5Exception {
public:
    UnsupportedFileFormatException(const std::string& path,
                                   const std::string& reason);

    const std::string& path()   const noexcept { return path_; }
    const std::string& reason() const noexcept { return reason_; }

private:
    std::string path_;
    std::string reason_;
};