#include "text_utils.hpp"

#include <stdexcept>
#include <thread>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

fileBuffer::fileBuffer(const std::string& path)
{
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0)
    {
        throw std::runtime_error("Не удалось открыть файл: " + path);
    }

    struct stat st{};
    if (::fstat(fd_, &st) != 0)
    {
        ::close(fd_); fd_ = -1;
        throw std::runtime_error("fstat: " + path);
    }

    if (st.st_size <= 0) return;

    size_ = static_cast<std::size_t>(st.st_size);

    void* p = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (p == MAP_FAILED)
    {
        ::close(fd_); fd_ = -1; size_ = 0;
        throw std::runtime_error("mmap: " + path);
    }
    data_ = static_cast<const char*>(p);

    ::posix_madvise(const_cast<char*>(data_), size_, POSIX_MADV_SEQUENTIAL);
}

void fileBuffer::close() noexcept
{
    if (data_)
    {
        ::munmap(const_cast<char*>(data_), size_); data_ = nullptr;
    }
    if (fd_ >= 0)
    {
        ::close(fd_); fd_ = -1;
    }
    size_ = 0;
}

fileBuffer::~fileBuffer() { close(); }

fileBuffer::fileBuffer(fileBuffer&& o) noexcept
    : data_(o.data_), size_(o.size_), fd_(o.fd_)
{
    o.data_ = nullptr; o.size_ = 0; o.fd_ = -1;
}

fileBuffer& fileBuffer::operator=(fileBuffer&& o) noexcept
{
    if (this == &o) return *this;
    close();
    data_ = o.data_; size_ = o.size_; fd_ = o.fd_;
    o.data_ = nullptr; o.size_ = 0; o.fd_ = -1;
    return *this;
}

namespace utf8
{

    namespace
    {

        void appendUtf8(std::string& out, char32_t cp)
        {
            if (cp <= 0x7F) {
                out.push_back(static_cast<char>(cp));
            } else if (cp <= 0x7FF) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp <= 0xFFFF) {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }

        char32_t toLowerCp(char32_t cp) noexcept
        {
            if (cp >= U'A' && cp <= U'Z')     return cp + 32;
            if (cp >= 0x0410 && cp <= 0x042F) return cp + 0x20;
            if (cp >= 0x0400 && cp <= 0x040F) return cp + 0x50;
            return cp;
        }

    } // namespace

    bool decodeOne(std::string_view s, std::size_t& i, char32_t& cp) noexcept
    {
        if (i >= s.size()) return false;

        const auto c0 = static_cast<unsigned char>(s[i]);

        if (c0 < 0x80)
        {
            cp = c0; ++i;
            return true;
        }

        std::size_t len = 0;

        if ((c0 & 0xE0) == 0xC0)      { len = 2; cp = c0 & 0x1F; }
        else if ((c0 & 0xF0) == 0xE0) { len = 3; cp = c0 & 0x0F; }
        else if ((c0 & 0xF8) == 0xF0) { len = 4; cp = c0 & 0x07; }
        else return false;

        if (i + len > s.size()) return false;

        for (std::size_t k = 1; k < len; ++k)
        {
            const auto c = static_cast<unsigned char>(s[i + k]);
            if ((c & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (c & 0x3F);
        }

        i += len;
        return true;
    }

    bool isWordCodepoint(char32_t cp) noexcept
    {
        if (cp >= U'A' && cp <= U'Z') return true;
        if (cp >= U'a' && cp <= U'z') return true;
        if (cp >= U'0' && cp <= U'9') return true;
        if (cp >= 0x00C0 && cp <= 0x024F) return true;
        if (cp >= 0x0400 && cp <= 0x04FF) return true;
        if (cp >= 0x0500 && cp <= 0x052F) return true;
        return false;
    }

    std::string toLower(std::string_view s)
    {
        // Быстрый путь: слово не содержит ни [A-Z], ни байтов >= 0x80 —
        // значит, нижний регистр уже достигнут. Копируем одним memcpy.
        bool needsWork = false;
        for (unsigned char c : s)
        {
            if ((c >= 'A' && c <= 'Z') || c >= 0x80) { needsWork = true; break; }
        }
        if (!needsWork) return std::string(s);

        std::string out;
        out.reserve(s.size());

        std::size_t i = 0;
        while (i < s.size())
        {
            const std::size_t start = i;
            char32_t cp = 0;

            if (!decodeOne(s, i, cp))
            {
                out.push_back(s[start]);
                i = start + 1;
                continue;
            }

            appendUtf8(out, toLowerCp(cp));
        }
        return out;
    }

} // namespace utf8

namespace parallel
{

    namespace
    {

        std::size_t adjustBoundary(std::string_view text, std::size_t pos)
        {
            while (pos < text.size())
            {
                std::size_t i = pos;
                char32_t cp = 0;

                if (!utf8::decodeOne(text, i, cp))
                {
                    ++pos; continue;
                }

                if (!utf8::isWordCodepoint(cp))   return pos;

                pos = i;
            }
            return text.size();
        }

    } // namespace

    std::size_t hardwareThreads()
    {
        auto n = std::thread::hardware_concurrency();
        return n ? n : 1;
    }

    std::vector<Chunk> splitChunks(std::string_view text, std::size_t n)
    {
        std::vector<Chunk> chunks;
        if (n == 0)
        {
            n = 1;
        }

        const std::size_t len = text.size();
        if (len < n * 4096)
        {
            n = 1;
        }
        chunks.reserve(n);

        std::size_t begin = 0;
        for (std::size_t i = 0; i + 1 < n; ++i)
        {
            const std::size_t target = (i + 1) * len / n;
            const std::size_t end = adjustBoundary(text, target);
            chunks.emplace_back(begin, end);
            begin = end;
        }
        chunks.emplace_back(begin, len);

        return chunks;
    }

} // namespace parallel