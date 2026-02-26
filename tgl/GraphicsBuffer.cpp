#include "GraphicsBuffer.h"

GraphicsBuffer::GraphicsBuffer()
{
    glGenBuffers(1, &m_vbo);
    m_vboSize = 0;
    glGenBuffers(1, &m_ibo);
    m_iboSize = 0;
    glGenVertexArrays(1, &m_vao);
}

GraphicsBuffer::~GraphicsBuffer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ibo);
    glDeleteVertexArrays(1, &m_vao);
}

void GraphicsBuffer::sync(GLuint posIndex, GLuint uv0Index, GLuint uv1Index,
                          std::vector<Vertex> &verts, std::vector<GLuint> &indices)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    size_t currentVboSize = verts.size() * sizeof(Vertex);
    size_t currentIboSize = indices.size() * sizeof(GLuint);
    if (currentVboSize > m_vboSize || currentVboSize < m_vboSize / 4)
    {
        glBufferData(GL_ARRAY_BUFFER, currentVboSize, verts.data(), GL_DYNAMIC_DRAW);
        m_vboSize = currentVboSize;

        // Set VAO
        glBindVertexArray(m_vao);
        glVertexAttribPointer(posIndex, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(offsetof(Vertex, pos)));
        glVertexAttribPointer(uv0Index, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(offsetof(Vertex, uv0)));
        glVertexAttribPointer(uv1Index, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(offsetof(Vertex, uv1)));
        glEnableVertexAttribArray(posIndex);
        glEnableVertexAttribArray(uv0Index);
        glEnableVertexAttribArray(uv1Index);
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
}
