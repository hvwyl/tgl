#include "GraphicsShader.h"

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

float sdRect(vec2 uv1)
{
    vec2 dist = abs(uv1) - vec2(0.5);
    float sdf = length(max(dist, 0.0)) + min(max(dist.x, dist.y), 0.0);
    return 1.0 - smoothstep(-1.0, 1.0, sdf / max(fwidth(sdf), 0.00001));
}

float sdCircle(vec2 uv1)
{
    float sdf = length(uv1) - 0.5;
    return 1.0 - smoothstep(-1.0, 1.0, sdf / max(fwidth(sdf), 0.00001));
}

float sdScissor(vec2 pmin, vec2 pmax, float aa) {
    vec2 dist = vec2(
        min(v_pos.x - pmin.x, pmax.x - v_pos.x),
        min(v_pos.y - pmin.y, pmax.y - v_pos.y)
    );
    float sdf = min(dist.x, dist.y);
    return smoothstep(-aa, aa, sdf);
}

////////////////////////////////

void main()
{
    vec2 pxStepPos = fwidth(v_pos);
    float aaPos = max(pxStepPos.x, pxStepPos.y);
    float scissorMask = sdScissor(u_scissor.xy, u_scissor.zw, aaPos);
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

GraphicsShader::GraphicsShader()
    : Shader{}
{
    compile(default_header, default_vshader, default_fshader);

#define GET_ATTRIB_LOC(name) locs.name = getAttribLocation(#name)
#define GET_UNIFORM_LOC(name) locs.name = getUniformLocation(#name)
    // Get Attribs Locations
    GET_ATTRIB_LOC(a_pos);
    GET_ATTRIB_LOC(a_uv0);
    GET_ATTRIB_LOC(a_uv1);
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
#undef GET_ATTRIB_LOC
#undef GET_UNIFORM_LOC
}
