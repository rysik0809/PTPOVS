#pragma once

#include <cstddef>
#include <string>
#include <string_view>

/**
 * @brief Владелец содержимого файла в памяти.
 *
 * Класс читает файл целиком в бинарном режиме и предоставляет
 * либо невладеющий view, либо константную ссылку на строку.
 */
class fileBuffer
{
public:
    /**
     * @param path путь к файлу.
     * @throws std::runtime_error если файл не удалось открыть или прочитать.
     */
    explicit fileBuffer(const std::string& path);

    /// Невладеющий вид на содержимое.
    /// Валиден, пока живёт сам fileBuffer.
    std::string_view view() const noexcept { return text_; }

    /// Константная ссылка на владеющую строку. Без копирования.
    /// Для случаев, когда нужен именно std::string.
    const std::string& str() const noexcept { return text_; }

    /// Размер в байтах.
    std::size_t size() const noexcept { return text_.size(); }

    bool empty() const noexcept { return text_.empty(); }

    // Копирование запрещено: класс владеет буфером.
    fileBuffer(const fileBuffer&) = delete;
    fileBuffer& operator=(const fileBuffer&) = delete;

    // Перемещение разрешено.
    fileBuffer(fileBuffer&&) noexcept = default;
    fileBuffer& operator=(fileBuffer&&) noexcept = default;

private:
    std::string text_;
};