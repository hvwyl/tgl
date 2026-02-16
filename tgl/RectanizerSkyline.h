#ifndef RECTANIZERSKYLINE_H
#define RECTANIZERSKYLINE_H

#include <vector>

class RectanizerSkyline
{
public:
    RectanizerSkyline(int w, int h) : m_width(w), m_height(h)
    {
        reset();
    }

    ~RectanizerSkyline() = default;

    // Reset the layout.
    void reset();

    // Attempt to add a rect. Return true on success; false on failure. If
    // successful the position in the atlas is returned in 'refx' and 'refy.
    bool addRect(int w, int h, int &refx, int &refy);

    // Return the percentage of the atlas that is filled.
    float percentFull() const
    {
        return m_areaSoFar / ((float)m_width * m_height);
    }

private:
    const int m_width;
    const int m_height;
    int m_areaSoFar;

    struct Segment
    {
        int x, y;
        int width;
    };
    std::vector<Segment> m_skyline;

    // Can a width x height rectangle fit in the free space represented by
    // the skyline segments >= 'skylineIndex'? If so, return the y-location
    // at which it fits (the x location is pulled from 'skylineIndex's segment.
    // Otherwise return -1.
    int rectangleFits(int skylineIndex, int width, int height) const;
    // Update the skyline structure to include a width x height rect located
    // at x,y.
    void addSkylineLevel(int skylineIndex, int x, int y, int width, int height);
};

#endif
