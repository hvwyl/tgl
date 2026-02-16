#include "FontFace.h"

FontManager &FontManager::getInstance()
{
    static FontManager instance;
    return instance;
}

FontFace::FontFace(const char *path)
    : m_fontManager(FontManager::getInstance())
{
    if (FT_New_Face(m_fontManager.m_ftLib, path, 0, &m_ftFace))
        throw std::runtime_error("Freetype error: FT_New_Face");
}

static unsigned long freetypeStreamRecRead(FT_Stream stream,
                                           unsigned long offset,
                                           unsigned char *buffer,
                                           unsigned long count)
{
    std::istream *file = static_cast<std::istream *>(stream->descriptor.pointer);
    file->seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (file->fail())
        return 0;
    file->read(reinterpret_cast<char *>(buffer), count);
    return static_cast<unsigned long>(file->gcount());
}

static void freetypeStreamRecClose(FT_Stream stream)
{
}

FontFace::FontFace(std::unique_ptr<std::istream> file)
    : m_fontManager(FontManager::getInstance()), m_istreamPointer(std::move(file))
{
    m_istreamPointer->seekg(0, std::ios::end);
    std::streamsize totalSize = m_istreamPointer->tellg();
    m_istreamPointer->seekg(0, std::ios::beg);

    m_ftStreamRec.base = nullptr;
    m_ftStreamRec.size = static_cast<unsigned long>(totalSize);
    m_ftStreamRec.pos = 0;
    m_ftStreamRec.descriptor.pointer = static_cast<void *>(m_istreamPointer.get());
    m_ftStreamRec.read = freetypeStreamRecRead;
    m_ftStreamRec.close = freetypeStreamRecClose;

    FT_Open_Args ftOpenArgs;
    ftOpenArgs.flags = FT_OPEN_STREAM;
    ftOpenArgs.stream = &m_ftStreamRec;

    if (FT_Open_Face(m_fontManager.m_ftLib, &ftOpenArgs, 0, &m_ftFace))
        throw std::runtime_error("Freetype error: FT_Open_Face");
}

FontFace::~FontFace()
{
    if (m_ftFace != nullptr)
    {
        FT_Done_Face(m_ftFace);
        m_ftFace = nullptr;
    }
}

FontFace::FontFace(FontFace &&other) noexcept
    : m_fontManager(FontManager::getInstance()),
      m_istreamPointer(std::move(other.m_istreamPointer)),
      m_ftStreamRec(other.m_ftStreamRec),
      m_ftFace(other.m_ftFace)
{
    m_ftStreamRec.descriptor.pointer = static_cast<void *>(m_istreamPointer.get());
    other.m_ftFace = nullptr;
}

FontFace &FontFace::operator=(FontFace &&other) noexcept
{
    if (this != &other)
    {
        if (m_ftFace != nullptr)
            FT_Done_Face(m_ftFace);
        m_istreamPointer = std::move(other.m_istreamPointer);
        m_ftStreamRec = other.m_ftStreamRec;
        m_ftFace = other.m_ftFace;
        m_ftStreamRec.descriptor.pointer = static_cast<void *>(m_istreamPointer.get());
        other.m_ftFace = nullptr;
    }
    return *this;
}
