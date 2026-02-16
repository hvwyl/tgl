#ifndef FONTATLAS_H
#define FONTATLAS_H

#include "FontFace.h"
#include "Bitmap.h"
#include "Geometry.h"
#include "RectanizerSkyline.h"
#include "Texture.h"
#include <unordered_map>
#include <cstring>

struct GlyphKey
{
    uint32_t codepoint;
    uint32_t pixelSize;
    bool operator==(const GlyphKey &other) const
    {
        return codepoint == other.codepoint && pixelSize == other.pixelSize;
    }
};

struct GlyphKeyHash
{
    size_t operator()(const GlyphKey &key) const
    {
        return std::hash<uint32_t>()(key.codepoint) ^ (std::hash<uint32_t>()(key.pixelSize) << 16);
    }
};

struct GlyphValue
{
    /* Metric */
    int width, height;
    int bearingX, bearingY;
    float advance;

    FT_Pos bbox_xMin;
    FT_Pos bbox_yMin;

    /* Glyph */
    size_t versionUV;
    Bounds textureUV;
};

class FontAtlas
{
public:
    static constexpr size_t ATLAS_SIZE = 1024;
    static constexpr size_t ATLAS_PADDING = 1;

    FontAtlas(std::unique_ptr<FontFace> face, size_t atlasSize = ATLAS_SIZE);
    ~FontAtlas() = default;
    void reset();

    inline size_t maxPixelSize() const
    {
        return m_atlasSize - 2 * ATLAS_PADDING;
    }
    GlyphValue *metrics(uint32_t codepoint, size_t pixelSize);
    GlyphValue *glyph(uint32_t codepoint, size_t pixelSize);

    // Texture
    inline std::shared_ptr<Texture> getTexture()
    {
        return m_texture;
    };
    void syncTexture();

    // Getters
    inline size_t getHeight() const { return m_atlasBuffer.getHeight(); }
    inline size_t getWidth() const { return m_atlasBuffer.getWidth(); }

    // Buffer
    inline size_t rowBytes() { return m_atlasBuffer.rowBytes(); }
    inline size_t bufferSize() { return m_atlasBuffer.bufferSize(); }
    inline unsigned char *bufferPointer() { return m_atlasBuffer.bufferPointer(); }

private:
    std::unique_ptr<FontFace> m_fontFace;
    size_t m_atlasSize;
    Bitmap m_atlasBuffer;
    std::unordered_map<GlyphKey, GlyphValue, GlyphKeyHash> m_atlasMap;
    RectanizerSkyline m_rectanizer;
    std::shared_ptr<Texture> m_texture;
    bool m_isDirty = false;
    size_t m_currentVersion = 1;
    size_t m_pixelSize = 0;

    FT_GlyphSlot loadCharFTGlyphSlot(uint32_t codepoint, size_t pixelSize);
    void loadCharMetrics(FT_GlyphSlot glyph_slot, GlyphValue &out_item);
    bool loadCharToAtlas(FT_GlyphSlot glyph_slot, GlyphValue &out_item);
};

#endif
