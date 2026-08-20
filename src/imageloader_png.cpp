/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <csetjmp>
#include <string>

#include <png.h> // IWYU pragma: keep

#include "imageloader_png.h"

#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "u4file.h"

ImageLoader *PngImageLoader::instance = registerLoader(
    new PngImageLoader, "image/png"
);

static void png_read_xu4(
    const png_structp png_ptr, const png_bytep data, const png_size_t length
)
{
    auto *file = static_cast<U4FILE *>(png_get_io_ptr(png_ptr));
    const png_size_t check =
            // ReSharper disable once CppRedundantCastExpression
            file->read(data, static_cast<png_size_t>(1), length);
    if (check != length) {
        png_error(png_ptr, "Read Error");
    }
}


/**
 * Loads in the PNG with the libpng library.
 */
Image *PngImageLoader::load(
    U4FILE *file, int width, int height, int bpp
)
{
    if (width != -1 || height != -1 || bpp != -1) {
        errorWarning("dimensions set for PNG image, will be ignored");
    }
    unsigned char header[8];
    file->read(header, 1, sizeof(header));
    if (png_sig_cmp(
            // ReSharper disable once CppRedundantCastExpression
            static_cast<png_byte *>(header), 0, sizeof(header)
        ) != 0) {
        return nullptr;
    }
    png_structp png_ptr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr
    );
    if (!png_ptr) {
        return nullptr;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(
            &png_ptr,
            nullptr,
            nullptr
        );
        return nullptr;
    }
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(
            &png_ptr, &info_ptr, nullptr
        );
        return nullptr;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return nullptr;
    }
    png_set_read_fn(png_ptr, file, &png_read_xu4);
    png_set_sig_bytes(png_ptr, sizeof(header));
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
    png_uint_32 p_width, p_height;
    int bit_depth, color_type, interlace_type;
    int compression_type, filter_method;
    png_get_IHDR(
        png_ptr,
        info_ptr,
        &p_width,
        &p_height,
        &bit_depth,
        &color_type,
        &interlace_type,
        &compression_type,
        &filter_method
    );
    width = static_cast<int>(p_width);
    height = static_cast<int>(p_height);
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        bpp = bit_depth;
    } else if (color_type == PNG_COLOR_TYPE_RGB) {
        bpp = bit_depth * 3;
    } else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        bpp = bit_depth * 4;
    } else {
        bpp = 0; //prevent "clobbered by longjmp" warning
        errorFatal("Unsupported PNG_COLOR_TYPE!");
    }

    png_byte **row_pointers = png_get_rows(png_ptr, info_ptr);
    auto *raw = new unsigned char[width * height * bpp / 8];
    unsigned char *p = raw;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width * bpp / 8; j++) {
            *p++ = row_pointers[i][j];
        }
    }
    Image *image = Image::create(
        width, height, bpp == 4 || bpp == 8, Image::SOFTWARE
    );
    if (!image) {
        delete[] raw;
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return nullptr;
    }
    if (color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
        image->alphaOff();
    }
    if (bpp == 4 || bpp == 8) {
        int num_png_palette;
        png_colorp png_palette;
        png_get_PLTE(png_ptr, info_ptr, &png_palette, &num_png_palette);
        if (num_png_palette < 0 || num_png_palette > 256) {
            errorFatal("PNG Palette with more than 256 entries!");
        }
        auto *palette = new RGBA[num_png_palette];
        for (int c = 0; c < num_png_palette; c++) {
            palette[c].r = png_palette[c].red;
            palette[c].g = png_palette[c].green;
            palette[c].b = png_palette[c].blue;
            palette[c].a = IM_OPAQUE;
        }
        image->setPalette(palette, num_png_palette);
        delete[] palette;
    }
    setFromRawData(image, width, height, bpp, raw);
    delete[] raw;
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return image;
} // PngImageLoader::load
