#include "GLWindow.h"
#include "GraphicsRecorder.h"
#include "GraphicsRenderer.h"
#include <cstdio>

#include "fontpath.hpp"

class TestWindow : public GLWindow
{
public:
    GraphicsRecorder recorder;
    GraphicsRenderer renderer;
    Font simsun{FONT_NotoSerif_PATH};

    TestWindow(int width, int height, const char *title)
        : GLWindow(width, height, title)
    {
        recorder.clear();
        recorder.setFillColor(Color::fromRGBf(1.0f, 1.0f, 0.0f));

        recorder.setFontFamily(simsun);
        recorder.setFontPixelSize(96);
        auto mt = L"Hello, World!";
        GraphicsRecorder::TextMetrics metrics = recorder.measureText(mt);
        recorder.save();
        recorder.setFillColor(Color::fromRGBf(0.0f, 1.0f, 1.0f));
        recorder.drawRect(10, 290, metrics.width, 4);
        recorder.drawRect(10, 290 - metrics.ascent, metrics.width, 4);
        recorder.drawRect(10, 290 + metrics.descent, metrics.width, 4);
        recorder.setFontPixelSize(32);
        recorder.drawText(10, 400, "metrics:");
        recorder.drawText(10, 432, std::string("width:") + std::to_string(metrics.width));
        recorder.drawText(10, 464, std::string("ascent:") + std::to_string(metrics.ascent));
        recorder.drawText(10, 496, std::string("descent:") + std::to_string(metrics.descent));
        recorder.restore();
        recorder.drawText(10, 296, mt);

        renderer.commit(recorder);
    }

    ~TestWindow() {}

protected:
    bool m_needRender = true;

    void render() override
    {
        if (m_needRender)
        {
            glViewport(0, 0, getWidth(), getHeight());
            renderer.setResolution(getWidth(), getHeight());
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            renderer.render();
            swapBuffers();
            m_needRender = false;
        }
    }

    void onResize(int width, int height) override
    {
        m_needRender = true;
    }
};

int main()
{
    try
    {
        TestWindow window{1280, 720, "Text Test"};
        GLWindow::runWaitEvents(window);
    }
    catch (std::exception &e)
    {
        std::printf("%s\n", e.what());
    }
    return 0;
}
