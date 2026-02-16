#ifndef GRAPHICSBUFFER_H
#define GRAPHICSBUFFER_H

#include "Geometry.h"
#include "OpenGLHeader.h"
#include <vector>
#include <cstddef>

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

//
// GraphicsBuffer
//
class GraphicsBuffer
{
public:
    GraphicsBuffer();
    ~GraphicsBuffer();
    inline void bindVAO() const { glBindVertexArray(m_vao); }
    void sync(GLuint posIndex, GLuint uv0Index, GLuint uv1Index);
    void clear();

    // Vertices & Indices
    std::vector<Vertex> verts;
    std::vector<GLuint> indices;

private:
    // VBO & IBO & VAO
    GLuint m_vbo;
    size_t m_vboSize;
    GLuint m_ibo;
    size_t m_iboSize;
    GLuint m_vao;
};

#endif
