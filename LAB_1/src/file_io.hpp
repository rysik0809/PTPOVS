#pragma once

#include <string>
#include <string_view>

namespace file_io
{
class fileBuffer
{
public:
    explicit fileBuffer(const std::string& path);
    std::string_view view() const noexcept { return text_; }
    const std::string& str() const noexcept { return text_; }
    std::size_t size() const noexcept { return text_.size(); }
    bool empty() const noexcept { return text_.empty(); }

    fileBuffer(const fileBuffer&) = delete;
    fileBuffer& operator=(const fileBuffer&) = delete;
    fileBuffer(fileBuffer&&) noexcept = default;
    fileBuffer& operator=(fileBuffer&&) noexcept = default;

private:
    std::string text_;
};
}