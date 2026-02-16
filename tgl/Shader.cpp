#include "Shader.h"
#include <cstdio>

static void dumpShaderError(GLuint shader, const char *type)
{
    GLchar str[512 + 1];
    GLsizei len = 0;
    glGetShaderInfoLog(shader, 512, &len, str);
    if (len > 512)
        len = 512;
    str[len] = '\0';
    std::printf("Shader error: %s\n%s\n", type, str);
};

static void dumpProgramError(GLuint prog)
{
    GLchar str[512 + 1];
    GLsizei len = 0;
    glGetProgramInfoLog(prog, 512, &len, str);
    if (len > 512)
        len = 512;
    str[len] = '\0';
    std::printf("Program error:\n%s\n", str);
}

Shader::~Shader()
{
    if (m_prog != 0)
    {
        glDeleteProgram(m_prog);
        m_prog = 0;
    }
}

void Shader::compile(const char *header, const char *vshader, const char *fshader)
{
    GLint status;
    const char *str[3] = {header, "\n", nullptr};

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    str[2] = vshader;
    glShaderSource(vert, 3, str, 0);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        dumpShaderError(vert, "vert");
        return;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    str[2] = fshader;
    glShaderSource(frag, 3, str, 0);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        dumpShaderError(frag, "frag");
        return;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        dumpProgramError(prog);
        return;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    m_prog = prog;
}
