#include "GraphicsRenderer.h"
#include "GraphicsRecorder.h"
#include <cstdio>

static constexpr const char *default_header =
#ifdef SHADER_GL_ES
    "#version 300 es\nprecision highp float;\0";
#else
    "#version 330 core\0";
#endif

static constexpr const char *default_vshader = R"(
in vec2 a_pos;
in vec2 a_uv0;
in vec2 a_uv1;

out vec2 v_pos;
out vec2 v_uv0;
out vec2 v_uv1;

/* Uniforms */
uniform vec2 u_resolution;

/* VertShaders */
void main()
{
    v_pos = a_pos;
    v_uv0 = a_uv0;
    v_uv1 = a_uv1;
    gl_Position = vec4(2.0 * v_pos.x / u_resolution.x - 1.0, 1.0 - 2.0 * v_pos.y / u_resolution.y, 0.0, 1.0);
}
)";

static constexpr const char *default_fshader = R"(
in vec2 v_pos;
in vec2 v_uv0;
in vec2 v_uv1;

/* Uniforms */
uniform vec2 u_resolution;
uniform uvec2 u_fragmentType;
uniform float u_alpha;

// Scissor Purposes
uniform vec4 u_scissor;

// Filling Purposes
uniform vec4 u_color;
uniform uint u_imageParams;
uniform vec3 u_gradientParam0;
uniform vec3 u_gradientParam1;

/* Samplers */
uniform sampler2D u_texture;
uniform sampler2D u_fontAtlas;

/* FragShaders */
#define LAYOUT_RGB888 0u
#define LAYOUT_BGR888 1u
#define LAYOUT_RGBA8888 2u
#define LAYOUT_BGRA8888 3u
#define LAYOUT_Lum8 4u
#define LAYOUT_Alpha8 5u
#define LAYOUT_LumAlpha88 6u
#define FLAG_FLIP_X (1u<<0)
#define FLAG_FLIP_Y (1u<<1)
#define FLAG_PREMULTIPLIED (1u<<2)

vec4 imageColor(vec2 uv0)
{
    if (bool(u_imageParams & FLAG_FLIP_X)) uv0.x = 1.0 - uv0.x;
    if (bool(u_imageParams & FLAG_FLIP_Y)) uv0.y = 1.0 - uv0.y;
    vec4 color = texture(u_texture, uv0);
    uint pixelLayout = u_imageParams >> 16;
    switch (pixelLayout)
    {
    case LAYOUT_RGB888:
    case LAYOUT_RGBA8888:
        color = color.rgba;
        break;
    case LAYOUT_BGR888:
    case LAYOUT_BGRA8888:
        color = color.bgra;
        break;
    case LAYOUT_Lum8:
        color = vec4(color.r, color.r, color.r, 1.0);
        break;
    case LAYOUT_Alpha8:
        color = vec4(1.0, 1.0, 1.0, color.r);
        break;
    case LAYOUT_LumAlpha88:
        color = vec4(color.r, color.r, color.r, color.g);
        break;
    }
    if (!bool(u_imageParams & FLAG_PREMULTIPLIED))
        color.rgb *= color.a;
    return color;
}

vec4 linearGradientColor(vec2 pos)
{
    vec2 v0 = pos - u_gradientParam0.xy;
    vec2 v1 = u_gradientParam1.xy - u_gradientParam0.xy;
    float t = clamp(dot(v0, v1) / dot(v1, v1), 0.0, 1.0);
    return texture(u_texture, vec2(t, 0.0));
}

vec4 radialGradientColor(vec2 pos)
{
    float d0 = length(pos - u_gradientParam0.xy);
    float d1 = length(pos - u_gradientParam1.xy);
    float offset0 = d0 - u_gradientParam0.z;
    float offset1 = d1 - u_gradientParam1.z;
    float t = clamp(offset0 / (offset0 - offset1), 0.0, 1.0);
    return texture(u_texture, vec2(t, 0.0));
}

vec4 conicGradientColor(vec2 pos)
{
    vec2 v0 = pos - u_gradientParam0.xy;
    float angle = mod(radians(180.0) - atan(v0.y, -v0.x) - u_gradientParam0.z, radians(360.0));
    return texture(u_texture, vec2(angle / radians(360.0), 0.0));
}

////////////////////////////////

float sdfAA(float sdf)
{
    return 1.0 - smoothstep(-1.0, 1.0, sdf / max(fwidth(sdf), 0.00001));
}

float sdRect(vec2 uv1)
{
    vec2 dist = abs(uv1) - vec2(0.5);
    float sdf = length(max(dist, 0.0)) + min(max(dist.x, dist.y), 0.0);
    return sdfAA(sdf);
}

float sdCircle(vec2 uv1)
{
    float sdf = length(uv1) - 0.5;
    return sdfAA(sdf);
}

float sdScissor(vec2 pmin, vec2 pmax) {
    vec2 dist = vec2(
        min(v_pos.x - pmin.x, pmax.x - v_pos.x),
        min(v_pos.y - pmin.y, pmax.y - v_pos.y)
    );
    float sdf = -min(dist.x, dist.y);
    return sdfAA(sdf);
}

////////////////////////////////

void main()
{
    float scissorMask = sdScissor(u_scissor.xy, u_scissor.zw);
    if (scissorMask < 0.05)
        discard;
    vec4 resultColor = vec4(0.0);

    // Filling
    switch(u_fragmentType.x)
    {
    case 0u: // Color
        resultColor = u_color;
        break;
    case 1u: // Image
        resultColor = imageColor(v_uv0);
        break;
    case 2u: // Linear Gradient
        resultColor = linearGradientColor(v_pos);
        break;
    case 3u: // Radial Gradient
        resultColor = radialGradientColor(v_pos);
        break;
    case 4u: // Conic Gradient
        resultColor = conicGradientColor(v_pos);
        break;
    }

    // Drawing
    switch(u_fragmentType.y)
    {
    case 0u: // Rect
        resultColor *= sdRect(v_uv1 - vec2(0.5));
        break;
    case 1u: // Circle
        resultColor *= sdCircle(v_uv1 - vec2(0.5));
        break;
    case 3u: // Font
        resultColor *= texture(u_fontAtlas, v_uv1).r;
        break;
    }

    // Scissoring
    resultColor *= scissorMask;

    // Alpha Blending
    resultColor *= u_alpha;

    // Output
    gl_FragColor = resultColor;
}
)";

GraphicsRenderer::GraphicsRenderer()
{
    // Initialize shader
    m_shader.compile(default_header, default_vshader, default_fshader);
    if (m_shader.isValid() == 0)
    {
        std::printf("Shader compilation failed\n");
    }

#define GET_ATTRIB_LOC(name) m_locs.name = m_shader.getAttribLocation(#name)
    // Get Attribs Locations
    GET_ATTRIB_LOC(a_pos);
    GET_ATTRIB_LOC(a_uv0);
    GET_ATTRIB_LOC(a_uv1);
#undef GET_ATTRIB_LOC

#define GET_UNIFORM_LOC(name) m_locs.name = m_shader.getUniformLocation(#name)
    // Get Uniforms Locations
    GET_UNIFORM_LOC(u_resolution);
    GET_UNIFORM_LOC(u_fragmentType);
    GET_UNIFORM_LOC(u_alpha);
    GET_UNIFORM_LOC(u_scissor);
    GET_UNIFORM_LOC(u_color);
    GET_UNIFORM_LOC(u_imageParams);
    GET_UNIFORM_LOC(u_gradientParam0);
    GET_UNIFORM_LOC(u_gradientParam1);
    // Get Samplers Locations
    GET_UNIFORM_LOC(u_texture);
    GET_UNIFORM_LOC(u_fontAtlas);
#undef GET_UNIFORM_LOC

    // Initialize buffer
    glGenBuffers(1, &m_vbo);
    m_vboSize = 0;
    glGenBuffers(1, &m_ibo);
    m_iboSize = 0;
    glGenVertexArrays(1, &m_vao);

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

GraphicsRenderer::~GraphicsRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ibo);
    glDeleteVertexArrays(1, &m_vao);
}

void GraphicsRenderer::commit(const GraphicsRecorder &recorder)
{
    // Upload font
    recorder.syncFontTexture();

    // Upload vertices
    const std::vector<Vertex> &verts = recorder.m_verts;
    const std::vector<GLuint> &indices = recorder.m_indices;
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    size_t currentVboSize = verts.size() * sizeof(Vertex);
    size_t currentIboSize = indices.size() * sizeof(GLuint);
    if (currentVboSize > m_vboSize || currentVboSize < m_vboSize / 4)
    {
        glBufferData(GL_ARRAY_BUFFER, currentVboSize, verts.data(), GL_DYNAMIC_DRAW);
        m_vboSize = currentVboSize;

        // Set VAO
        glBindVertexArray(m_vao);
        glVertexAttribPointer(m_locs.a_pos, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(offsetof(Vertex, pos)));
        glVertexAttribPointer(m_locs.a_uv0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(offsetof(Vertex, uv0)));
        glVertexAttribPointer(m_locs.a_uv1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(offsetof(Vertex, uv1)));
        glEnableVertexAttribArray(m_locs.a_pos);
        glEnableVertexAttribArray(m_locs.a_uv0);
        glEnableVertexAttribArray(m_locs.a_uv1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, currentVboSize, verts.data());
    }

    // Upload indices
    if (currentIboSize > m_iboSize || currentIboSize < m_iboSize / 4)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, currentIboSize, indices.data(), GL_DYNAMIC_DRAW);
        m_iboSize = currentIboSize;

        // Set VAO
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    }
    else
    {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, currentIboSize, indices.data());
    }

    // Upload calls
    m_calls = recorder.m_calls;
    if (m_calls.size() < m_calls.capacity() / 4)
        m_calls.shrink_to_fit();
}

void GraphicsRenderer::render()
{
    // Bind shader
    m_shader.bind();

    // Bind VAO
    glBindVertexArray(m_vao);

    // Renderer
    glUniform2f(m_locs.u_resolution, static_cast<float>(m_width), static_cast<float>(m_height));
    glUniform1i(m_locs.u_texture, 0);
    glUniform1i(m_locs.u_fontAtlas, 1);

    for (const Call &call : m_calls)
    {
        glBlendFuncSeparate(call.state.sfactor, call.state.dfactor,
                            call.state.sfactor, call.state.dfactor);
        glUniform2ui(m_locs.u_fragmentType, call.state.fillType, call.param.drawType);
        glUniform1f(m_locs.u_alpha, call.state.alpha);
        glUniform4f(m_locs.u_scissor,
                    call.state.scissor.minx, call.state.scissor.miny,
                    call.state.scissor.maxx, call.state.scissor.maxy);
        switch (call.state.fillType)
        {
        case FILL_COLOR:
        {
            // fillColorPass
            glUniform4f(m_locs.u_color,
                        call.state.color.r, call.state.color.g,
                        call.state.color.b, call.state.color.a);
            break;
        }

        case FILL_IMAGE:
        {
            // drawImagePass
            glUniform1ui(m_locs.u_imageParams, call.state.imageParams);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, call.state.texture->getTex());
            break;
        }

        case FILL_LINEAR_GRADIENT:
        case FILL_RADIAL_GRADIENT:
        case FILL_CONIC_GRADIENT:
        {
            // fillGradientPass
            glUniform3f(m_locs.u_gradientParam0,
                        call.state.gradientParam0[0], call.state.gradientParam0[1], call.state.gradientParam0[2]);
            glUniform3f(m_locs.u_gradientParam1,
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
            glBindTexture(GL_TEXTURE_2D, call.param.fontTexture->getTex());
            break;

        default:
            break;
        }
        glDrawElements(GL_TRIANGLES, call.indiceCount, GL_UNSIGNED_INT, call.indiceOffset);
    }
}
