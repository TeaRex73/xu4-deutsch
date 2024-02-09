/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <vector>

#include "config.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_u4.h"
#include "rle.h"
#include "lzw/u4decode.h"

ImageLoader *U4RawImageLoader::instance =
    ImageLoader::registerLoader(new U4RawImageLoader, "image/x-u4raw");
ImageLoader *U4RleImageLoader::instance =
    ImageLoader::registerLoader(new U4RleImageLoader, "image/x-u4rle");
ImageLoader *U4LzwImageLoader::instance =
    ImageLoader::registerLoader(new U4LzwImageLoader, "image/x-u4lzw");

RGBA *U4PaletteLoader::bwPalette = nullptr;
RGBA *U4PaletteLoader::egaPalette = nullptr;
RGBA *U4PaletteLoader::vgaPalette = nullptr;


/**
 * Loads in the raw image and apply the standard U4 16 or 256 color
 * palette.
 */
Image *U4RawImageLoader::load(U4FILE *file, int width, int height, int bpp)
{
    if ((width == -1) || (height == -1) || (bpp == -1)) {
        errorFatal("dimensions not set for u4raw image");
    }
    U4ASSERT(
        bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32,
        "invalid bpp: %d",
        bpp
    );
    long rawLen = file->length();
    unsigned char *raw = static_cast<unsigned char *>(std::malloc(rawLen));
    file->read(raw, 1, rawLen);
    long requiredLength = (width * height * bpp / 8);
    if (rawLen < requiredLength) {
        if (raw) {
            std::free(raw);
        }
        errorWarning(
            "u4Raw Image of size %ld does not fit anticipated size %ld",
            rawLen,
            requiredLength
        );
        return nullptr;
    }
    Image *image = Image::create(width, height, bpp <= 8, Image::SOFTWARE);
    if (!image) {
        if (raw) {
            std::free(raw);
        }
        return nullptr;
    }
    image->alphaOff();
    U4PaletteLoader paletteLoader;
    if (bpp == 8) {
        image->setPalette(paletteLoader.loadVgaPalette(), 256);
    } else if (bpp == 4) {
        image->setPalette(paletteLoader.loadEgaPalette(), 16);
    } else if (bpp == 1) {
        image->setPalette(paletteLoader.loadBWPalette(), 2);
    }
    setFromRawData(image, width, height, bpp, raw);
    std::free(raw);
    return image;
} // U4RawImageLoader::load


/**
 * Loads in the rle-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U4RleImageLoader::load(U4FILE *file, int width, int height, int bpp)
{
    if ((width == -1) || (height == -1) || (bpp == -1)) {
        errorFatal("dimensions not set for u4rle image");
    }
    U4ASSERT(
        bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32,
        "invalid bpp: %d",
        bpp
    );
    long compressedLen = file->length();
    unsigned char *compressed =
        static_cast<unsigned char *>(std::malloc(compressedLen));
    file->read(compressed, 1, compressedLen);
    unsigned char *raw = nullptr;
    long rawLen = rleDecompressMemory(compressed, compressedLen, &raw);
    std::free(compressed);
    compressed = nullptr;
    if (rawLen != (width * height * bpp / 8)) {
        if (raw) {
            std::free(raw);
        }
        return nullptr;
    }
    Image *image = Image::create(width, height, bpp <= 8, Image::SOFTWARE);
    if (!image) {
        if (raw) {
            std::free(raw);
        }
        return nullptr;
    }
    image->alphaOff();
    U4PaletteLoader paletteLoader;
    if (bpp == 8) {
        image->setPalette(paletteLoader.loadVgaPalette(), 256);
    } else if (bpp == 4) {
        image->setPalette(paletteLoader.loadEgaPalette(), 16);
    } else if (bpp == 1) {
        image->setPalette(paletteLoader.loadBWPalette(), 2);
    }
    setFromRawData(image, width, height, bpp, raw);
    std::free(raw);
    return image;
} // U4RleImageLoader::load


/**
 * Loads in the lzw-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U4LzwImageLoader::load(U4FILE *file, int width, int height, int bpp)
{
    if ((width == -1) || (height == -1) || (bpp == -1)) {
        errorFatal("dimensions not set for u4lzw image");
    }
    U4ASSERT(
        bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32,
        "invalid bpp: %d",
        bpp
    );
    long compressedLen = file->length();
    unsigned char *compressed =
        static_cast<unsigned char *>(std::malloc(compressedLen));
    file->read(compressed, 1, static_cast<std::size_t>(compressedLen));
    unsigned char *raw = nullptr;
    long rawLen = decompress_u4_memory(compressed, compressedLen, &raw);
    std::free(compressed);
    compressed = nullptr;
    if (rawLen != (width * height * bpp / 8)) {
        if (raw) {
            std::free(raw);
        }
        return nullptr;
    }
    Image *image = Image::create(width, height, bpp <= 8, Image::SOFTWARE);
    if (!image) {
        if (raw) {
            std::free(raw);
        }
        return nullptr;
    }
    image->alphaOff();
    U4PaletteLoader paletteLoader;
    if (bpp == 8) {
        image->setPalette(paletteLoader.loadVgaPalette(), 256);
    } else if (bpp == 4) {
        image->setPalette(paletteLoader.loadEgaPalette(), 16);
    } else if (bpp == 1) {
        image->setPalette(paletteLoader.loadBWPalette(), 2);
    }
    setFromRawData(image, width, height, bpp, raw);
    std::free(raw);
    return image;
} // U4LzwImageLoader::load

void U4PaletteLoader::cleanup()
{
    delete[] bwPalette;
    delete[] egaPalette;
    delete[] vgaPalette;
}

/**
 * Loads a simple black & white palette
 */
RGBA *U4PaletteLoader::loadBWPalette()
{
    if (bwPalette == nullptr) {
        bwPalette = new RGBA[2];
        bwPalette[0].r = 0;
        bwPalette[0].g = 0;
        bwPalette[0].b = 0;
        bwPalette[1].r = 255;
        bwPalette[1].g = 255;
        bwPalette[1].b = 255;
    }
    return bwPalette;
}


/**
 * Loads the basic EGA palette from egaPalette.xml
 */
RGBA *U4PaletteLoader::loadEgaPalette()
{
    if (egaPalette == nullptr) {
        int index = 0;
        const Config *config = Config::getInstance();
        egaPalette = new RGBA[16];
        std::vector<ConfigElement> paletteConf =
            config->getElement("egaPalette").getChildren();
        U4ASSERT(
            paletteConf.size() == 16, "EGA palette does not have 16 entries"
        );
        for (std::vector<ConfigElement>::const_iterator i =
                 paletteConf.cbegin();
             i != paletteConf.cend();
             ++i) {
            if (i->getName() != "color") {
                continue;
            }
            egaPalette[index].r = i->getInt("red");
            egaPalette[index].g = i->getInt("green");
            egaPalette[index].b = i->getInt("blue");
            index++;
        }
    }
    return egaPalette;
}


/**
 * Load the 256 color VGA palette from a file.
 */
RGBA *U4PaletteLoader::loadVgaPalette()
{
    if (vgaPalette == nullptr) {
        U4FILE *pal = u4fopen("u4vga.pal");
        if (!pal) {
            return nullptr;
        }
        vgaPalette = new RGBA[256];
        for (int i = 0; i < 256; i++) {
            vgaPalette[i].r = u4fgetc(pal) * 255 / 63;
            vgaPalette[i].g = u4fgetc(pal) * 255 / 63;
            vgaPalette[i].b = u4fgetc(pal) * 255 / 63;
        }
        u4fclose(pal);
    }
    return vgaPalette;
}
