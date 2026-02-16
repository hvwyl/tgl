#ifndef GLWINDOW_H
#define GLWINDOW_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <string>
#include <memory>

// #define GLWINDOW_DUMP_GL_INFO
// #define GLWINDOW_ENABLE_MSAA
// #define GLWINDOW_ENABLE_HIDPIFACTOR

class GLWindow
{
public:
    GLWindow(int width, int height, std::string title);
    ~GLWindow();

    static void runPollEvents(GLWindow &window);
    static void runWaitEvents(GLWindow &window);

    inline void swapBuffers() { glfwSwapBuffers(m_window); };
    inline void setWindowTitle(std::string title) { glfwSetWindowTitle(m_window, title.c_str()); };
    inline void setWindowShouldClose(bool value)
    {
        glfwSetWindowShouldClose(m_window, value ? GLFW_TRUE : GLFW_FALSE);
    };
    inline int getWidth() const { return m_width; }
    inline int getHeight() const { return m_height; }

#ifdef GLWINDOW_ENABLE_HIDPIFACTOR
    inline float getHidipFactorX() const { return m_hidpi_factor_x; }
    inline float getHidipFactorY() const { return m_hidpi_factor_y; }
#endif

protected:
    virtual void onResize(int width, int height) {};
    virtual void onKeyDown(int key, int scancode, int action, int mods) {};
    virtual void onKeyUp(int key, int scancode, int action, int mods) {};
    virtual void onMouseButton(int button, int action, int mods) {};
    virtual void onMouseMove(double xpos, double ypos) {};
    virtual void onMouseEnter() {};
    virtual void onMouseLeave() {};
    virtual void onScroll(double xoffset, double yoffset) {};
    virtual void onCharacter(unsigned int codepoint) {};
    virtual void onRefresh() {};
    virtual void onClose() {};

    virtual void render() = 0;

private:
    GLFWwindow *m_window = nullptr;
    int m_width;
    int m_height;
#ifdef GLWINDOW_ENABLE_HIDPIFACTOR
    float m_hidpi_factor_x = 1.0f;
    float m_hidpi_factor_y = 1.0f;
#endif

    static int &windowCount();
    static bool &isInitialized();

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
    static void cursorEnterCallback(GLFWwindow *window, int entered);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    static void charCallback(GLFWwindow *window, unsigned int codepoint);
    static void windowRefreshCallback(GLFWwindow *window);
    static void windowCloseCallback(GLFWwindow *window);
};

#endif
