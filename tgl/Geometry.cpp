#include "Geometry.h"
#include <algorithm>

static float hue2rgb(float h, float m1, float m2)
{
    if (h < 0)
        h += 1;
    if (h > 1)
        h -= 1;
    if (h < 1.0f / 6.0f)
        return m1 + (m2 - m1) * h * 6.0f;
    else if (h < 3.0f / 6.0f)
        return m2;
    else if (h < 4.0f / 6.0f)
        return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
    return m1;
}

Color Color::fromHSLA(float h, float s, float l, uint8_t a)
{
    h = std::fmod(h, 1.0f);
    if (h < 0.0f)
        h += 1.0f;
    s = std::clamp(s, 0.0f, 1.0f);
    l = std::clamp(l, 0.0f, 1.0f);
    float m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
    float m1 = 2 * l - m2;
    return Color{
        std::clamp(hue2rgb(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f),
        std::clamp(hue2rgb(h, m1, m2), 0.0f, 1.0f),
        std::clamp(hue2rgb(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f),
        static_cast<float>(a) / 255.0f};
}

Color Color::lerpRGBA(Color c0, Color c1, float u)
{
    u = std::clamp(u, 0.0f, 1.0f);
    float oneminu = 1.0f - u;
    return Color{
        c0.r * oneminu + c1.r * u,
        c0.g * oneminu + c1.g * u,
        c0.b * oneminu + c1.b * u,
        c0.a * oneminu + c1.a * u};
}
