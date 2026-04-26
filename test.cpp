#include "GLWindow.h"
#include "Decoder.h"
#include "GraphicsRecorder.h"
#include "GraphicsRenderer.h"
#include <cstdio>

#include "fontpath.hpp"

class TestWindow : public GLWindow
{
public:
    GraphicsRecorder recorder;
    GraphicsRenderer renderer;
    Image image;

    TestWindow(int width, int height, const char *title)
        : GLWindow(width, height, title)
    {
        Decoder decoder("test.bmp");
        image = decoder.createImage();

        recorder.clear();
        recorder.setFillColor(Color::fromRGBf(1.0f, 1.0f, 0.0f));
        recorder.setFillColor(Color::fromRGBf(1.0f, 1.0f, 1.0f));
        recorder.drawRect(0, 0, 250, 50);
        recorder.setFillColor(Color::fromRGBf(1.0f, 0.0f, 0.0f));
        recorder.drawRect(50, 50, 1280, 720);
        recorder.setFillImage(image);
        recorder.drawImage(80, 80, 0.5f);
        // 渐变测试
        recorder.save();
        recorder.setCompositeOperation(GraphicsRecorder::COMPOSITE_LIGHTER);
        std::vector<Gradient::ColorStop> colorStops;
        colorStops.push_back(Gradient::ColorStop{0.0f, Color::fromRGBf(1.0f, 0.0f, 0.0f)});
        colorStops.push_back(Gradient::ColorStop{1.0f, Color::fromRGBf(0.0f, 0.0f, 1.0f)});
        recorder.setFillLinearGradient(Gradient{colorStops}, 0.0f, 0.0f, 200.0f, 200.0f);
        recorder.drawRect(0, 0, 200, 200);
        recorder.restore();
        // 裁剪测试 + Alpha测试
        recorder.setScissor(150.0f, 150.0f, 100.0f, 100.0f);
        recorder.setCompositeGlobalAlpha(0.5f);
        recorder.setFillColor(Color::fromRGBf(0.0f, 1.0f, 0.0f));
        recorder.drawRect(0, 0, 500.0f, 500.0f);
        recorder.unsetScissor();
        recorder.setCompositeGlobalAlpha(1.0f);
        renderer.commit(recorder);
        recorder.drawRect(900, 300, 100.0f, 100.0f);
        recorder.setFontFamily(Font{FONT_NotoSerif_PATH});
        recorder.setFontPixelSize(64);
        recorder.drawText(10, 64, L"Hello, World!");
        recorder.setFillColor(Color::fromRGBf(1.0f, 1.0f, 1.0f));
        recorder.setFillImage(image);
        recorder.setFillImageClip(image.crop(380, 46, 600, 600));
        recorder.drawRect(500, 500, 200, 200);
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
    TestWindow window{1280, 720, "GLWindow Test"};
    GLWindow::runWaitEvents(window);
    return 0;
}
