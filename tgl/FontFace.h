#ifndef FONTFACE_H
#define FONTFACE_H

#include <istream>
#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H

//
// FontManager
//
class FontManager
{
public:
    static FontManager &getInstance();

private:
    inline FontManager()
    {
        if (FT_Init_FreeType(&m_ftLib))
            throw std::runtime_error("Freetype error: FT_Init_FreeType");
    };
    inline ~FontManager()
    {
        if (m_ftLib != nullptr)
            FT_Done_FreeType(m_ftLib);
    };

    FT_Library m_ftLib = nullptr;

    friend class FontFace;
};

//
// FontFace
//
class FontFace
{
public:
    FontFace(const char *path);
    FontFace(std::unique_ptr<std::istream> file);
    ~FontFace();

    inline void setPixelSize(size_t pixelSize)
    {
        if (FT_Set_Pixel_Sizes(m_ftFace, 0, pixelSize))
            throw std::runtime_error("Freetype error: FT_Set_Pixel_Sizes");
    }
    inline void loadChar(uint64_t codepoint)
    {
        if (FT_Load_Char(m_ftFace, codepoint, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP))
            throw std::runtime_error("Freetype error: FT_Load_Char");
    }

    // Getters
    inline FT_GlyphSlot getGlyphSlot() const
    {
        return m_ftFace->glyph;
    }
    inline FT_Face getFTFace() const
    {
        return m_ftFace;
    }
    inline FT_Library getFTLibrary() const
    {
        return m_fontManager.m_ftLib;
    }

    // Copying and move semantics
    FontFace(const FontFace &other) = delete;
    FontFace &operator=(const FontFace &other) = delete;
    FontFace(FontFace &&other) noexcept;
    FontFace &operator=(FontFace &&other) noexcept;

private:
    class FontManager &m_fontManager;
    std::unique_ptr<std::istream> m_istreamPointer;

    FT_StreamRec m_ftStreamRec;
    FT_Face m_ftFace = nullptr;
};

#endif
