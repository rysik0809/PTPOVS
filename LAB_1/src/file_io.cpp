#include "file_io.hpp"

#include <fstream>
#include <stdexcept>

file_io::fileBuffer::fileBuffer(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) 
    {
        throw std::runtime_error("Не удалось прочитать: " + path);
    }

    auto size = file.tellg();

    if (size < 0)
    {
        throw std::runtime_error("Размер не определен: " + path);
    }

    file.seekg(0, std::ios::beg);

    text_.resize(static_cast<std::size_t>(size));

    if (size >0 && !file.read(text_.data(), size))
    {
        throw std::runtime_error("Ошибка чтения: " + path);
    }
}