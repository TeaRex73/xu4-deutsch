/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "debug.h"
#include "image.h"
#include "imageloader.h"

std::map<std::string, ImageLoader *> *ImageLoader::loaderMap = nullptr;


/**
 * This class method returns the registered concrete subclass
 * appropriate for loading images of a type given by fileType.
 */
ImageLoader *ImageLoader::getLoader(const std::string &fileType)
{
    U4ASSERT(
        loaderMap != nullptr,
        "ImageLoader::getLoader loaderMap not initialized"
    );
    if (loaderMap->find(fileType) == loaderMap->end()) {
        return nullptr;
    }
    return (*loaderMap)[fileType];
}


/**
 * Register an image loader.  Concrete subclasses should register an
 * instance at startup.  This method is safe to call from a global
 * object constructor or static initializer.
 */
ImageLoader *ImageLoader::registerLoader(
    ImageLoader *loader, const std::string &type
)
{
    if (loaderMap == nullptr) {
        loaderMap = new std::map<std::string, ImageLoader *>;
    }
    (*loaderMap)[type] = loader;
    return loader;
}

void ImageLoader::cleanup()
{
    for (std::map<std::string, ImageLoader *>::iterator i =
             loaderMap->begin();
         i != loaderMap->end();
         ++i) {
        delete i->second;
    }
    delete loaderMap;
}

/**
 * Fill in the image pixel data from an uncompressed std::string of bytes.
 */
void ImageLoader::setFromRawData(
    const Image *image,
    int width,
    int height,
    int bpp,
    const unsigned char *rawData
)
{
    int x, y;
    switch (__builtin_expect(bpp, 4)) {
    case 32:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                image->putPixel(
                    x,
                    y,
                    rawData[(y * width + x) * 4],
                    rawData[(y * width + x) * 4 + 1],
                    rawData[(y * width + x) * 4 + 2],
                    rawData[(y * width + x) * 4 + 3]
                );
            }
        }
        break;
    case 24:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                image->putPixel(
                    x,
                    y,
                    rawData[(y * width + x) * 3],
                    rawData[(y * width + x) * 3 + 1],
                    rawData[(y * width + x) * 3 + 2],
                    IM_OPAQUE
                );
            }
        }
        break;
    case 8:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                image->putPixelIndex(
                    x,
                    y,
                    rawData[y * width + x]
                );
            }
        }
        break;
    case 4:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x += 2) {
                image->putPixelIndex(
                    x,
                    y,
                    rawData[(y * width + x) / 2] >> 4
                );
                image->putPixelIndex(
                    x + 1,
                    y,
                    rawData[(y * width + x) / 2] & 0x0f
                );
            }
        }
        break;
    case 1:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x += 8) {
                image->putPixelIndex(
                    x + 0,
                    y,
                    (rawData[(y * width + x) / 8] >> 7) & 0x01
                );
                image->putPixelIndex(
                    x + 1,
                    y,
                    (rawData[(y * width + x) / 8] >> 6) & 0x01
                );
                image->putPixelIndex(
                    x + 2,
                    y,
                    (rawData[(y * width + x) / 8] >> 5) & 0x01
                );
                image->putPixelIndex(
                    x + 3,
                    y,
                    (rawData[(y * width + x) / 8] >> 4) & 0x01
                );
                image->putPixelIndex(
                    x + 4,
                    y,
                    (rawData[(y * width + x) / 8] >> 3) & 0x01
                );
                image->putPixelIndex(
                    x + 5,
                    y,
                    (rawData[(y * width + x) / 8] >> 2) & 0x01
                );
                image->putPixelIndex(
                    x + 6,
                    y,
                    (rawData[(y * width + x) / 8] >> 1) & 0x01
                );
                image->putPixelIndex(
                    x + 7,
                    y,
                    (rawData[(y * width + x) / 8] >> 0) & 0x01
                );
            }
        }
        break;
    default:
        U4ASSERT(0, "invalid bits-per-pixel (bpp): %d", bpp);
    } // switch
} // ImageLoader::setFromRawData
