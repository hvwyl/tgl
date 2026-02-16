#ifndef TEXTURE_H
#define TEXTURE_H

#include "OpenGLHeader.h"
#include <cstdint>
#include <cstddef>

//
// Texture
//
class Texture
{
    /**
     * Coordinates:
     *
     * (0,0)  (1,0)
     *  +------+
     *  |      |
     *  |      |
     *  +------+
     * (0,1)  (1,1)
     */
public:
    enum Format : GLenum
    {
        FORMAT_RED = GL_RED,
        FORMAT_RG = GL_RG,
        FORMAT_RGB = GL_RGB,
        FORMAT_RGBA = GL_RGBA
    };
    enum Flags : uint32_t
    {
        FLAG_UNALIGNED = 1 << 0, // Disable 4-byte row alignment for pixel unpacking. (GL_UNPACK_ALIGNMENT)
        FLAG_MIPMAPS = 1 << 1,   // Generate mipmaps during creation of the image.
        FLAG_NEAREST = 1 << 2,   // Image interpolation is Nearest instead Linear.
        FLAG_REPEAT_X = 1 << 3,  // Repeat image in X direction.
        FLAG_REPEAT_Y = 1 << 4,  // Repeat image in Y direction.
        FLAG_FLOAT = 1 << 5,     // Use GL_FLOAT instead of GL_UNSIGNED_BYTE for pixel unpacking.
    };
    Texture() : m_tex(0), m_format(FORMAT_RGBA), m_width(0), m_height(0), m_flags(0) {}
    Texture(size_t width, size_t height, uint32_t format, uint32_t flags, const unsigned char *pixels);
    ~Texture();
    void update(size_t x, size_t y, size_t width, size_t height, const unsigned char *pixels);

    // Getters
    inline GLuint getTex() const { return m_tex; }
    inline uint32_t getFormat() const { return m_format; }
    inline uint32_t getFlags() const { return m_flags; }
    inline int getWidth() const { return m_width; }
    inline int getHeight() const { return m_height; }
    inline bool isValid() const { return m_tex != 0; }

    // Copying and move semantics
    Texture(const Texture &other) = delete;
    Texture &operator=(const Texture &other) = delete;
    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&other) noexcept;

private:
    GLuint m_tex;
    GLenum m_format;
    size_t m_width;
    size_t m_height;
    uint32_t m_flags;
};

#endif
