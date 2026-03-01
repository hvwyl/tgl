#ifndef IMAGE_H
#define IMAGE_H

// #define IMAGE_ENABLE_PNG_SUPPORT
// #define IMAGE_ENABLE_JPEG_SUPPORT

#include "Bitmap.h"
#include "Geometry.h"
#include "Texture.h"
#include <memory>

//
// Image
//
class Image
{
    /**
     * Coordinates:
     *
     * LA->
     * (0,0)  (w,0)
     *  +------+
     *  |      |
     *  |      |
     *  +------+
     * (0,h)  (w,h)
     *         ->HA
     */
public:
    enum Layout : uint16_t
    {
        LAYOUT_RGB888,
        LAYOUT_BGR888,
        LAYOUT_RGBA8888,
        LAYOUT_BGRA8888,
        LAYOUT_Lum8,
        LAYOUT_Alpha8,
        LAYOUT_LumAlpha88
    };
    enum Flags : uint16_t
    {
        FLAG_NONE = 0u,
        FLAG_FLIP_X = (1u << 0),
        FLAG_FLIP_Y = (1u << 1),
        FLAG_PREMULTIPLIED = (1u << 2),
    };
    Image() {};
    Image(size_t width, size_t height, uint16_t layout, uint16_t flags);
    Image(const Bitmap &bitmap, uint16_t layout, uint16_t flags);
    ~Image() = default;

    inline void update(const Bitmap &bitmap, size_t x, size_t y, size_t width, size_t height)
    {
        m_texture->update(x, y, width, height, bitmap.bufferPointer());
    }

    // Clipping
    struct Clip
    {
        Bounds uv0b;
    };

    Clip crop(float sx, float sy, float sWidth, float sHeight) const;

    // Getters
    inline int getHeight() const { return m_texture->getHeight(); }
    inline int getWidth() const { return m_texture->getWidth(); }
    inline bool isValid() const { return m_texture != nullptr && m_texture->isValid(); }

private:
    std::shared_ptr<Texture> m_texture = nullptr;
    uint16_t m_layout;
    uint16_t m_flags;
    friend class Graphics;
};

#endif
