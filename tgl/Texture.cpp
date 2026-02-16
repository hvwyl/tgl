#include "Texture.h"
#include <cstdio>

#define CHECK_GL_ERROR(str)                                                           \
    {                                                                                 \
        GLenum err = glGetError();                                                    \
        if (err != GL_NO_ERROR)                                                       \
            std::printf("%s <function:%s, glGetError:%x>\n", str, __FUNCTION__, err); \
    }

#define PIXEL_STORE_SETUP(a, w, x, y)            \
    {                                            \
        glPixelStorei(GL_UNPACK_ALIGNMENT, a);   \
        glPixelStorei(GL_UNPACK_ROW_LENGTH, w);  \
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, x); \
        glPixelStorei(GL_UNPACK_SKIP_ROWS, y);   \
    }

#define PIXEL_STORE_RESET()                      \
    {                                            \
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);   \
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);  \
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0); \
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);   \
    }

Texture::Texture(size_t width, size_t height, uint32_t format, uint32_t flags, const unsigned char *pixels)
    : m_tex(0), m_format(format), m_width(width), m_height(height), m_flags(flags)
{
    // Generate texture
    glGenTextures(1, &m_tex);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, m_tex);

    // Setup pixel store
    PIXEL_STORE_SETUP((m_flags & FLAG_UNALIGNED) ? 1 : 4, m_width, 0, 0);

    // Upload texture
    if (m_flags & FLAG_FLOAT)
        glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_width, m_height, 0, m_format, GL_FLOAT, pixels);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_width, m_height, 0, m_format, GL_UNSIGNED_BYTE, pixels);

    // Setup min filter
    if (m_flags & FLAG_MIPMAPS)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        (m_flags & FLAG_NEAREST) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        (m_flags & FLAG_NEAREST) ? GL_NEAREST : GL_LINEAR);

    // Setup mag filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    (m_flags & FLAG_NEAREST) ? GL_NEAREST : GL_LINEAR);

    // Setup wrap mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    (m_flags & FLAG_REPEAT_X) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    (m_flags & FLAG_REPEAT_Y) ? GL_REPEAT : GL_CLAMP_TO_EDGE);

    // Reset pixel store
    PIXEL_STORE_RESET();

    // Generate mipmaps
    if (m_flags & FLAG_MIPMAPS)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Check
    CHECK_GL_ERROR("create_tex");

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
    if (m_tex != 0)
    {
        glDeleteTextures(1, &m_tex);
        m_tex = 0;
    }
}

void Texture::update(size_t x, size_t y, size_t width, size_t height, const unsigned char *pixels)
{
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, m_tex);

    // Setup pixel store
    PIXEL_STORE_SETUP((m_flags & FLAG_UNALIGNED) ? 1 : 4, m_width, x, y);

    // Upload texture
    if (m_flags & FLAG_FLOAT)
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, m_format, GL_FLOAT, pixels);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, m_format, GL_UNSIGNED_BYTE, pixels);

    // Reset pixel store
    PIXEL_STORE_RESET();

    // Check
    CHECK_GL_ERROR("update_tex");

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::Texture(Texture &&other) noexcept
    : m_tex(other.m_tex),
      m_format(other.m_format),
      m_width(other.m_width),
      m_height(other.m_height),
      m_flags(other.m_flags)
{
    other.m_tex = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept
{
    if (this != &other)
    {
        if (m_tex != 0)
        {
            glDeleteTextures(1, &m_tex);
        }
        m_tex = other.m_tex;
        m_format = other.m_format;
        m_width = other.m_width;
        m_height = other.m_height;
        m_flags = other.m_flags;
        other.m_tex = 0;
    }
    return *this;
}
