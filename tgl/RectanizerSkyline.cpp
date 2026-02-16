#include "RectanizerSkyline.h"

#include <cassert>

void RectanizerSkyline::reset()
{
    m_areaSoFar = 0;
    if (m_skyline.size() < m_skyline.capacity() / 2)
    {
        decltype(m_skyline)().swap(m_skyline);
    }
    else
    {
        m_skyline.clear();
    }
    m_skyline.emplace_back(Segment{0, 0, m_width});
}
bool RectanizerSkyline::addRect(int width, int height, int &refx, int &refy)
{
    if ((unsigned)width > (unsigned)m_width || (unsigned)height > (unsigned)m_height)
    {
        return false;
    }

    // find position for new rectangle
    int bestWidth = m_width + 1;
    int bestX = 0;
    int bestY = m_height + 1;
    int bestIndex = -1;
    for (int i = 0; i < m_skyline.size(); ++i)
    {
        int y = rectangleFits(i, width, height);
        if (y >= 0)
        {
            // minimize y position first, then width of skyline
            if (y < bestY || (y == bestY && m_skyline[i].width < bestWidth))
            {
                bestIndex = i;
                bestWidth = m_skyline[i].width;
                bestX = m_skyline[i].x;
                bestY = y;
            }
        }
    }

    // add rectangle to skyline
    if (-1 != bestIndex)
    {
        addSkylineLevel(bestIndex, bestX, bestY, width, height);
        refx = bestX;
        refy = bestY;

        m_areaSoFar += width * height;
        return true;
    }
    else
    {
        refx = 0;
        refy = 0;
        return false;
    }
}

int RectanizerSkyline::rectangleFits(int skylineIndex, int width, int height) const
{
    int x = m_skyline[skylineIndex].x;
    if (x + width > m_width)
    {
        return -1;
    }

    int widthLeft = width;
    int i = skylineIndex;
    int y = m_skyline[skylineIndex].y;
    while (widthLeft > 0)
    {
        y = std::max(y, m_skyline[i].y);
        if (y + height > m_height)
        {
            return -1;
        }
        widthLeft -= m_skyline[i].width;
        ++i;
        assert(i < m_skyline.size() || widthLeft <= 0);
    }

    return y;
}

void RectanizerSkyline::addSkylineLevel(int skylineIndex, int x, int y, int width, int height)
{
    Segment newSegment;
    newSegment.x = x;
    newSegment.y = y + height;
    newSegment.width = width;
    m_skyline.insert(m_skyline.begin() + skylineIndex, newSegment);

    assert(newSegment.x + newSegment.width <= m_width);
    assert(newSegment.y <= m_height);

    // delete width of the new skyline segment from following ones
    for (int i = skylineIndex + 1; i < m_skyline.size(); ++i)
    {
        // The new segment subsumes all or part of m_skyline[i]
        assert(m_skyline[i - 1].x <= m_skyline[i].x);

        if (m_skyline[i].x < m_skyline[i - 1].x + m_skyline[i - 1].width)
        {
            int shrink = m_skyline[i - 1].x + m_skyline[i - 1].width - m_skyline[i].x;

            m_skyline[i].x += shrink;
            m_skyline[i].width -= shrink;

            if (m_skyline[i].width <= 0)
            {
                // fully consumed
                m_skyline.erase(m_skyline.begin() + i);
                --i;
            }
            else
            {
                // only partially consumed
                break;
            }
        }
        else
        {
            break;
        }
    }

    // merge m_skylines
    for (int i = 0; i < m_skyline.size() - 1; ++i)
    {
        if (m_skyline[i].y == m_skyline[i + 1].y)
        {
            m_skyline[i].width += m_skyline[i + 1].width;
            m_skyline.erase(m_skyline.begin() + i + 1);
            --i;
        }
    }
}
