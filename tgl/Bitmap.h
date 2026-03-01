#ifndef BITMAP_H
#define BITMAP_H

#include <memory>
#include <cstring>
#include <cassert>

//
// Bitmap
//
class Bitmap
{
    /**
     * Coordinates:
     *
     * LA->
     * (0,0)  (w,0)(a,0)
     *  +------+---+
     *  |      |   |
     *  |      |   |
     *  +------+---+
     * (0,h)  (w,h)(a,h)
     *              ->HA
     */
public:
    Bitmap(uint32_t width, uint32_t height, size_t bytesPerPixel = 1);
    ~Bitmap() = default;

    // Buffer
    inline size_t bufferSize() const { return m_bufferSize; }
    inline unsigned char *bufferPointer() const
    {
        return reinterpret_cast<unsigned char *>(m_buffer.get());
    }
    inline size_t rowBytes() const { return m_rowBytes; }
    inline unsigned char *rowPointer(uint32_t row) const
    {
        assert(row < m_height);
        return reinterpret_cast<unsigned char *>(m_buffer.get()) + row * m_rowBytes;
    }
    inline size_t bytesPerPixel() const { return m_bytesPerPixel; }
    inline void clear() { std::memset(bufferPointer(), 0, bufferSize()); }

    // Accessor
    template <typename T>
    inline T &at(uint32_t x, uint32_t y, size_t offset = 0)
    {
        assert(x < m_width && y < m_height && offset + sizeof(T) <= m_bytesPerPixel);
        return *reinterpret_cast<T *>(rowPointer(y) + x * m_bytesPerPixel + offset);
    }

    // Getters
    inline uint32_t getHeight() const { return m_height; }
    inline uint32_t getWidth() const { return m_width; }
    inline bool isValid() const { return m_buffer != nullptr; }

    // Copying and move semantics
    Bitmap(const Bitmap &other) = delete;
    Bitmap &operator=(const Bitmap &other) = delete;
    Bitmap(Bitmap &&other) noexcept;
    Bitmap &operator=(Bitmap &&other) noexcept;

private:
    uint32_t m_width, m_height;
    size_t m_bytesPerPixel, m_rowBytes, m_bufferSize;
    std::unique_ptr<uint32_t[]> m_buffer;
};

#endif
