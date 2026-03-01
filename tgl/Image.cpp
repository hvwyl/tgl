#include "Image.h"

static inline uint32_t layoutToFormat(uint16_t layout)
{
    switch (layout)
    {
    case Image::LAYOUT_RGB888:
    case Image::LAYOUT_BGR888:
        return Texture::FORMAT_RGB;
    case Image::LAYOUT_RGBA8888:
    case Image::LAYOUT_BGRA8888:
        return Texture::FORMAT_RGBA;
    case Image::LAYOUT_Lum8:
    case Image::LAYOUT_Alpha8:
        return Texture::FORMAT_RED;
    case Image::LAYOUT_LumAlpha88:
        return Texture::FORMAT_RG;
    default:
        return 0;
    }
}

Image::Image(size_t width, size_t height, uint16_t layout, uint16_t flags)
    : m_layout(layout), m_flags(flags)
{
    m_texture = std::make_shared<Texture>(width, height, layoutToFormat(layout), 0, nullptr);
}

Image::Image(const Bitmap &bitmap, uint16_t layout, uint16_t flags)
    : m_layout(layout), m_flags(flags)
{
    if (!bitmap.isValid())
        return;
    m_texture = std::make_shared<Texture>(bitmap.getWidth(), bitmap.getHeight(), layoutToFormat(layout), 0, bitmap.bufferPointer());
}

Image::Clip Image::crop(float sx, float sy, float sWidth, float sHeight) const
{
    Bounds uv0b{0.0f, 0.0f, 1.0f, 1.0f};
    if (m_texture != nullptr)
    {
        // calculate texture coord
        uv0b.minx = sx / getWidth();
        uv0b.miny = sy / getHeight();
        uv0b.maxx = (sx + sWidth) / getWidth();
        uv0b.maxy = (sy + sHeight) / getHeight();
    }
    return Clip{uv0b};
}
