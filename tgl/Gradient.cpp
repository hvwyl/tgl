#include "Gradient.h"
#include <algorithm>
#include <cstdio>

static_assert(sizeof(float) == 4, "sizeof(float) must be equal to 4");

Gradient::Gradient(std::vector<ColorStop> colorStops, size_t resolution)
{
    std::unique_ptr<Color[]> gradientRamp{new Color[resolution]};

    if (colorStops.empty())
    {
        std::fill(gradientRamp.get(), gradientRamp.get() + resolution, Color::fromRGBAf(0.0f, 0.0f, 0.0f, 0.0f));
    }
    else if (colorStops.size() == 1 || resolution == 1)
    {
        std::fill(gradientRamp.get(), gradientRamp.get() + resolution, Color::premulColor(colorStops[0].color));
    }
    else
    {
        for (ColorStop &stop : colorStops)
            stop.color = Color::premulColor(stop.color);

        std::sort(colorStops.begin(), colorStops.end());

        const float step = 1.0f / (resolution - 1);
        for (size_t i = 0; i < resolution; i++)
        {
            float offset = std::clamp(static_cast<float>(i) * step, 0.0f, 1.0f);
            std::vector<ColorStop>::iterator it = std::upper_bound(
                colorStops.begin(), colorStops.end(), offset,
                [](float val, const ColorStop &stop)
                { return val < stop.offset; });
            if (it == colorStops.begin())
                gradientRamp[i] = it->color;
            else if (it == colorStops.end())
                gradientRamp[i] = (it - 1)->color;
            else
            {
                const ColorStop &stop1 = *(it);
                const ColorStop &stop0 = *(it - 1);
                const float u = (offset - stop0.offset) / (stop1.offset - stop0.offset);
                gradientRamp[i] = Color::lerpRGBA(stop0.color, stop1.color, u);
            }
        }
    }

    m_texture = std::make_shared<Texture>(resolution, 1, Texture::FORMAT_RGBA, Texture::FLAG_FLOAT,
                                          reinterpret_cast<unsigned char *>(gradientRamp.get()));
};
