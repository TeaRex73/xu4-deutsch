/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>
#include <cstdlib>
#include <png.h>

#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_png.h"

ImageLoader *PngImageLoader::instance = ImageLoader::registerLoader(
    new PngImageLoader, "image/png"
);

static void png_read_xu4(
    png_structp png_ptr, png_bytep data, png_size_t length
)
{
    png_size_t check;
    U4FILE *file;
    file = static_cast<U4FILE *>(png_get_io_ptr(png_ptr));
    check = file->read(data, static_cast<png_size_t>(1), length);
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
    if ((width != -1) || (height != -1) || (bpp != -1)) {
        errorWarning("dimensions set for PNG image, will be ignored");
    }
    unsigned char header[8];
    file->read(header, 1, sizeof(header));
    if (png_sig_cmp(
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
            static_cast<png_infopp>(nullptr),
            static_cast<png_infopp>(nullptr)
        );
        return nullptr;
    }
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(
            &png_ptr, &info_ptr, static_cast<png_infopp>(nullptr)
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
    png_uint_32 pwidth, pheight;
    int bit_depth, color_type, interlace_type;
    int compression_type, filter_method;
    png_get_IHDR(
        png_ptr,
        info_ptr,
        &pwidth,
        &pheight,
        &bit_depth,
        &color_type,
        &interlace_type,
        &compression_type,
        &filter_method
    );
    width = pwidth;
    height = pheight;
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
    unsigned char *raw = new unsigned char[width * height * bpp / 8];
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
    if ((bpp == 4) || (bpp == 8)) {
        int num_pngpalette;
        png_colorp pngpalette;
        png_get_PLTE(png_ptr, info_ptr, &pngpalette, &num_pngpalette);
        if (num_pngpalette < 0 || num_pngpalette > 256) {
            errorFatal("PNG Palette with more than 256 entries!");
        } else {
            RGBA *palette = new RGBA[num_pngpalette];
            for (int c = 0; c < num_pngpalette; c++) {
                palette[c].r = pngpalette[c].red;
                palette[c].g = pngpalette[c].green;
                palette[c].b = pngpalette[c].blue;
                palette[c].a = IM_OPAQUE;
            }
            image->setPalette(palette, num_pngpalette);
            delete[] palette;
        }
    }
    setFromRawData(image, width, height, bpp, raw);
    delete[] raw;
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return image;
} // PngImageLoader::load
