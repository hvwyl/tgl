#ifndef GRAPHICSSHADER_H
#define GRAPHICSSHADER_H

// #define SHADER_GL_ES

#include "Shader.h"

//
// GraphicsShader
//
class GraphicsShader : public Shader
{
public:
    GraphicsShader();
    ~GraphicsShader() = default;

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
    ShaderLocs locs;
};

#endif
