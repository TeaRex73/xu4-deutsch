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
#include "imageloader_fmtowns.h"
#include "imageloader_u4.h"
#include "lzw/u4decode.h"

using std::vector;

ImageLoader *FMTOWNSImageLoader::instance_tif = ImageLoader::registerLoader(
    new FMTOWNSImageLoader(510), "image/fmtowns-tif"
);


/**
 * Loads in an FM TOWNS files, which we assume is 16 bits.
 */
Image *FMTOWNSImageLoader::load(U4FILE *file, int width, int height, int bpp)
{
    if ((width == -1) || (height == -1) || (bpp == -1)) {
        errorFatal("dimensions not set for fmtowns image");
    }
    ASSERT((bpp == 16) | (bpp == 4), "invalid bpp: %d", bpp);
    long rawLen = file->length() - offset;
    file->seek(offset, 0);
    unsigned char *raw = (unsigned char *)malloc(rawLen);
    file->read(raw, 1, rawLen);
    long requiredLength = (width * height * bpp / 8);
    if (rawLen < requiredLength) {
        if (raw) {
            free(raw);
        }
        errorWarning(
            "FMTOWNS Image of size %ld does not fit anticipated size %ld",
            rawLen,
            requiredLength
        );
        return NULL;
    }
    Image *image = Image::create(
        width, height, bpp <= 8, Image::HARDWARE
    );
    if (!image) {
        if (raw) {
            free(raw);
        }
        return NULL;
    }
    if (bpp == 4) {
        U4PaletteLoader pal;
        image->setPalette(pal.loadEgaPalette(), 16);
        setFromRawData(image, width, height, bpp, raw);
    }
    if (bpp == 16) {
        unsigned char low5 = 0x1F;
        unsigned char high6 = ~3;
        unsigned char high3 = ~31;
        unsigned char low2 = 3;
        unsigned char lastbit = 128;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char byte0 =
                    raw[(y * width + x) * 2];
                unsigned char byte1 =
                    raw[(y * width + x) * 2 + 1];
                int r = (byte0 & low5);
                r <<= 3;
                int g = (byte0 & high3) >> 5;
                g |= ((byte1 & low2) << 3);
                g <<= 3;
                int b = byte1 & high6;
                b <<= 1;
                image->putPixel(
                    x, y, g, b, r, lastbit & byte1 ? IM_TRANSPARENT : IM_OPAQUE
                );
            }
        }
    }
    free(raw);
    return image;
} // FMTOWNSImageLoader::load
