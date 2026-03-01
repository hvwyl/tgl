#include "Graphics.h"
#include "FontAtlas.h"
#include <locale>
#include <codecvt>
#include <cstdio>

constexpr float FLOAT_EPSILON = 1e-6f;

Graphics::Graphics()
{
    if (m_shader.isValid() == 0)
    {
        std::printf("Shader compilation failed\n");
    }

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

    // Initialize blend ( Alpha blend, PREMULTIPLIED Shader )
    glEnable(GL_BLEND);
#if 0
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#endif
    // Initialize depth
    glDisable(GL_DEPTH_TEST);
    // Initialize stencil
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
    // Initialize texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Graphics::save()
{
    if (m_fontState.atlas != nullptr)
        m_fontState.atlas->syncTexture();
    m_stateStack.emplace(State{m_currentCall->state, m_fontState});
}

void Graphics::restore()
{
    if (!m_stateStack.empty())
    {
        switchToNewActiveCall();
        State &state = m_stateStack.top();
        m_currentCall->state = state.callState;
        m_fontState = state.fontState;
        m_stateStack.pop();
    }
}

void Graphics::setCompositeOperation(CompositeOperation compositeOperation)
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

void Graphics::setCompositeGlobalAlpha(float alpha)
{
    if (std::abs(m_currentCall->state.alpha - alpha) > FLOAT_EPSILON)
    {
        switchToNewActiveCall();
        m_currentCall->state.alpha = alpha;
    }
}

void Graphics::setFillColor(const Color &color)
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

void Graphics::setFillImage(const Image &image)
{
    uint32_t imageParams = (image.m_layout << 16) | image.m_flags;
    if (m_currentCall->state.fillType != FILL_IMAGE ||
        m_currentCall->state.texture != image.m_texture ||
        m_currentCall->state.imageParams != imageParams)
    {
        switchToNewActiveCall();
        m_currentCall->state.fillType = FILL_IMAGE;
        m_currentCall->state.imageParams = imageParams;
        m_currentCall->state.texture = image.m_texture;
    }
}

void Graphics::setFillLinearGradient(const Gradient &gradient, float x0, float y0, float x1, float y1)
{

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

void Graphics::setFillRadialGradient(const Gradient &gradient,
                                     float x0, float y0, float r0,
                                     float x1, float y1, float r1)
{
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

void Graphics::setFillConicGradient(const Gradient &gradient, float startAngle, float x, float y)
{
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

void Graphics::setScissor(float x, float y, float width, float height)
{

    switchToNewActiveCall();
    m_currentCall->state.scissor.minx = x;
    m_currentCall->state.scissor.miny = y;
    m_currentCall->state.scissor.maxx = x + width;
    m_currentCall->state.scissor.maxy = y + height;
}

void Graphics::unsetScissor()
{
    switchToNewActiveCall();
    m_currentCall->state.scissor.minx = -std::numeric_limits<float>::infinity();
    m_currentCall->state.scissor.miny = -std::numeric_limits<float>::infinity();
    m_currentCall->state.scissor.maxx = +std::numeric_limits<float>::infinity();
    m_currentCall->state.scissor.maxy = +std::numeric_limits<float>::infinity();
}

void Graphics::fillRect(float x, float y, float width, float height)
{
    const Bounds posb{x, y, x + width, y + height};
    const Bounds uv0b{0.0f, 0.0f, 1.0f, 1.0f};
    rectBounds(posb, uv0b);
}

void Graphics::fillRect(float x, float y, float width, float height, const Image::Clip &clip)
{
    CallState &state = m_currentCall->state;
    if (state.fillType != FILL_IMAGE)
        return;

    const Bounds posb{x, y, x + width, y + height};
    rectBounds(posb, clip.uv0b);
}

void Graphics::fillCircle(float x, float y, float width, float height)
{
    const Bounds posb{x, y, x + width, y + height};
    const Bounds uv0b{0.0f, 0.0f, 1.0f, 1.0f};
    circleBounds(posb, uv0b);
}

void Graphics::fillCircle(float x, float y, float width, float height, const Image::Clip &clip)
{
    CallState &state = m_currentCall->state;
    if (state.fillType != FILL_IMAGE)
        return;

    const Bounds posb{x, y, x + width, y + height};
    circleBounds(posb, clip.uv0b);
}

void Graphics::fillImage(float dx, float dy, float scale)
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

void Graphics::setFontFamily(const Font &font)
{
    if (m_fontState.atlas != nullptr)
        m_fontState.atlas->syncTexture();
    m_fontState.atlas = font.m_atlas;
}

void Graphics::setFontPixelSize(size_t pixelSize)
{
    if (m_fontState.atlas == nullptr)
        return;
    if (pixelSize > m_fontState.atlas->maxPixelSize())
        pixelSize = m_fontState.atlas->maxPixelSize();
    m_fontState.pixelSize = pixelSize;
}

void Graphics::fillText(float x, float y, const std::string &utf8string)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return fillText(x, y, converter.from_bytes(utf8string));
}

void Graphics::fillText(float x, float y, const std::wstring &utf16string)
{
    if (m_fontState.atlas == nullptr)
        return;
    FontAtlas &atlas = *(m_fontState.atlas);
    for (wchar_t ch : utf16string)
    {
        GlyphValue *glyph = m_fontState.atlas->glyph(ch, m_fontState.pixelSize);
        if (glyph == nullptr)
        {
            atlas.syncTexture();
            atlas.reset();
            glyph = m_fontState.atlas->glyph(ch, m_fontState.pixelSize);
        }
        const float baseX = x + glyph->bearingX;
        const float baseY = y - glyph->bearingY;
        const Bounds posb{baseX, baseY, baseX + glyph->width, baseY + glyph->height};
        const Bounds uv0b{0.0f, 0.0f, 1.0f, 1.0f};
        fontBounds(posb, uv0b, glyph->textureUV);
        x += glyph->advance;
    }
}

Graphics::TextMetrics Graphics::measureText(const std::string &utf8string)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return measureText(converter.from_bytes(utf8string));
}

Graphics::TextMetrics Graphics::measureText(const std::wstring &utf16string)
{
    if (m_fontState.atlas == nullptr)
        return TextMetrics{};
    FontAtlas &atlas = *(m_fontState.atlas);
    float width = 0.0f;
    float ascent = 0.0f;
    float descent = 0.0f;
    for (wchar_t ch : utf16string)
    {
        GlyphValue *glyph = m_fontState.atlas->metrics(ch, m_fontState.pixelSize);
        width += glyph->advance;
        ascent = std::max(ascent, static_cast<float>(glyph->bearingY));
        descent = std::max(descent, static_cast<float>(glyph->height - glyph->bearingY));
    }
    return TextMetrics{width, ascent, descent};
}

void Graphics::beginFrame()
{
    Call call;
    call.state = m_currentCall->state;
    call.param = m_currentCall->param;
    call.indiceOffset = reinterpret_cast<void *>(0);
    call.indiceCount = 0;
    decltype(m_calls){}.swap(m_calls);
    m_currentCall = &m_calls.emplace_back(call);

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

void Graphics::flushFrame()
{
    if (!(m_currentCall->indiceCount == 0 && m_calls.size() == 1))
    {
        // Upload font
        if (m_fontState.atlas != nullptr)
            m_fontState.atlas->syncTexture();

        // Upload vertices
        m_buffer.sync(m_shader.locs.a_pos, m_shader.locs.a_uv0, m_shader.locs.a_uv1, m_verts, m_indices);

        // Render
        m_shader.bind();
        m_buffer.bindVAO();
        glUniform2f(m_shader.locs.u_resolution, static_cast<float>(m_width), static_cast<float>(m_height));
        glUniform1i(m_shader.locs.u_texture, 0);
        glUniform1i(m_shader.locs.u_fontAtlas, 1);

        for (const Call &call : m_calls)
        {
            glBlendFuncSeparate(call.state.sfactor, call.state.dfactor,
                                call.state.sfactor, call.state.dfactor);
            glUniform2ui(m_shader.locs.u_fragmentType, call.state.fillType, call.param.drawType);
            glUniform1f(m_shader.locs.u_alpha, call.state.alpha);
            glUniform4f(m_shader.locs.u_scissor,
                        call.state.scissor.minx, call.state.scissor.miny,
                        call.state.scissor.maxx, call.state.scissor.maxy);
            switch (call.state.fillType)
            {
            case FILL_COLOR:
            {
                // fillColorPass
                glUniform4f(m_shader.locs.u_color,
                            call.state.color.r, call.state.color.g,
                            call.state.color.b, call.state.color.a);
                break;
            }

            case FILL_IMAGE:
            {
                // fillImagePass
                glUniform1ui(m_shader.locs.u_imageParams, call.state.imageParams);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, call.state.texture->getTex());
                break;
            }

            case FILL_LINEAR_GRADIENT:
            case FILL_RADIAL_GRADIENT:
            case FILL_CONIC_GRADIENT:
            {
                // fillGradientPass
                glUniform3f(m_shader.locs.u_gradientParam0,
                            call.state.gradientParam0[0], call.state.gradientParam0[1], call.state.gradientParam0[2]);
                glUniform3f(m_shader.locs.u_gradientParam1,
                            call.state.gradientParam1[0], call.state.gradientParam1[1], call.state.gradientParam1[2]);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, call.state.texture->getTex());
                break;
            }

            default:
                break;
            }
            switch (call.param.drawType)
            {
            case DRAW_FONT:
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, call.param.fontAtlas->getTex());
                break;

            default:
                break;
            }
            glDrawElements(GL_TRIANGLES, call.indiceCount, GL_UNSIGNED_INT, call.indiceOffset);
        }
    }
}

void Graphics::switchToNewActiveCall()
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

void Graphics::switchToNewDrawTypeCall(DrawType drawType, bool extraCheck)
{
    if (m_currentCall->param.drawType != drawType || extraCheck)
    {
        switchToNewActiveCall();
        m_currentCall->param.drawType = drawType;
    }
}

void Graphics::buildBounds(const Bounds &posb, const Bounds &uv0b, const Bounds &uv1b)
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

void Graphics::rectBounds(const Bounds &posb, const Bounds &uv0b)
{
    switchToNewDrawTypeCall(DRAW_RECT);
    const Bounds uv1b{0.0f, 0.0f, 1.0f, 1.0f};
    buildBounds(posb, uv0b, uv1b);
}

void Graphics::circleBounds(const Bounds &posb, const Bounds &uv0b)
{
    switchToNewDrawTypeCall(DRAW_CIRCLE);
    const Bounds uv1b{0.0f, 0.0f, 1.0f, 1.0f};
    buildBounds(posb, uv0b, uv1b);
}

void Graphics::fontBounds(const Bounds &posb, const Bounds &uv0b, const Bounds &uv1b)
{
    switchToNewDrawTypeCall(DRAW_FONT, m_currentCall->param.fontAtlas != m_fontState.atlas->getTexture());
    m_currentCall->param.fontAtlas = m_fontState.atlas->getTexture();
    buildBounds(posb, uv0b, uv1b);
}
