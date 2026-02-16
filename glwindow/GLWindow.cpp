#include "GLWindow.h"
#include <cstdio>

GLWindow::GLWindow(int width, int height, std::string title)
    : m_width(width), m_height(height)
{
    bool loadGL = false;
    // Initialize GLFW
    if (!isInitialized())
    {
        // Set error callback
        glfwSetErrorCallback([](int error, const char *description)
                             { std::fprintf(stderr, "Error: %s\n", description); });

        // Init GLFW
        if (!glfwInit())
        {
            std::fprintf(stderr, "Failed to initialize GLFW\n");
            return;
        }

        // OpenGL 3.3
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef GLWINDOW_ENABLE_MSAA
        // Enable Multisampling
        glfwWindowHint(GLFW_SAMPLES, 4);
#endif

#ifdef GLWINDOW_ENABLE_HIDPIFACTOR
        // Hi-DPI Support
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_TRUE);
#endif
        isInitialized() = true;

        // Need to load GL
        loadGL = true;
    }

    // Create a GLFW window
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        return;
    }

    // Increment window count
    windowCount()++;

#ifdef GLWINDOW_ENABLE_HIDPIFACTOR
    // Update framebuffer size
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    m_hidpi_factor_x = static_cast<float>(m_width) / width;
    m_hidpi_factor_y = static_cast<float>(m_height) / height;
#endif

    // Set user pointer
    glfwSetWindowUserPointer(m_window, this);

    // Set callbacks
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetCursorEnterCallback(m_window, cursorEnterCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetCharCallback(m_window, charCallback);
    glfwSetWindowRefreshCallback(m_window, windowRefreshCallback);
    glfwSetWindowCloseCallback(m_window, windowCloseCallback);

    // Make the OpenGL context current
    glfwMakeContextCurrent(m_window);

    // Load GLAD
    if (loadGL)
    {
        gladLoadGL(glfwGetProcAddress);
    }

    // Dump GL info
#ifdef GLWINDOW_DUMP_GL_INFO
    std::fprintf(stdout, "OpenGL Version: %s\n"
                         "GLSL Version: %s\n"
                         "Vendor: %s\n"
                         "Renderer: %s\n",
                 glGetString(GL_VERSION),
                 glGetString(GL_SHADING_LANGUAGE_VERSION),
                 glGetString(GL_VENDOR),
                 glGetString(GL_RENDERER));
#endif
}

GLWindow::~GLWindow()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;

        // Decrement window count
        windowCount()--;
    }

    if (windowCount() == 0 && isInitialized())
    {
        // Terminate GLFW
        glfwTerminate();
        isInitialized() = false;
    }
}

void GLWindow::runPollEvents(GLWindow &window)
{
    while (!glfwWindowShouldClose(window.m_window))
    {
        window.render();
        glfwPollEvents();
    }
}

void GLWindow::runWaitEvents(GLWindow &window)
{
    while (!glfwWindowShouldClose(window.m_window))
    {
        window.render();
        glfwWaitEvents();
    }
}

int &GLWindow::windowCount()
{
    static int s_windowCount = 0;
    return s_windowCount;
}

bool &GLWindow::isInitialized()
{
    static bool s_initialized = false;
    return s_initialized;
}

void GLWindow::framebufferSizeCallback(GLFWwindow *m_window, int width, int height)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
    {
        thisptr->m_width = width;
        thisptr->m_height = height;
        thisptr->onResize(thisptr->m_width, thisptr->m_height);
    }
}

void GLWindow::keyCallback(GLFWwindow *m_window, int key, int scancode, int action, int mods)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            thisptr->onKeyDown(key, scancode, action, mods);
        else if (action == GLFW_RELEASE)
            thisptr->onKeyUp(key, scancode, action, mods);
    }
}

void GLWindow::mouseButtonCallback(GLFWwindow *m_window, int button, int action, int mods)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
        thisptr->onMouseButton(button, action, mods);
}

void GLWindow::cursorPosCallback(GLFWwindow *m_window, double xpos, double ypos)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
        thisptr->onMouseMove(xpos, ypos);
}

void GLWindow::cursorEnterCallback(GLFWwindow *m_window, int entered)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
    {
        if (entered)
            thisptr->onMouseEnter();
        else
            thisptr->onMouseLeave();
    }
}

void GLWindow::scrollCallback(GLFWwindow *m_window, double xoffset, double yoffset)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
        thisptr->onScroll(xoffset, yoffset);
}

void GLWindow::charCallback(GLFWwindow *m_window, unsigned int codepoint)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
        thisptr->onCharacter(codepoint);
}

void GLWindow::windowRefreshCallback(GLFWwindow *m_window)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
        thisptr->onRefresh();
}

void GLWindow::windowCloseCallback(GLFWwindow *m_window)
{
    GLWindow *thisptr = static_cast<GLWindow *>(glfwGetWindowUserPointer(m_window));
    if (thisptr)
        thisptr->onClose();
}
