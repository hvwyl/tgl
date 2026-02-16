#include "FontAtlas.h"

#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include FT_BITMAP_H
#include FT_BBOX_H

FontAtlas::FontAtlas(std::unique_ptr<FontFace> face, size_t atlasSize)
    : m_fontFace(std::move(face)),
      m_atlasSize(std::max(ATLAS_SIZE, atlasSize)),
      m_atlasBuffer(m_atlasSize, m_atlasSize, 1),
      m_rectanizer(m_atlasSize, m_atlasSize)
{
    m_texture = std::make_shared<Texture>(getWidth(), getHeight(), Texture::FORMAT_RED, 0, nullptr);
}

void FontAtlas::reset()
{
    m_atlasBuffer.clear();
    ++m_currentVersion;
    if (m_currentVersion >= 10)
    {
        if (m_atlasMap.size() < m_atlasMap.bucket_count() / 2)
            decltype(m_atlasMap){}.swap(m_atlasMap);
        else
            m_atlasMap.clear();
        m_currentVersion = 1;
    }
    m_rectanizer.reset();
    m_isDirty = false;
    m_texture = std::make_shared<Texture>(getWidth(), getHeight(), Texture::FORMAT_RED, 0, nullptr);
}

GlyphValue *FontAtlas::metrics(uint32_t codepoint, size_t pixelSize)
{
    GlyphKey key{codepoint, pixelSize};
    std::unordered_map<GlyphKey, GlyphValue, GlyphKeyHash>::iterator it = m_atlasMap.find(key);
    if (it != m_atlasMap.end())
        return &it->second;
    else
    {
        GlyphValue new_item;
        FT_GlyphSlot glyph_slot = loadCharFTGlyphSlot(codepoint, pixelSize);
        loadCharMetrics(glyph_slot, new_item);
        return &m_atlasMap.insert(std::make_pair(key, new_item)).first->second;
    }
}

GlyphValue *FontAtlas::glyph(uint32_t codepoint, size_t pixelSize)
{
    GlyphKey key{codepoint, pixelSize};
    std::unordered_map<GlyphKey, GlyphValue, GlyphKeyHash>::iterator it = m_atlasMap.find(key);
    if (it != m_atlasMap.end())
    {
        GlyphValue &cur_item = it->second;
        if (cur_item.versionUV == m_currentVersion)
            return &cur_item;
        FT_GlyphSlot glyph_slot = loadCharFTGlyphSlot(codepoint, pixelSize);
        if (!loadCharToAtlas(glyph_slot, cur_item))
            return nullptr;
        else
            return &cur_item;
    }
    else
    {
        FT_GlyphSlot glyph_slot = loadCharFTGlyphSlot(codepoint, pixelSize);
        GlyphValue new_item;
        loadCharMetrics(glyph_slot, new_item);
        if (!loadCharToAtlas(glyph_slot, new_item))
            return nullptr;
        else
            return &m_atlasMap.insert(std::make_pair(key, new_item)).first->second;
    }
}

void FontAtlas::syncTexture()
{
    if (m_isDirty)
    {
        m_texture->update(0, 0, getWidth(), getHeight(), bufferPointer());
        m_isDirty = false;
    }
}

FT_GlyphSlot FontAtlas::loadCharFTGlyphSlot(uint32_t codepoint, size_t pixelSize)
{
    if (m_pixelSize != pixelSize)
    {
        m_pixelSize = pixelSize;
        m_fontFace->setPixelSize(pixelSize);
    }
    m_fontFace->loadChar(codepoint);
    return m_fontFace->getGlyphSlot();
}

void FontAtlas::loadCharMetrics(FT_GlyphSlot glyph_slot, GlyphValue &out_item)
{
    FT_BBox bbox;
    if (FT_Outline_Get_BBox(&glyph_slot->outline, &bbox))
        throw std::runtime_error("Freetype error: FT_Outline_Get_BBox");
    int char_width = std::max(static_cast<int>((bbox.xMax - bbox.xMin + 63) >> 6), 1);
    int char_height = std::max(static_cast<int>((bbox.yMax - bbox.yMin + 63) >> 6), 1);

    out_item.width = char_width;
    out_item.height = char_height;
    out_item.bearingX = glyph_slot->metrics.horiBearingX >> 6;
    out_item.bearingY = glyph_slot->metrics.horiBearingY >> 6;
    out_item.advance = static_cast<float>(glyph_slot->advance.x) / 64.0f;
    out_item.bbox_xMin = bbox.xMin;
    out_item.bbox_yMin = bbox.yMin;
    out_item.versionUV = 0;
}

bool FontAtlas::loadCharToAtlas(FT_GlyphSlot glyph_slot, GlyphValue &out_item)
{
    if (glyph_slot->outline.n_points == 0)
    {
        out_item.versionUV = m_currentVersion;
        return true;
    }

    int char_width = out_item.width;
    int char_height = out_item.height;
    int padded_width = char_width + 2 * ATLAS_PADDING;
    int padded_height = char_height + 2 * ATLAS_PADDING;

    int pos_x, pos_y;
    if (!m_rectanizer.addRect(padded_width, padded_height, pos_x, pos_y))
    {
        out_item.versionUV = 0;
        return false;
    }

    int draw_x = pos_x + ATLAS_PADDING;
    int draw_y = pos_y + ATLAS_PADDING;

    FT_Bitmap bitmap;
    bitmap.rows = char_height;
    bitmap.width = char_width;
    bitmap.pitch = m_atlasBuffer.rowBytes();
    bitmap.buffer = &m_atlasBuffer.at<unsigned char>(draw_x, draw_y);
    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;

    FT_Raster_Params rasterParams;
    rasterParams.target = &bitmap;
    rasterParams.source = &glyph_slot->outline;
    rasterParams.flags = FT_RASTER_FLAG_AA;

    FT_Outline_Translate(&glyph_slot->outline, -out_item.bbox_xMin, -out_item.bbox_yMin);
    if (FT_Outline_Render(m_fontFace->getFTLibrary(), &glyph_slot->outline, &rasterParams))
        throw std::runtime_error("Freetype error: FT_Outline_Render");

    Bounds &textureUV = out_item.textureUV;
    textureUV.minx = static_cast<float>(draw_x) / getWidth();
    textureUV.miny = static_cast<float>(draw_y) / getHeight();
    textureUV.maxx = textureUV.minx + static_cast<float>(char_width) / getWidth();
    textureUV.maxy = textureUV.miny + static_cast<float>(char_height) / getHeight();

    m_isDirty = true;

    out_item.versionUV = m_currentVersion;
    return true;
}
