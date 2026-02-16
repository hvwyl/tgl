#ifndef FONT_H
#define FONT_H

#include <memory>

class FontAtlas;

//
// Font
//
class Font
{
public:
    Font(const char *path);
    Font(std::unique_ptr<std::istream> file);
    ~Font() = default;

private:
    std::shared_ptr<FontAtlas> m_atlas = nullptr;

    friend class Graphics;
};

#endif
