#include "file_io.hpp"

#include <fstream>
#include <stdexcept>

fileBuffer::fileBuffer(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Не удалось открыть файл: " + path);
    }

    const std::streampos end = file.tellg();
    if (end == std::streampos(-1)) {
        throw std::runtime_error("Не удалось определить размер файла: " + path);
    }

    const std::streamsize size = static_cast<std::streamsize>(end);
    if (size > 0) {
        text_.resize(static_cast<std::size_t>(size));

        file.seekg(0, std::ios::beg);
        if (!file.read(text_.data(), size)) {
            throw std::runtime_error("Ошибка чтения файла: " + path);
        }
    }
}