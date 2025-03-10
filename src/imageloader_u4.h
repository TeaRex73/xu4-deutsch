/*
 * $Id$
 */

#ifndef IMAGELOADER_U4_H
#define IMAGELOADER_U4_H

#include "imageloader.h"

struct RGBA;


/**
 * Loader for U4 raw images.  Raw images are just an uncompressed
 * stream of pixel data with no palette information (e.g. shapes.ega,
 * charset.ega).  This loader handles the original 4-bit images, as
 * well as the 8-bit VGA upgrade images.
 */
class U4RawImageLoader:public ImageLoader {
public:
    virtual Image *load(U4FILE *file, int width, int height, int bpp) override;

private:
    static ImageLoader *instance;
};


/**
 * Loader for U4 images with RLE compression.  Like raw images, the
 * data is just a stream of pixel data with no palette information
 * (e.g. start.ega, rune_*.ega).  This loader handles the original
 * 4-bit images, as well as the 8-bit VGA upgrade images.
 */
class U4RleImageLoader:public ImageLoader {
public:
    virtual Image *load(U4FILE *file, int width, int height, int bpp) override;

private:
    static ImageLoader *instance;
};


/**
 * Loader for U4 images with LZW compression.  Like raw images, the
 * data is just a stream of pixel data with no palette information
 * (e.g. title.ega, tree.ega).  This loader handles the original 4-bit
 * images, as well as the 8-bit VGA upgrade images.
 */
class U4LzwImageLoader:public ImageLoader {
public:
    virtual Image *load(U4FILE *file, int width, int height, int bpp) override;

private:
    static ImageLoader *instance;
};

class U4PaletteLoader {
public:
    static void cleanup();
    static RGBA *loadBWPalette();
    static RGBA *loadEgaPalette();
    static RGBA *loadVgaPalette();

private:
    static RGBA *bwPalette;
    static RGBA *egaPalette;
    static RGBA *vgaPalette;

};

#endif /* IMAGELOADER_U4_H */
