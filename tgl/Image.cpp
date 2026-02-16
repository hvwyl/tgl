#include "Image.h"

static inline uint32_t layoutToFormat(uint16_t layout)
{
    switch (layout)
    {
    case Image::LAYOUT_RGB888:
    case Image::LAYOUT_BGR888:
        return Texture::FORMAT_RGB;
    case Image::LAYOUT_RGBA8888:
    case Image::LAYOUT_BGRA8888:
        return Texture::FORMAT_RGBA;
    case Image::LAYOUT_Lum8:
    case Image::LAYOUT_Alpha8:
        return Texture::FORMAT_RED;
    case Image::LAYOUT_LumAlpha88:
        return Texture::FORMAT_RG;
    default:
        return 0;
    }
}

Image::Image(size_t width, size_t height, uint16_t layout, uint16_t flags)
    : m_layout(layout), m_flags(flags)
{
    m_texture = std::make_shared<Texture>(width, height, layoutToFormat(layout), 0, nullptr);
}

Image::Image(const Bitmap &bitmap, uint16_t layout, uint16_t flags)
    : m_layout(layout), m_flags(flags)
{
    m_texture = std::make_shared<Texture>(bitmap.getWidth(), bitmap.getHeight(), layoutToFormat(layout), 0, bitmap.bufferPointer());
}

Image::Clip Image::crop(float sx, float sy, float sWidth, float sHeight) const
{
    Bounds uv0b{0.0f, 0.0f, 1.0f, 1.0f};
    if (m_texture != nullptr)
    {
        // calculate texture coord
        uv0b.minx = sx / getWidth();
        uv0b.miny = sy / getHeight();
        uv0b.maxx = (sx + sWidth) / getWidth();
        uv0b.maxy = (sy + sHeight) / getHeight();
    }
    return Clip{uv0b};
}

#pragma pack(push, 1)

struct BMPFileHeader
{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader
{
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

#pragma pack(pop)

Image Image::fromBMPFile(std::unique_ptr<std::istream> file)
{
    BMPFileHeader file_header;
    file->read(reinterpret_cast<char *>(&file_header), sizeof(BMPFileHeader));
    if (file_header.bfType != 0x4D42)
        throw std::runtime_error("BMP error: Not a valid BMP file");

    BMPInfoHeader info_header;
    file->read(reinterpret_cast<char *>(&info_header), sizeof(BMPInfoHeader));
    if (info_header.biCompression != 0 || (info_header.biBitCount != 24 && info_header.biBitCount != 32))
        throw std::runtime_error("BMP error: Unsupport format");

    Bitmap bitmap{static_cast<size_t>(std::abs(info_header.biWidth)),
                  static_cast<size_t>(std::abs(info_header.biHeight)),
                  static_cast<size_t>(info_header.biBitCount / 8)};

    file->seekg(file_header.bfOffBits);
    file->read(reinterpret_cast<char *>(bitmap.bufferPointer()), bitmap.bufferSize());
    if (file->gcount() != bitmap.bufferSize())
        throw std::runtime_error("BMP error: Cannot read pixel data");

    uint16_t layout = info_header.biBitCount == 24 ? LAYOUT_BGR888 : LAYOUT_BGRA8888;
    uint16_t flags = 0;
    if (info_header.biWidth < 0)
        flags |= FLAG_FLIP_X;
    if (info_header.biHeight > 0)
        flags |= FLAG_FLIP_Y;

    return Image{bitmap, layout, flags};
}

#ifdef IMAGE_ENABLE_PNG_SUPPORT

#include <png.h>

static void pngError(png_structp png_ptr, png_const_charp msg)
{
    (void)png_ptr;
    throw std::runtime_error(msg);
}

static void pngWarn(png_structp png_ptr, png_const_charp msg)
{
    (void)png_ptr;
    std::printf("Warning: %s\n", msg);
}

static void pngRead(png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::istream *file = reinterpret_cast<std::istream *>(png_get_io_ptr(png_ptr));
    file->read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(length));
}

Image Image::fromPNGFile(std::unique_ptr<std::istream> file)
{
    unsigned char sig[8];
    file->read(reinterpret_cast<char *>(sig), 8);
    if (file->gcount() != 8 || !png_check_sig(sig, 8))
        throw std::runtime_error("PNG error: Not a valid PNG file");

    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    try
    {
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, pngError, pngWarn);
        if (!png_ptr)
            throw std::runtime_error("PNG error: Failed to create png read struct");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
            throw std::runtime_error("PNG error: Failed to create png info struct");

        png_set_read_fn(png_ptr, reinterpret_cast<void *>(file.get()), pngRead);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        png_byte color_type = png_get_color_type(png_ptr, info_ptr);

        if (bit_depth == 16)
            png_set_strip_16(png_ptr);
        else if (bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr);

        uint16_t layout;
        switch (color_type)
        {
        case PNG_COLOR_TYPE_GRAY:
            layout = LAYOUT_Lum8;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            layout = LAYOUT_LumAlpha88;
            break;
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(png_ptr);
        case PNG_COLOR_TYPE_RGB:
            layout = LAYOUT_RGB888;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
        default:
            layout = LAYOUT_RGBA8888;
            break;
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
            switch (color_type)
            {
            case PNG_COLOR_TYPE_GRAY:
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                layout = LAYOUT_LumAlpha88;
                break;
            case PNG_COLOR_TYPE_PALETTE:
            case PNG_COLOR_TYPE_RGB:
                layout = LAYOUT_RGBA8888;
                break;
            default:
                break;
            }
        }

        png_read_update_info(png_ptr, info_ptr);

        png_byte channels = png_get_channels(png_ptr, info_ptr);

        Bitmap bitmap{width, height, channels};

        size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
        std::unique_ptr<png_bytep[]> row_pointers(new png_bytep[height]);
        for (png_uint_32 i = 0; i < height; ++i)
            row_pointers[i] = bitmap.rowPointer(i);

        png_read_image(png_ptr, row_pointers.get());
        png_read_end(png_ptr, nullptr);

        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

        return Image{bitmap, layout, 0};
    }
    catch (...)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        throw;
    }
}

#endif

#ifdef IMAGE_ENABLE_JPEG_SUPPORT

#include <jpeglib.h>

static void jpegError(j_common_ptr cinfo)
{
    char err_msg[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, err_msg);
    jpeg_destroy(cinfo);
    throw std::runtime_error(err_msg);
}

#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE 4096
#endif

struct jpeg_istream_source_mgr
{
    struct jpeg_source_mgr pub;
    std::istream *file;
    JOCTET *buffer;
};

typedef struct jpeg_istream_source_mgr *jpeg_istream_source_mgr_ptr;

static void jpegIstreamInitSource(j_decompress_ptr cinfo)
{
}

static boolean jpegIstreamFillInputBuffer(j_decompress_ptr cinfo)
{
    jpeg_istream_source_mgr_ptr src = (jpeg_istream_source_mgr_ptr)cinfo->src;
    std::istream *file = src->file;

    file->read(reinterpret_cast<char *>(src->buffer), INPUT_BUFFER_SIZE);
    size_t bytes_read = file->gcount();

    if (bytes_read <= 0)
    {
        if (file->tellg() == 0)
            jpegError((j_common_ptr)cinfo);
        src->buffer[0] = (JOCTET)0xFF;
        src->buffer[1] = (JOCTET)JPEG_EOI;
        bytes_read = 2;
    }

    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = bytes_read;

    return TRUE;
}

static void jpegIstreamSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
    jpeg_istream_source_mgr_ptr src = (jpeg_istream_source_mgr_ptr)cinfo->src;

    if (num_bytes <= 0)
        return;

    while (num_bytes > (long)src->pub.bytes_in_buffer)
    {
        num_bytes -= (long)src->pub.bytes_in_buffer;
        if (!jpegIstreamFillInputBuffer(cinfo))
            return;
    }

    src->pub.next_input_byte += (size_t)num_bytes;
    src->pub.bytes_in_buffer -= (size_t)num_bytes;
}

static void jpegIstreamTermSource(j_decompress_ptr cinfo)
{
}

Image Image::fromJPEGFile(std::unique_ptr<std::istream> file)
{
    uint16_t sig;
    file->read(reinterpret_cast<char *>(&sig), sizeof(uint16_t));
    if (file->gcount() != 2 || sig != 0xD8FF)
        throw std::runtime_error("Not a valid JPEG file!");

    file->seekg(0, std::ios::beg);

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpegError;

    jpeg_create_decompress(&cinfo);

    jpeg_istream_source_mgr_ptr src;
    if (cinfo.src == nullptr)
    {
        cinfo.src = (struct jpeg_source_mgr *)(*cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_PERMANENT, sizeof(jpeg_istream_source_mgr));
        src = (jpeg_istream_source_mgr_ptr)cinfo.src;
        src->buffer = (JOCTET *)(cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_PERMANENT, INPUT_BUFFER_SIZE * sizeof(JOCTET));
    }
    src->pub.init_source = jpegIstreamInitSource;
    src->pub.fill_input_buffer = jpegIstreamFillInputBuffer;
    src->pub.skip_input_data = jpegIstreamSkipInputData;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    src->pub.term_source = jpegIstreamTermSource;
    src->file = file.get();
    src->pub.bytes_in_buffer = 0;
    src->pub.next_input_byte = nullptr;

    try
    {
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        size_t width = cinfo.output_width;
        size_t height = cinfo.output_height;
        int components = cinfo.output_components;

        uint16_t layout;
        switch (components)
        {
        case 1:
            layout = LAYOUT_Lum8;
            break;
        case 3:
            layout = LAYOUT_RGB888;
            break;
        case 4:
            layout = LAYOUT_RGBA8888;
            break;
        default:
            throw std::runtime_error("Unsupported JPEG color component.");
        }

        Bitmap bitmap{width, height, static_cast<size_t>(components)};
        JSAMPROW row_buffer[1];
        while (cinfo.output_scanline < cinfo.output_height)
        {
            row_buffer[0] = (JSAMPLE *)(bitmap.rowPointer(cinfo.output_scanline));
            jpeg_read_scanlines(&cinfo, row_buffer, 1);
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        return Image{bitmap, layout, 0};
    }
    catch (...)
    {
        jpeg_abort_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        throw;
    }
}

#endif
