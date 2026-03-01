#ifndef SHADER_H
#define SHADER_H

#include "OpenGLHeader.h"

//
// Shader
//
class Shader
{
public:
    Shader() : m_prog(0) {};
    ~Shader();
    void compile(const char *header, const char *vshader, const char *fshader);
    inline void bind() const { glUseProgram(m_prog); }
    inline GLint getAttribLocation(const char *attrib) const { return glGetAttribLocation(m_prog, attrib); }
    inline GLint getUniformLocation(const char *uniform) const { return glGetUniformLocation(m_prog, uniform); }
    inline GLuint getUniformBlockIndex(const char *uniform) const { return glGetUniformBlockIndex(m_prog, uniform); }
    inline int isValid() const { return m_prog != 0; }
    inline GLuint getProg() const { return m_prog; }

private:
    GLuint m_prog;
};

#endif
