#include "Decoder.h"

#define MAX_SIG_LENGTH 8

#ifdef DECODER_ENABLE_PNG_SUPPORT
#include <png.h>
#endif

#ifdef DECODER_ENABLE_JPEG_SUPPORT
#include <jpeglib.h>
#endif

Bitmap Decoder::decode(std::unique_ptr<std::istream> file, Decoder *decoder)
{
    decoder->m_flags = Image::FLAG_NONE;
    unsigned char sig[MAX_SIG_LENGTH];
    uint16_t &sigu16 = *reinterpret_cast<uint16_t *>(sig);
    file->read(reinterpret_cast<char *>(&sig), sizeof(uint16_t));
    if (file->gcount() != 2)
        goto error;

    if (sigu16 == 0x4D42)
    {
        return decodeBMP(std::move(file), decoder);
    }
#ifdef DECODER_ENABLE_JPEG_SUPPORT
    else if (sigu16 == 0xD8FF)
    {
        return decodeJPEG(std::move(file), decoder);
    }
#endif

#ifdef DECODER_ENABLE_PNG_SUPPORT
    file->read(reinterpret_cast<char *>(&sig) + sizeof(uint16_t), MAX_SIG_LENGTH - sizeof(uint16_t));
    if (file->gcount() != MAX_SIG_LENGTH - sizeof(uint16_t))
        goto error;

    if (png_check_sig(sig, 8))
    {
        return decodePNG(std::move(file), decoder);
    }
#endif

error:
    throw std::runtime_error("Not a valid file!");
}

#pragma pack(push, 1)

struct BMPFileHeader
{
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

Bitmap Decoder::decodeBMP(std::unique_ptr<std::istream> file, Decoder *decoder)
{
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    file->read(reinterpret_cast<char *>(&file_header), sizeof(BMPFileHeader));
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

    decoder->m_layout = info_header.biBitCount == 24 ? Image::LAYOUT_BGR888 : Image::LAYOUT_BGRA8888;
    if (info_header.biWidth < 0)
        decoder->m_flags |= Image::FLAG_FLIP_X;
    if (info_header.biHeight > 0)
        decoder->m_flags |= Image::FLAG_FLIP_Y;

    return std::move(bitmap);
}

#ifdef DECODER_ENABLE_PNG_SUPPORT

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

Bitmap Decoder::decodePNG(std::unique_ptr<std::istream> file, Decoder *decoder)
{
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

        switch (color_type)
        {
        case PNG_COLOR_TYPE_GRAY:
            decoder->m_layout = Image::LAYOUT_Lum8;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            decoder->m_layout = Image::LAYOUT_LumAlpha88;
            break;
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(png_ptr);
        case PNG_COLOR_TYPE_RGB:
            decoder->m_layout = Image::LAYOUT_RGB888;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
        default:
            decoder->m_layout = Image::LAYOUT_RGBA8888;
            break;
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
            switch (color_type)
            {
            case PNG_COLOR_TYPE_GRAY:
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                decoder->m_layout = Image::LAYOUT_LumAlpha88;
                break;
            case PNG_COLOR_TYPE_PALETTE:
            case PNG_COLOR_TYPE_RGB:
                decoder->m_layout = Image::LAYOUT_RGBA8888;
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

        return std::move(bitmap);
    }
    catch (...)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        throw;
    }
}

#endif

#ifdef DECODER_ENABLE_JPEG_SUPPORT

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

Bitmap Decoder::decodeJPEG(std::unique_ptr<std::istream> file, Decoder *decoder)
{
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

        switch (components)
        {
        case 1:
            decoder->m_layout = Image::LAYOUT_Lum8;
            break;
        case 3:
            decoder->m_layout = Image::LAYOUT_RGB888;
            break;
        case 4:
            decoder->m_layout = Image::LAYOUT_RGBA8888;
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

        return std::move(bitmap);
    }
    catch (...)
    {
        jpeg_abort_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        throw;
    }
}

#endif
