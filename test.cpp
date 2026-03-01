#include "GLWindow.h"
#include "Decoder.h"
#include "Graphics.h"
#include <cstdio>

#include "fontpath.hpp"

class TestWindow : public GLWindow
{
public:
    Graphics ctx;
    Image image;

    TestWindow(int width, int height, const char *title)
        : GLWindow(width, height, title)
    {
        ctx.setResolution(width, height);
        Decoder decoder("test.bmp");
        image = decoder.createImage();
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
            ctx.setFillColor(Color::fromRGBf(1.0f, 1.0f, 1.0f));
            ctx.fillRect(0, 0, 250, 50);
            ctx.setFillColor(Color::fromRGBf(1.0f, 0.0f, 0.0f));
            ctx.fillRect(50, 50, 1280, 720);
            ctx.setFillImage(image);
            ctx.fillImage(80, 80, 0.5f);
            // 渐变测试
            ctx.save();
            ctx.setCompositeOperation(Graphics::COMPOSITE_LIGHTER);
            std::vector<Gradient::ColorStop> colorStops;
            colorStops.push_back(Gradient::ColorStop{0.0f, Color::fromRGBf(1.0f, 0.0f, 0.0f)});
            colorStops.push_back(Gradient::ColorStop{1.0f, Color::fromRGBf(0.0f, 0.0f, 1.0f)});
            ctx.setFillLinearGradient(Gradient{colorStops}, 0.0f, 0.0f, 200.0f, 200.0f);
            ctx.fillRect(0, 0, 200, 200);
            ctx.restore();
            // 裁剪测试 + Alpha测试
            ctx.setScissor(150.0f, 150.0f, 100.0f, 100.0f);
            ctx.setCompositeGlobalAlpha(0.5f);
            ctx.setFillColor(Color::fromRGBf(0.0f, 1.0f, 0.0f));
            ctx.fillRect(0, 0, 500.0f, 500.0f);
            ctx.unsetScissor();
            ctx.setCompositeGlobalAlpha(1.0f);
            ctx.flushFrame();
            ctx.fillRect(900, 300, 100.0f, 100.0f);
            ctx.setFontFamily(Font{FONT_NotoSerif_PATH});
            ctx.setFontPixelSize(64);
            ctx.fillText(10, 64, L"Hello, World!");
            ctx.setFillColor(Color::fromRGBf(1.0f, 1.0f, 1.0f));
            ctx.setFillImage(image);
            ctx.fillCircle(500, 500, 200, 200, image.crop(380, 46, 600, 600));
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
    TestWindow window{1280, 720, "GLWindow Test"};
    GLWindow::runWaitEvents(window);
    return 0;
}
