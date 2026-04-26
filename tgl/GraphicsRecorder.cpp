#include "GraphicsRecorder.h"
#include "FontAtlas.h"
#include <locale>
#include <codecvt>

constexpr float FLOAT_EPSILON = 1e-6f;

GraphicsRecorder::GraphicsRecorder()
{
    // Initialize Calls
    Call call;
    // blend: source-over
    call.state.sfactor = GL_ONE;
    call.state.dfactor = GL_ONE_MINUS_SRC_ALPHA;
    // alpha: 1.0f
    call.state.alpha = 1.0f;
    // scissor: none
    call.state.scissor.minx = -std::numeric_limits<float>::infinity();
    call.state.scissor.miny = -std::numeric_limits<float>::infinity();
    call.state.scissor.maxx = +std::numeric_limits<float>::infinity();
    call.state.scissor.maxy = +std::numeric_limits<float>::infinity();
    // fill: none
    call.state.fillType = FILL_NONE;
    // draw: rect
    call.param.drawType = DRAW_RECT;
    // indice: 0, 0
    call.indiceOffset = reinterpret_cast<void *>(0);
    call.indiceCount = 0;
    m_currentCall = &m_calls.emplace_back(call);
}

void GraphicsRecorder::clear()
{
    Call call;
    call.state = m_currentCall->state;
    call.param = m_currentCall->param;
    call.indiceOffset = reinterpret_cast<void *>(0);
    call.indiceCount = 0;
    decltype(m_calls){}.swap(m_calls);
    m_currentCall = &m_calls.emplace_back(call);

    // Shrink state stack
    if (m_stateStack.size() < m_stateStack.capacity() / 4)
        m_stateStack.shrink_to_fit();

    // Clear buffers
    if (m_verts.size() < m_verts.capacity() / 4)
        decltype(m_verts){}.swap(m_verts);
    else
        m_verts.clear();

    if (m_indices.size() < m_indices.capacity() / 4)
        decltype(m_indices){}.swap(m_indices);
    else
        m_indices.clear();
}

void GraphicsRecorder::save()
{
    syncFontTexture();
    m_stateStack.push_back(State{m_currentCall->state, m_drawState});
}

void GraphicsRecorder::restore()
{
    if (!m_stateStack.empty())
    {
        switchToNewActiveCall();
        State &state = m_stateStack.back();
        m_currentCall->state = state.callState;
        m_drawState = state.drawState;
        m_stateStack.pop_back();
    }
}

void GraphicsRecorder::setCompositeOperation(CompositeOperation compositeOperation)
{
    GLenum sfactor = GL_ONE;
    GLenum dfactor = GL_ZERO;
    switch (compositeOperation)
    {
    case COMPOSITE_SOURCE_OVER:
        sfactor = GL_ONE;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;

    case COMPOSITE_SOURCE_IN:
        sfactor = GL_DST_ALPHA;
        dfactor = GL_ZERO;
        break;

    case COMPOSITE_SOURCE_OUT:
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_ZERO;
        break;

    case COMPOSITE_ATOP:
        sfactor = GL_DST_ALPHA;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;

    case COMPOSITE_DESTINATION_OVER:
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_ONE;
        break;

    case COMPOSITE_DESTINATION_IN:
        sfactor = GL_ZERO;
        dfactor = GL_SRC_ALPHA;
        break;

    case COMPOSITE_DESTINATION_OUT:
        sfactor = GL_ZERO;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;

    case COMPOSITE_DESTINATION_ATOP:
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_SRC_ALPHA;
        break;

    case COMPOSITE_LIGHTER:
        sfactor = GL_ONE;
        dfactor = GL_ONE;
        break;

    case COMPOSITE_COPY:
        sfactor = GL_ONE;
        dfactor = GL_ZERO;
        break;

    case COMPOSITE_XOR:
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;

    default:
        sfactor = GL_ONE;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    }
    if (m_currentCall->state.sfactor != sfactor || m_currentCall->state.dfactor != dfactor)
    {
        switchToNewActiveCall();
        m_currentCall->state.sfactor = sfactor;
        m_currentCall->state.dfactor = dfactor;
    }
}

void GraphicsRecorder::setCompositeGlobalAlpha(float alpha)
{
    if (std::abs(m_currentCall->state.alpha - alpha) > FLOAT_EPSILON)
    {
        switchToNewActiveCall();
        m_currentCall->state.alpha = alpha;
    }
}

void GraphicsRecorder::setFillColor(const Color &color)
{
    Color premultipliedColor = Color::premulColor(color);
    if (m_currentCall->state.fillType != FILL_COLOR ||
        std::abs(m_currentCall->state.color.r - premultipliedColor.r) > FLOAT_EPSILON ||
        std::abs(m_currentCall->state.color.g - premultipliedColor.g) > FLOAT_EPSILON ||
        std::abs(m_currentCall->state.color.b - premultipliedColor.b) > FLOAT_EPSILON ||
        std::abs(m_currentCall->state.color.a - premultipliedColor.a) > FLOAT_EPSILON)
    {
        switchToNewActiveCall();
        m_currentCall->state.fillType = FILL_COLOR;
        m_currentCall->state.color = premultipliedColor;
    }
}

void GraphicsRecorder::setFillImage(const Image &image)
{
    if (!image.isValid())
        return;
    uint32_t imageParams = (image.m_layout << 16) | image.m_flags;
    if (m_currentCall->state.fillType != FILL_IMAGE ||
        m_currentCall->state.texture != image.m_texture ||
        m_currentCall->state.imageParams != imageParams)
    {
        switchToNewActiveCall();
        m_currentCall->state.fillType = FILL_IMAGE;
        m_currentCall->state.imageParams = imageParams;
        m_currentCall->state.texture = image.m_texture;
        m_drawState.imageClip = Image::CLIP_NONE;
    }
}

void GraphicsRecorder::setFillImageClip(const Image::Clip &clip)
{
    m_drawState.imageClip = clip;
}

void GraphicsRecorder::setFillLinearGradient(const Gradient &gradient, float x0, float y0, float x1, float y1)
{
    if (!gradient.isValid())
        return;
    if (m_currentCall->state.fillType != FILL_LINEAR_GRADIENT ||
        m_currentCall->state.texture != gradient.m_texture ||
        std::fabs(m_currentCall->state.gradientParam0[0] - x0) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam0[1] - y0) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam1[0] - x1) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam1[1] - y1) > FLOAT_EPSILON)
    {
        switchToNewActiveCall();
        m_currentCall->state.fillType = FILL_LINEAR_GRADIENT;
        m_currentCall->state.gradientParam0[0] = x0;
        m_currentCall->state.gradientParam0[1] = y0;
        m_currentCall->state.gradientParam1[0] = x1;
        m_currentCall->state.gradientParam1[1] = y1;
        m_currentCall->state.texture = gradient.m_texture;
    }
}

void GraphicsRecorder::setFillRadialGradient(const Gradient &gradient,
                                             float x0, float y0, float r0,
                                             float x1, float y1, float r1)
{
    if (!gradient.isValid())
        return;
    if (m_currentCall->state.fillType != FILL_RADIAL_GRADIENT ||
        m_currentCall->state.texture != gradient.m_texture ||
        std::fabs(m_currentCall->state.gradientParam0[0] - x0) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam0[1] - y0) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam0[2] - r0) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam1[0] - x1) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam1[1] - y1) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam1[2] - r1) > FLOAT_EPSILON)
    {
        switchToNewActiveCall();
        m_currentCall->state.fillType = FILL_RADIAL_GRADIENT;
        m_currentCall->state.gradientParam0[0] = x0;
        m_currentCall->state.gradientParam0[1] = y0;
        m_currentCall->state.gradientParam0[2] = r0;
        m_currentCall->state.gradientParam1[0] = x1;
        m_currentCall->state.gradientParam1[1] = y1;
        m_currentCall->state.gradientParam1[2] = r1;
        m_currentCall->state.texture = gradient.m_texture;
    }
}

void GraphicsRecorder::setFillConicGradient(const Gradient &gradient, float startAngle, float x, float y)
{
    if (!gradient.isValid())
        return;
    if (m_currentCall->state.fillType != FILL_CONIC_GRADIENT ||
        m_currentCall->state.texture != gradient.m_texture ||
        std::fabs(m_currentCall->state.gradientParam0[0] - x) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam0[1] - y) > FLOAT_EPSILON ||
        std::fabs(m_currentCall->state.gradientParam0[2] - startAngle) > FLOAT_EPSILON)
    {
        switchToNewActiveCall();
        m_currentCall->state.fillType = FILL_CONIC_GRADIENT;
        m_currentCall->state.gradientParam0[0] = x;
        m_currentCall->state.gradientParam0[1] = y;
        m_currentCall->state.gradientParam0[2] = startAngle;
        m_currentCall->state.texture = gradient.m_texture;
    }
}

void GraphicsRecorder::setScissor(float x, float y, float width, float height)
{
    switchToNewActiveCall();
    m_currentCall->state.scissor.minx = x;
    m_currentCall->state.scissor.miny = y;
    m_currentCall->state.scissor.maxx = x + width;
    m_currentCall->state.scissor.maxy = y + height;
}

void GraphicsRecorder::unsetScissor()
{
    switchToNewActiveCall();
    m_currentCall->state.scissor.minx = -std::numeric_limits<float>::infinity();
    m_currentCall->state.scissor.miny = -std::numeric_limits<float>::infinity();
    m_currentCall->state.scissor.maxx = +std::numeric_limits<float>::infinity();
    m_currentCall->state.scissor.maxy = +std::numeric_limits<float>::infinity();
}

void GraphicsRecorder::drawRect(float x, float y, float width, float height)
{
    const Bounds posb{x, y, x + width, y + height};
    rectBounds(posb, m_drawState.imageClip.uv0b);
}

void GraphicsRecorder::drawCircle(float x, float y, float width, float height)
{
    const Bounds posb{x, y, x + width, y + height};
    circleBounds(posb, m_drawState.imageClip.uv0b);
}

void GraphicsRecorder::drawCircle(float x, float y, float radius)
{
    const Bounds posb{x - radius, y - radius, x + radius, y + radius};
    circleBounds(posb, m_drawState.imageClip.uv0b);
}

void GraphicsRecorder::drawImage(float dx, float dy, float scale)
{
    CallState &state = m_currentCall->state;
    if (state.fillType != FILL_IMAGE)
        return;

    // bounding box
    float dWidth = state.texture->getWidth() * scale;
    float dHeight = state.texture->getHeight() * scale;
    const Bounds posb{dx, dy, dx + dWidth, dy + dHeight};
    const Bounds uv0b{0.0f, 0.0f, 1.0f, 1.0f};
    rectBounds(posb, uv0b);
}

void GraphicsRecorder::setFontFamily(const Font &font)
{
    syncFontTexture();
    m_drawState.fontAtlas = font.m_atlas;
}

void GraphicsRecorder::setFontPixelSize(size_t pixelSize)
{
    if (m_drawState.fontAtlas == nullptr)
        return;
    if (pixelSize > m_drawState.fontAtlas->maxPixelSize())
        pixelSize = m_drawState.fontAtlas->maxPixelSize();
    m_drawState.fontPixelSize = pixelSize;
}

void GraphicsRecorder::drawText(float x, float y, const std::string &utf8string)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return drawText(x, y, converter.from_bytes(utf8string));
}

void GraphicsRecorder::drawText(float x, float y, const std::wstring &utf16string)
{
    if (m_drawState.fontAtlas == nullptr)
        return;
    FontAtlas &atlas = *(m_drawState.fontAtlas);
    for (wchar_t ch : utf16string)
    {
        GlyphValue *glyph = m_drawState.fontAtlas->glyph(ch, m_drawState.fontPixelSize);
        if (glyph == nullptr)
        {
            atlas.syncTexture();
            atlas.reset();
            glyph = m_drawState.fontAtlas->glyph(ch, m_drawState.fontPixelSize);
        }
        const float baseX = x + glyph->bearingX;
        const float baseY = y - glyph->bearingY;
        const Bounds posb{baseX, baseY, baseX + glyph->width, baseY + glyph->height};
        fontBounds(posb, m_drawState.imageClip.uv0b, glyph->textureUV);
        x += glyph->advance;
    }
}

GraphicsRecorder::TextMetrics GraphicsRecorder::measureText(const std::string &utf8string)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return measureText(converter.from_bytes(utf8string));
}

GraphicsRecorder::TextMetrics GraphicsRecorder::measureText(const std::wstring &utf16string)
{
    if (m_drawState.fontAtlas == nullptr)
        return TextMetrics{};
    FontAtlas &atlas = *(m_drawState.fontAtlas);
    float width = 0.0f;
    float ascent = 0.0f;
    float descent = 0.0f;
    for (wchar_t ch : utf16string)
    {
        GlyphValue *glyph = m_drawState.fontAtlas->metrics(ch, m_drawState.fontPixelSize);
        width += glyph->advance;
        ascent = std::max(ascent, static_cast<float>(glyph->bearingY));
        descent = std::max(descent, static_cast<float>(glyph->height - glyph->bearingY));
    }
    return TextMetrics{width, ascent, descent};
}

void GraphicsRecorder::switchToNewActiveCall()
{
    if (m_currentCall->indiceCount > 0)
    {
        Call call;
        call.state = m_currentCall->state;
        call.param = m_currentCall->param;
        call.indiceOffset = reinterpret_cast<void *>(m_indices.size() * sizeof(GLuint));
        call.indiceCount = 0;
        m_currentCall = &m_calls.emplace_back(call);
    }
}

void GraphicsRecorder::switchToNewDrawTypeCall(DrawType drawType, bool extraCheck)
{
    if (m_currentCall->param.drawType != drawType || extraCheck)
    {
        switchToNewActiveCall();
        m_currentCall->param.drawType = drawType;
    }
}

void GraphicsRecorder::buildGeomBounds(const Bounds &posb, const Bounds &uv0b)
{
    const float exp_x = (posb.maxx - posb.minx) * 0.125f;
    const float exp_y = (posb.maxy - posb.miny) * 0.125f;
    const Bounds expb{posb.minx - exp_x, posb.miny - exp_y, posb.maxx + exp_x, posb.maxy + exp_y};

    const size_t base = m_verts.size();
    m_verts.insert(m_verts.end(),
                   {{Point{posb.minx, posb.miny}, Point{uv0b.minx, uv0b.miny}, Point{0.0f - 0.000f, 0.0f - 0.000f}},
                    {Point{posb.minx, posb.maxy}, Point{uv0b.minx, uv0b.maxy}, Point{0.0f - 0.000f, 1.0f + 0.000f}},
                    {Point{posb.maxx, posb.maxy}, Point{uv0b.maxx, uv0b.maxy}, Point{1.0f + 0.000f, 1.0f + 0.000f}},
                    {Point{posb.maxx, posb.miny}, Point{uv0b.maxx, uv0b.miny}, Point{1.0f + 0.000f, 0.0f - 0.000f}},
                    {Point{expb.minx, posb.miny}, Point{uv0b.minx, uv0b.miny}, Point{0.0f - 0.125f, 0.0f - 0.000f}},
                    {Point{expb.minx, posb.maxy}, Point{uv0b.minx, uv0b.maxy}, Point{0.0f - 0.125f, 1.0f + 0.000f}},
                    {Point{posb.minx, expb.maxy}, Point{uv0b.minx, uv0b.maxy}, Point{0.0f - 0.000f, 1.0f + 0.125f}},
                    {Point{posb.maxx, expb.maxy}, Point{uv0b.maxx, uv0b.maxy}, Point{1.0f + 0.000f, 1.0f + 0.125f}},
                    {Point{expb.maxx, posb.maxy}, Point{uv0b.maxx, uv0b.maxy}, Point{1.0f + 0.125f, 1.0f + 0.000f}},
                    {Point{expb.maxx, posb.miny}, Point{uv0b.maxx, uv0b.miny}, Point{1.0f + 0.125f, 0.0f - 0.000f}},
                    {Point{posb.maxx, expb.miny}, Point{uv0b.maxx, uv0b.miny}, Point{1.0f + 0.000f, 0.0f - 0.125f}},
                    {Point{posb.minx, expb.miny}, Point{uv0b.minx, uv0b.miny}, Point{0.0f - 0.000f, 0.0f - 0.125f}}});
    m_indices.insert(m_indices.end(), {base + 0, base + 1, base + 2, base + 0, base + 2, base + 3,
                                       base + 0, base + 4, base + 5, base + 0, base + 5, base + 1,
                                       base + 1, base + 6, base + 7, base + 1, base + 7, base + 2,
                                       base + 2, base + 8, base + 9, base + 2, base + 9, base + 3,
                                       base + 3, base + 10, base + 11, base + 3, base + 11, base + 0,
                                       base + 4, base + 0, base + 11, base + 6, base + 1, base + 5,
                                       base + 8, base + 2, base + 7, base + 10, base + 3, base + 9});
    m_currentCall->indiceCount += 42;
}

void GraphicsRecorder::buildFontBounds(const Bounds &posb, const Bounds &uv0b, const Bounds &uv1b)
{
    const size_t base = m_verts.size();
    m_verts.insert(m_verts.end(),
                   {{Point{posb.minx, posb.miny}, Point{uv0b.minx, uv0b.miny}, Point{uv1b.minx, uv1b.miny}},
                    {Point{posb.minx, posb.maxy}, Point{uv0b.minx, uv0b.maxy}, Point{uv1b.minx, uv1b.maxy}},
                    {Point{posb.maxx, posb.maxy}, Point{uv0b.maxx, uv0b.maxy}, Point{uv1b.maxx, uv1b.maxy}},
                    {Point{posb.maxx, posb.miny}, Point{uv0b.maxx, uv0b.miny}, Point{uv1b.maxx, uv1b.miny}}});
    m_indices.insert(m_indices.end(), {base + 0, base + 1, base + 2, base + 0, base + 2, base + 3});
    m_currentCall->indiceCount += 6;
}

void GraphicsRecorder::rectBounds(const Bounds &posb, const Bounds &uv0b)
{
    switchToNewDrawTypeCall(DRAW_RECT);
    buildGeomBounds(posb, uv0b);
}

void GraphicsRecorder::circleBounds(const Bounds &posb, const Bounds &uv0b)
{
    switchToNewDrawTypeCall(DRAW_CIRCLE);
    buildGeomBounds(posb, uv0b);
}

void GraphicsRecorder::fontBounds(const Bounds &posb, const Bounds &uv0b, const Bounds &uv1b)
{
    switchToNewDrawTypeCall(DRAW_FONT, m_currentCall->param.fontTexture != m_drawState.fontAtlas->getTexture());
    m_currentCall->param.fontTexture = m_drawState.fontAtlas->getTexture();
    buildFontBounds(posb, uv0b, uv1b);
}

void GraphicsRecorder::syncFontTexture() const
{
    if (m_drawState.fontAtlas != nullptr)
        m_drawState.fontAtlas->syncTexture();
}
