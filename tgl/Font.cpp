#include "Font.h"
#include "FontAtlas.h"

Font::Font(const char *path)
{
    m_atlas = std::make_shared<FontAtlas>(std::make_unique<FontFace>(path));
}

Font::Font(std::unique_ptr<std::istream> file)
{
    m_atlas = std::make_shared<FontAtlas>(std::make_unique<FontFace>(std::move(file)));
}
