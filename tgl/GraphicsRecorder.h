#ifndef GRAPHICSRECORDER_H
#define GRAPHICSRECORDER_H

#include "Image.h"
#include "Gradient.h"
#include "Font.h"

#include "GraphicsStructs.h"

//
// GraphicsRecorder
//
class GraphicsRecorder
{
public:
    GraphicsRecorder();
    ~GraphicsRecorder() = default;

    void clear();

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
    void setFillImageClip(const Image::Clip &clip = Image::CLIP_NONE);
    void setFillLinearGradient(const Gradient &gradient, float x0, float y0, float x1, float y1);
    void setFillRadialGradient(const Gradient &gradient,
                               float x0, float y0, float r0, float x1, float y1, float r1);
    void setFillConicGradient(const Gradient &gradient, float startAngle, float x, float y);

    void setScissor(float x, float y, float width, float height);
    void unsetScissor();

    void drawRect(float x, float y, float width, float height);
    void drawImage(float dx, float dy, float scale = 1.0f);

    void setFontFamily(const Font &font);
    void setFontPixelSize(size_t pixelSize);
    void drawText(float x, float y, const std::string &utf8string);
    void drawText(float x, float y, const std::wstring &utf16string);

    struct TextMetrics
    {
        float width;
        float ascent;
        float descent;
    };
    TextMetrics measureText(const std::string &utf8string);
    TextMetrics measureText(const std::wstring &utf16string);

    inline bool isEmpty() const
    {
        return m_currentCall->indiceCount == 0 && m_calls.size() == 1;
    }

private:
    // Vertices & Indices
    std::vector<Vertex> m_verts;
    std::vector<GLuint> m_indices;

    // Draw State
    struct DrawState
    {
        Image::Clip imageClip = Image::CLIP_NONE;
        std::shared_ptr<FontAtlas> fontAtlas = nullptr;
        size_t fontPixelSize = 0;
    };
    DrawState m_drawState;

    // State Stack
    struct State
    {
        CallState callState;
        DrawState drawState;
    };
    std::vector<State> m_stateStack;

    // Calls
    std::vector<Call> m_calls;
    Call *m_currentCall = nullptr;
    void switchToNewActiveCall();
    void switchToNewDrawTypeCall(DrawType drawType, bool extraCheck = false);

    void buildRectBounds(const Bounds &posb, const Bounds &uv0b);
    void buildFontBounds(const Bounds &posb, const Bounds &uv0b, const Bounds &uv1b);

    void syncFontTexture() const;

    friend class GraphicsRenderer;
};

#endif
