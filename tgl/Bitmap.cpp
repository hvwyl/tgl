#include "Bitmap.h"

static_assert(sizeof(uint32_t) == 4, "sizeof(uint32_t) must be equal to 4");

Bitmap::Bitmap(uint32_t width, uint32_t height, size_t bytesPerPixel)
    : m_width(width), m_height(height), m_bytesPerPixel(bytesPerPixel)
{
    m_rowBytes = (width * m_bytesPerPixel + 3) & ~((unsigned)3);
    m_bufferSize = m_rowBytes * height;
    m_buffer = std::make_unique<uint32_t[]>(m_bufferSize >> 2);
}

Bitmap::Bitmap(Bitmap &&other) noexcept
    : m_width(other.m_width),
      m_height(other.m_height),
      m_bytesPerPixel(other.m_bytesPerPixel),
      m_rowBytes(other.m_rowBytes),
      m_bufferSize(other.m_bufferSize),
      m_buffer(std::move(other.m_buffer))
{
}

Bitmap &Bitmap::operator=(Bitmap &&other) noexcept
{
    if (this != &other)
    {
        m_width = other.m_width;
        m_height = other.m_height;
        m_bytesPerPixel = other.m_bytesPerPixel;
        m_rowBytes = other.m_rowBytes;
        m_bufferSize = other.m_bufferSize;
        m_buffer = std::move(other.m_buffer);
    }
    return *this;
}
