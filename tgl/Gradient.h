#ifndef GRADIENT_H
#define GRADIENT_H

#include "Geometry.h"
#include "Texture.h"
#include <vector>
#include <memory>

//
// Gradient
//
class Gradient
{
public:
    struct ColorStop
    {
        float offset;
        Color color;
        inline bool operator<(const ColorStop &other) const { return offset < other.offset; }
    };

    Gradient(std::vector<ColorStop> colorStops, size_t resolution = 256);
    ~Gradient() = default;

    // Getters
    inline size_t getResolution() const { return m_texture->getWidth(); }

private:
    std::shared_ptr<Texture> m_texture = nullptr;
    friend class Graphics;
};

#endif
