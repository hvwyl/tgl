#ifndef DECODER_H
#define DECODER_H

#include "Bitmap.h"
#include "Image.h"
#include <fstream>

// #define DECODER_ENABLE_PNG_SUPPORT
// #define DECODER_ENABLE_JPEG_SUPPORT

class Decoder
{
public:
    Decoder(const char *path)
        : bitmap(decode(std::make_unique<std::ifstream>(path, std::ios::binary), this)) {}
    Decoder(std::unique_ptr<std::istream> file)
        : bitmap(decode(std::move(file), this)) {};
    ~Decoder() = default;

    Image createImage() const { return Image(bitmap, m_layout, m_flags); }

private:
    Bitmap bitmap;
    uint16_t m_layout;
    uint16_t m_flags;

    static Bitmap decode(std::unique_ptr<std::istream> file, Decoder *decoder);
    static Bitmap decodeBMP(std::unique_ptr<std::istream> file, Decoder *decoder);
#ifdef DECODER_ENABLE_PNG_SUPPORT
    static Bitmap decodePNG(std::unique_ptr<std::istream> file, Decoder *decoder);
#endif
#ifdef DECODER_ENABLE_JPEG_SUPPORT
    static Bitmap decodeJPEG(std::unique_ptr<std::istream> file, Decoder *decoder);
#endif
};

#endif
