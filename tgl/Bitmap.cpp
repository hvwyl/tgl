#include "Bitmap.h"

static_assert(sizeof(uint32_t) == 4, "sizeof(uint32_t) must be equal to 4");

Bitmap::Bitmap(uint32_t width, uint32_t height, size_t bytesPerPixel)
    : m_width(width), m_height(height), m_bytesPerPixel(bytesPerPixel)
{
    m_rowBytes = (width * m_bytesPerPixel + 3) & ~((unsigned)3);
    m_bufferSize = m_rowBytes * height;
    m_buffer = std::make_unique<uint32_t[]>(m_bufferSize >> 2);
}
