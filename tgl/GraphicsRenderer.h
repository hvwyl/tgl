#ifndef GRAPHICSRENDERER_H
#define GRAPHICSRENDERER_H

#include "RendererShader.h"
#include "RendererBuffer.h"

class GraphicsRecorder;

//
// GraphicsRenderer
//
class GraphicsRenderer
{
public:
    GraphicsRenderer();
    ~GraphicsRenderer() = default;

    inline void setResolution(size_t width, size_t height)
    {
        m_width = width;
        m_height = height;
    }

    void commit(const GraphicsRecorder &recorder);
    void render();

private:
    size_t m_width;
    size_t m_height;

    RendererShader m_shader;
    RendererBuffer m_buffer;

    std::vector<Call> m_calls;
};

#endif
