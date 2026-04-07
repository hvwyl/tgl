#ifndef GRAPHICSSTRUCTS_H
#define GRAPHICSSTRUCTS_H

#include "Geometry.h"

//
// Vertex
//
struct Vertex
{
    Point pos;
    Point uv0;
    Point uv1;
};

static_assert(std::is_pod_v<Vertex> == true);

#include "Texture.h"
#include <memory>

//
// CallParam
//
enum DrawType : uint32_t
{
    DRAW_RECT = 0u,
    DRAW_CIRCLE = 1u,
    DRAW_FONT = 3u
};
struct CallParam
{
    DrawType drawType;
    std::shared_ptr<Texture> fontTexture = nullptr;
};

//
// CallState
//
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

//
// Call
//
struct Call
{
    CallParam param;
    CallState state;
    void *indiceOffset;
    GLsizei indiceCount;
};

#endif
