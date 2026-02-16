#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cstdint>
#include <cmath>

//
// Color
//
class Color
{
public:
    float r, g, b, a;

    // Returns a color value from red, green, blue and alpha values.
    static constexpr inline Color fromRGBAf(float r, float g, float b, float a) { return Color{r, g, b, a}; };
    static constexpr inline Color fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return Color{
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f};
    };

    // Returns a color value from red, green, blue values. Alpha will be set to 1.0f (255).
    static constexpr inline Color fromRGBf(float r, float g, float b) { return fromRGBAf(r, g, b, 1.0f); };
    static constexpr inline Color fromRGB(uint8_t r, uint8_t g, uint8_t b) { return fromRGBA(r, g, b, 255); };

    // Returns color value specified by hue, saturation and lightness and alpha.
    // HSL values are all in range [0.0f..1.0f], alpha in range [0..255]
    static Color fromHSLA(float h, float s, float l, uint8_t a);

    // Returns color value specified by hue, saturation and lightness.
    // HSL values are all in range [0.0f..1.0f], alpha will be set to 255.
    static inline Color fromHSL(float h, float s, float l) { return fromHSLA(h, s, l, 255); };

    // Returns premultiplied color value for rendering
    static inline Color premulColor(const Color &c)
    {
        return Color{c.r * c.a, c.g * c.a, c.b * c.a, c.a};
    }

    // Linearly interpolates from color c0 to c1, and returns resulting color value.
    static Color lerpRGBA(Color c0, Color c1, float u);
};

static_assert(std::is_pod_v<Color> == true);

//
// Point
//
class Point
{
public:
    float x, y;

    constexpr inline Point operator+(const Point &p) const { return Point{x + p.x, y + p.y}; }
    constexpr inline Point operator-(const Point &p) const { return Point{x - p.x, y - p.y}; }
    template <typename T>
    constexpr inline Point operator*(T s) const { return Point{static_cast<float>(x * s), static_cast<float>(y * s)}; }
    template <typename T>
    constexpr inline Point operator/(T s) const { return Point{static_cast<float>(x / s), static_cast<float>(y / s)}; }
    template <typename T>
    friend constexpr inline Point operator*(T s, const Point &p) { return p * s; }
    constexpr inline double length() const { return std::hypot(static_cast<double>(x), static_cast<double>(y)); }
    constexpr inline double lengthSq() const { return x * x + y * y; }
    constexpr inline Point transform(float a11, float a22) const
    {
        return Point{a11 * x, a22 * y};
    }
    constexpr inline Point transform(float a11, float a12, float a21, float a22) const
    {
        return Point{a11 * x + a12 * y, a21 * x + a22 * y};
    }
    constexpr inline Point transform(float a11, float a12, float a21, float a22, float a13, float a23) const
    {
        return Point{a11 * x + a12 * y + a13, a21 * x + a22 * y + a23};
    }
    constexpr inline Point translate(float tx, float ty) const
    {
        return Point{x + tx, y + ty};
    }
    constexpr inline Point rotate(const Point &dir) const
    {
        return transform(dir.x, -dir.y, dir.y, dir.x);
    }
    constexpr inline Point rotate(float angle) const
    {
        return rotate(Point{std::cos(angle), std::sin(angle)});
    }
    constexpr inline Point rotate90() const
    {
        return Point{-y, x};
    }
    constexpr inline Point rotate180() const
    {
        return Point{-x, -y};
    }
    constexpr inline Point rotate270() const
    {
        return Point{y, -x};
    }
    static constexpr inline Point normalize(const Point &p)
    {
        double ln = p.length();
        return (ln == 0) ? Point{p.x, p.y} : Point{static_cast<float>(p.x / ln), static_cast<float>(p.y / ln)};
    }
    static constexpr inline double cross(const Point &p1, const Point &p2)
    {
        return static_cast<double>(p1.x) * p2.y - static_cast<double>(p1.y) * p2.x;
    }
    static constexpr inline double dot(const Point &p1, const Point &p2)
    {
        return static_cast<double>(p1.x) * p2.x + static_cast<double>(p1.y) * p2.y;
    }
};

static_assert(std::is_pod_v<Point> == true);

//
// Bounds
//
class Bounds
{
public:
    float minx, miny, maxx, maxy;

    Bounds() : minx(+std::numeric_limits<float>::infinity()),
               miny(+std::numeric_limits<float>::infinity()),
               maxx(-std::numeric_limits<float>::infinity()),
               maxy(-std::numeric_limits<float>::infinity()) {}
    Bounds(float minx, float miny, float maxx, float maxy)
        : minx(minx), miny(miny), maxx(maxx), maxy(maxy) {}
    Bounds(const Point &p)
        : minx(p.x), miny(p.y), maxx(p.x), maxy(p.y) {}
    Bounds(const Point &p1, const Point &p2)
        : minx(std::min(p1.x, p2.x)),
          miny(std::min(p1.y, p2.y)),
          maxx(std::max(p1.x, p2.x)),
          maxy(std::max(p1.y, p2.y)) {}
    Bounds operator+(const Bounds &b) const
    {
        return Bounds{std::min(minx, b.minx), std::min(miny, b.miny),
                      std::max(maxx, b.maxx), std::max(maxy, b.maxy)};
    }
};

#endif
