#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "GraphicsShader.h"
#include "GraphicsBuffer.h"
#include "Image.h"
#include "Gradient.h"
#include "Font.h"
#include <stack>

//
// Graphics
//
class Graphics
{
public:
    Graphics();
    ~Graphics() = default;
    inline void setResolution(size_t width, size_t height)
    {
        m_width = width;
        m_height = height;
    }

    void save();
    void restore();

    enum CompositeOperation
    {
        COMPOSITE_SOURCE_OVER,
        COMPOSITE_SOURCE_IN,
        COMPOSITE_SOURCE_OUT,
        COMPOSITE_ATOP,
        COMPOSITE_DESTINATION_OVER,
        COMPOSITE_DESTINATION_IN,
        COMPOSITE_DESTINATION_OUT,
        COMPOSITE_DESTINATION_ATOP,
        COMPOSITE_LIGHTER,
        COMPOSITE_COPY,
        COMPOSITE_XOR,
    };
    void setCompositeOperation(CompositeOperation compositeOperation);
    void setCompositeGlobalAlpha(float alpha);
    void setFillColor(const Color &color);
    void setFillImage(const Image &image);
    void setFillLinearGradient(const Gradient &gradient, float x0, float y0, float x1, float y1);
    void setFillRadialGradient(const Gradient &gradient,
                               float x0, float y0, float r0, float x1, float y1, float r1);
    void setFillConicGradient(const Gradient &gradient, float startAngle, float x, float y);

    void setScissor(float x, float y, float width, float height);
    void unsetScissor();

    void fillRect(float x, float y, float width, float height);
    void fillRect(float x, float y, float width, float height, const Image::Clip &clip);
    void fillCircle(float x, float y, float width, float height);
    void fillCircle(float x, float y, float width, float height, const Image::Clip &clip);
    void fillImage(float dx, float dy, float scale = 1.0f);

    void setFontFamily(const Font &font);
    void setFontPixelSize(size_t pixelSize);
    void fillText(float x, float y, const std::string &utf8string);
    void fillText(float x, float y, const std::wstring &utf16string);

    struct TextMetrics
    {
        float width;
        float ascent;
        float descent;
    };
    TextMetrics measureText(const std::string &utf8string);
    TextMetrics measureText(const std::wstring &utf16string);

    void beginFrame();
    void flushFrame();

private:
    size_t m_width;
    size_t m_height;

    GraphicsShader m_shader;
    GraphicsBuffer m_buffer;

    // Param
    enum DrawType : uint32_t
    {
        DRAW_RECT = 0u,
        DRAW_CIRCLE = 1u,
        DRAW_FONT = 3u
    };
    struct CallParam
    {
        DrawType drawType;
        std::shared_ptr<Texture> fontAtlas = nullptr;
    };

    // State
    enum FillType : uint32_t
    {
        FILL_COLOR = 0u,
        FILL_IMAGE = 1u,
        FILL_LINEAR_GRADIENT = 2u,
        FILL_RADIAL_GRADIENT = 3u,
        FILL_CONIC_GRADIENT = 4u,
        FILL_NONE = 255u
    };
    struct CallState
    {
        /* BlendFunc */
        GLenum sfactor;
        GLenum dfactor;
        /* Uniforms */
        float alpha;
        Bounds scissor;
        FillType fillType;
        Color color;
        uint32_t imageParams;
        float gradientParam0[3];
        float gradientParam1[3];
        /* Textures */
        std::shared_ptr<Texture> texture = nullptr;
    };

    // Font
    struct FontState
    {
        std::shared_ptr<FontAtlas> atlas = nullptr;
        size_t pixelSize = 0;
    };
    FontState m_fontState;

    // State Stack
    struct State
    {
        CallState callState;
        FontState fontState;
    };
    std::stack<State> m_stateStack;

    // Calls
    struct Call
    {
        CallParam param;
        CallState state;
        void *indiceOffset;
        GLsizei indiceCount;
    };
    std::vector<Call> m_calls;
    Call *m_currentCall = nullptr;
    void switchToNewActiveCall();
    void switchToNewDrawTypeCall(DrawType drawType, bool extraCheck = true);

    void buildBounds(const Bounds &posb, const Bounds &uv0b, const Bounds &uv1b);
    void rectBounds(const Bounds &posb, const Bounds &uv0b);
    void circleBounds(const Bounds &posb, const Bounds &uv0b);
    void fontBounds(const Bounds &posb, const Bounds &uv0b, const Bounds &uv1b);
};

#endif
