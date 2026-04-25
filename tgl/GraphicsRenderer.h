#ifndef GRAPHICSRENDERER_H
#define GRAPHICSRENDERER_H

// #define SHADER_GL_ES

#include "Shader.h"
#include "GraphicsStructs.h"
#include "OpenGLHeader.h"
#include <vector>
#include <cstddef>

class GraphicsRecorder;

//
// GraphicsRenderer
//
class GraphicsRenderer
{
public:
    GraphicsRenderer();
    ~GraphicsRenderer();

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

    // Shader
    Shader m_shader;
    struct ShaderLocs
    {
        // Attribs
        GLint a_pos;
        GLint a_uv0;
        GLint a_uv1;
        // Uniforms
        GLint u_resolution;
        GLint u_fragmentType;
        GLint u_alpha;
        GLint u_scissor;
        GLint u_color;
        GLint u_imageParams;
        GLint u_gradientParam0;
        GLint u_gradientParam1;
        // Samplers
        GLint u_texture;
        GLint u_fontAtlas;
    };
    ShaderLocs m_locs;

    // VBO & IBO & VAO
    GLuint m_vbo;
    size_t m_vboSize;
    GLuint m_ibo;
    size_t m_iboSize;
    GLuint m_vao;

    // Calls
    std::vector<Call> m_calls;
};

#endif
