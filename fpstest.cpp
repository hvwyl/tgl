#include "GLWindow.h"
#include "Graphics.h"

#include <sstream>

#include "calculateFPS.h"
#include "fontpath.hpp"

class TestWindow : public GLWindow
{
public:
    Graphics ctx;
    double fps;

    float rectX = 0.0f;
    float rectY = 0.0f;
    const float rectWidth = 50.0f;
    const float rectHeight = 50.0f;
    float rectSpeedX = 150.0f;
    float rectSpeedY = 120.0f;
    double lastFrameTime = 0.0;

    TestWindow(int width, int height, const char *title)
        : GLWindow(width, height, title)
    {
        ctx.setResolution(width, height);
        ctx.setFontFamily(Font{FONT_NotoSerif_PATH});
        ctx.setFontPixelSize(32);
        lastFrameTime = glfwGetTime();
    }

    ~TestWindow() {}

protected:
    void render() override
    {
        glViewport(0, 0, getWidth(), getHeight());
        ctx.setResolution(getWidth(), getHeight());

        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ctx.beginFrame();
        ctx.setFillColor(Color::fromRGB(255, 255, 255));
        std::ostringstream ss;
        ss << "FPS: " << fps << "  " << "Screen: " << getWidth() << "x" << getHeight();
        ctx.fillText(0, 32, ss.str());

        rectX += rectSpeedX * deltaTime;
        rectY += rectSpeedY * deltaTime;

        int windowWidth = getWidth();
        int windowHeight = getHeight();

        if (rectX <= 0.0f)
        {
            rectX = 0.0f;
            rectSpeedX = -rectSpeedX;
        }
        else if (rectX + rectWidth >= windowWidth)
        {
            rectX = windowWidth - rectWidth;
            rectSpeedX = -rectSpeedX;
        }

        if (rectY <= 0.0f)
        {
            rectY = 0.0f;
            rectSpeedY = -rectSpeedY;
        }
        else if (rectY + rectHeight >= windowHeight)
        {
            rectY = windowHeight - rectHeight;
            rectSpeedY = -rectSpeedY;
        }

        ctx.fillRect(rectX, rectY, rectWidth, rectHeight);

        ctx.flushFrame();
        swapBuffers();
        fps = calculateFPS();
    }

    void onResize(int width, int height) override
    {
        rectX = 0.0f;
        rectY = 0.0f;
    }
};

int main()
{
    TestWindow window{1280, 720, "FPS Test"};
    GLWindow::runPollEvents(window);
    return 0;
}
