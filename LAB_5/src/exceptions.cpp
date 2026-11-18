#include "exceptions.h"

Lab5Exception::Lab5Exception(const std::string& message)
    : std::runtime_error(message) {}

FileTooLargeException::FileTooLargeException(const std::string& path,
                                             std::uintmax_t size)
    : Lab5Exception("Запрещён анализ файла больше 24 МБ: " + path +
                    " (размер: " + std::to_string(size) + " байт)"),
      path_(path),
      size_(size) {}

FileRemovedException::FileRemovedException(const std::string& path)
    : Lab5Exception("Файл был удалён или недоступен: " + path),
      path_(path) {}

DirectoryAccessException::DirectoryAccessException(const std::string& path,
                                                   const std::string& reason)
    : Lab5Exception("Нет прав на чтение каталога " + path + ": " + reason),
      path_(path),
      reason_(reason) {}

UnsupportedFileFormatException::UnsupportedFileFormatException(
    const std::string& path,
    const std::string& reason)
    : Lab5Exception("Файл не соответствует формату Markdown " + path +
                    ": " + reason),
      path_(path),
      reason_(reason) {}