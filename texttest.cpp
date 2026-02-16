#include "GLWindow.h"
#include "Graphics.h"
#include <cstdio>

#include "fontpath.hpp"

class TestWindow : public GLWindow
{
public:
    Graphics ctx;
    Font simsun{FONT_NotoSerif_PATH};

    TestWindow(int width, int height, const char *title)
        : GLWindow(width, height, title)
    {
        ctx.setResolution(width, height);
    }

    ~TestWindow() {}

protected:
    bool m_needRender = true;

    void render() override
    {
        if (m_needRender)
        {
            glViewport(0, 0, getWidth(), getHeight());
            ctx.setResolution(getWidth(), getHeight());
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ctx.beginFrame();
            ctx.setFillColor(Color::fromRGBf(1.0f, 1.0f, 0.0f));

            ctx.setFontFamily(simsun);
            ctx.setFontPixelSize(96);
            auto mt = L"Hello, World!";
            Graphics::TextMetrics metrics = ctx.measureText(mt);
            ctx.save();
            ctx.setFillColor(Color::fromRGBf(0.0f, 1.0f, 1.0f));
            ctx.fillRect(10, 290, metrics.width, 4);
            ctx.fillRect(10, 290 - metrics.ascent, metrics.width, 4);
            ctx.fillRect(10, 290 + metrics.descent, metrics.width, 4);
            ctx.setFontPixelSize(32);
            ctx.fillText(10, 400, "metrics:");
            ctx.fillText(10, 432, std::string("width:") + std::to_string(metrics.width));
            ctx.fillText(10, 464, std::string("ascent:") + std::to_string(metrics.ascent));
            ctx.fillText(10, 496, std::string("descent:") + std::to_string(metrics.descent));
            ctx.restore();
            ctx.fillText(10, 296, mt);

            ctx.flushFrame();
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
