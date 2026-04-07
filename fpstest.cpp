#include "GLWindow.h"
#include "GraphicsRecorder.h"
#include "GraphicsRenderer.h"

#include <sstream>

#include "calculateFPS.h"
#include "fontpath.hpp"

class TestWindow : public GLWindow
{
public:
    GraphicsRecorder recorder;
    GraphicsRenderer renderer;
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
        recorder.setFontFamily(Font{FONT_NotoSerif_PATH});
        recorder.setFontPixelSize(32);
        lastFrameTime = glfwGetTime();
    }

    ~TestWindow() {}

protected:
    void render() override
    {
        glViewport(0, 0, getWidth(), getHeight());
        renderer.setResolution(getWidth(), getHeight());

        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        recorder.clear();
        recorder.setFillColor(Color::fromRGB(255, 255, 255));
        std::ostringstream ss;
        ss << "FPS: " << fps << "  " << "Screen: " << getWidth() << "x" << getHeight();
        recorder.drawText(0, 32, ss.str());

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

        recorder.drawRect(rectX, rectY, rectWidth, rectHeight);

        renderer.commit(recorder);
        renderer.render();
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
