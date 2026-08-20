/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>
#include <cstdlib>
#include <string>

#include "imageloader_fmtowns.h"

#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_u4.h"
#include "u4file.h"


ImageLoader *FMTOWNSImageLoader::instance_tif = registerLoader(
    new FMTOWNSImageLoader(510), "image/fmtowns-tif"
);

/**
 * Loads in an FM TOWNS files, which we assume is 16 bits.
 */
Image *FMTOWNSImageLoader::load(
    U4FILE *file, const int width, const int height, const int bpp
)
{
    if (width == -1 || height == -1 || bpp == -1) {
        errorFatal("dimensions not set for fmtowns image");
    }
    U4ASSERT(bpp == 16 || bpp == 4, "invalid bpp: %d", bpp);
    const long rawLen = file->length() - offset;
    file->seek(offset, SEEK_SET);
    auto *raw = static_cast<unsigned char *>(std::malloc(rawLen));
    file->read(raw, 1, rawLen);
    const long requiredLength = width * height * bpp / 8;
    if (rawLen < requiredLength) {
        if (raw) {
            std::free(raw);
        }
        errorWarning(
            "FMTOWNS Image of size %ld does not fit anticipated size %ld",
            rawLen,
            requiredLength
        );
        return nullptr;
    }
    Image *image = Image::create(
        width, height, bpp <= 8, Image::SOFTWARE
    );
    if (!image) {
        if (raw) {
            std::free(raw);
        }
        return nullptr;
    }
    if (bpp == 4) {
        image->setPalette(U4PaletteLoader::loadEgaPalette(), 16);
        setFromRawData(image, width, height, bpp, raw);
    }
    if (bpp == 16) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                constexpr unsigned char last_bit = 128;
                constexpr unsigned char low2 = 3;
                constexpr unsigned char high3 = ~31;
                constexpr unsigned char high6 = ~3;
                constexpr unsigned char low5 = 0x1F;
                const unsigned char byte0 =
                    raw[(y * width + x) * 2];
                const unsigned char byte1 =
                    raw[(y * width + x) * 2 + 1];
                int r = byte0 & low5;
                r <<= 3;
                int g = (byte0 & high3) >> 5;
                g |= (byte1 & low2) << 3;
                g <<= 3;
                int b = byte1 & high6;
                b <<= 1;
                image->putPixel(
                    x,
                    y,
                    g + 0,
                    b + 0,
                    r + 0,
                    last_bit & byte1 ? IM_TRANSPARENT : IM_OPAQUE
                );
            }
        }
    }
    std::free(raw);
    return image;
} // FMTOWNSImageLoader::load
