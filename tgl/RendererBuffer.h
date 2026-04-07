#ifndef RENDERERBUFFER_H
#define RENDERERBUFFER_H

#include "GraphicsStructs.h"
#include "OpenGLHeader.h"
#include <vector>
#include <cstddef>

//
// RendererBuffer
//
class RendererBuffer
{
public:
    RendererBuffer();
    ~RendererBuffer();
    inline void bindVAO() const { glBindVertexArray(m_vao); }
    void sync(GLuint posIndex, GLuint uv0Index, GLuint uv1Index,
              const std::vector<Vertex> &verts, const std::vector<GLuint> &indices);

private:
    // VBO & IBO & VAO
    GLuint m_vbo;
    size_t m_vboSize;
    GLuint m_ibo;
    size_t m_iboSize;
    GLuint m_vao;
};

#endif
