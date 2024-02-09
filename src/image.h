/*
 * $Id$
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <cstdint>
#include "types.h"
#include "u4file.h"
#include "textcolor.h"

struct SDL_Surface;
typedef SDL_Surface *BackendSurface;


#define DARK_GRAY_HALO RGBA(14, 15, 16, 255)

class alignas(int) RGBA {
public:
    RGBA(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
        :r(r), g(g), b(b), a(a)
    {
    }

    RGBA()
        :r(0), g(0), b(0), a(255)
    {
    }

    std::uint8_t r, g, b, a;
};

bool operator==(const RGBA &lhs, const RGBA &rhs);

class Image;

class SubImage {
public:
    SubImage()
        :name(), srcImageName(), x(0), y(0), width(0), height(0)
    {
    }

    std::string name;
    std::string srcImageName;
    int x, y, width, height;
};

#define IM_OPAQUE 255u
#define IM_TRANSPARENT 0u


/**
 * A simple image object that can be drawn and read/written to at the
 * pixel level.
 * @todo
 *  <ul>
 *      <li>drawing methods should be pushed to Drawable subclass</li>
 *  </ul>
 */
class Image {
public:
    // disallow assignments, copy contruction
    Image(const Image &) = delete;
    Image(Image &&) = delete;
    const Image &operator=(const Image &) = delete;
    const Image &operator=(Image &&) = delete;

    enum Type {
        HARDWARE,
        SOFTWARE
    };

    static Image *create(int w, int h, bool indexed, Type type);
    static Image *createScreenImage();
    static Image *duplicate(Image *image);
    ~Image();
    /* palette handling */
    void setPalette(const RGBA *colors, unsigned int n_colors);
    void setPaletteFromImage(const Image *src);
    bool getTransparentIndex(unsigned int &index) const;
    void performTransparencyHack(
        unsigned int colorValue,
        unsigned int numFrames,
        unsigned int currentFrameIndex,
        unsigned int haloWidth,
        unsigned int hoibpd // haloOpacityIncrementByPixelDistance
    );
    void setTransparentIndex(unsigned int index);
    bool setFontColor(ColorFG fg, ColorBG bg);
    bool setFontColorFG(ColorFG fg);
    bool setFontColorBG(ColorBG bg);
    // returns the color of the specified palette index
    RGBA getPaletteColor(int index) const;
    // sets the specified palette index to the specified RGB color
    bool setPaletteIndex(unsigned int index, RGBA color);
    // returns the palette index of the specified RGB color
    int getPaletteIndex(RGBA color) const;
    static RGBA setColor(
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b,
        std::uint8_t a = IM_OPAQUE
    );
    /* alpha handling */
    bool isAlphaOn() const;
    void alphaOn();
    void alphaOff();
    /* Will clear the image to the background color,
       and set the internal backgroundColor variable */
    void initializeToBackgroundColor(
        RGBA backgroundColor = DARK_GRAY_HALO
    );
    /* Will make the pixels that match the background
       color disappear, with a blur halo */
    void makeBackgroundColorTransparent(
        int haloSize = 0, int shadowOpacity = 255
    );

    /* writing to image */


    /**
     * Sets the color of a single pixel.
     */
    void putPixel(
        int x,
        int y,
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b,
        std::uint8_t a,
        bool anyway = false
    ) const; // TODO Consider using &
    void putPixelIndex(
        int x, int y, unsigned int index, bool anyway = false
    ) const;
    void fillRect(
        int x,
        int y,
        int w,
        int h,
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b,
        std::uint8_t a = IM_OPAQUE,
        bool anyway = false
    ) const;
    /* reading from image */
    void getPixel(
        int x,
        int y,
        std::uint8_t &r,
        std::uint8_t &g,
        std::uint8_t &b,
        std::uint8_t &a
    ) const;
    void getPixelIndex(int x, int y, unsigned int &index) const;

    /* image drawing methods */


    /**
     * Draws the entire image onto the screen at the given offset.
     */
    void draw(int x, int y) const
    {
        drawOn(nullptr, x, y);
    }


    /**
     * Draws a piece of the image onto the screen at the given offset.
     * The area of the image to draw is defined by the rectangle rx, ry,
     * rw, rh.
     */
    void drawSubRect(int x, int y, int rx, int ry, int rw, int rh) const
    {
        drawSubRectOn(nullptr, x, y, rx, ry, rw, rh);
    }


    /**
     * Draws a piece of the image onto the screen, inverted.
     * The area of the image to draw is defined by the rectangle rx, ry,
     * rw, rh.
     */
    void drawSubRectInverted(
        int x, int y, int rx, int ry, int rw, int rh
    ) const
    {
        drawSubRectInvertedOn(nullptr, x, y, rx, ry, rw, rh);
    }

    /* image drawing methods for drawing onto another image instead of
       the screen */
    void drawOn(Image *d, int x, int y, bool anyway = false) const;
    void drawSubRectOn(
        Image *d,
        int x,
        int y,
        int rx,
        int ry,
        int rw,
        int rh,
        bool anyway = false
    ) const;
    void drawSubRectInvertedOn(
        Image *d,
        int x,
        int y,
        int rx,
        int ry,
        int rw,
        int rh,
        bool anyway = false
    ) const;

    int width() const
    {
        return w;
    }

    int height() const
    {
        return h;
    }

    bool isIndexed() const
    {
        return indexed;
    }

    BackendSurface getSurface()
    {
        return surface;
    }

    void save(const std::string &filename);
    void drawHighlighted() const;

private:
    unsigned int w, h;
    bool indexed;
    RGBA backgroundColor;
    Image(); /* use create method to construct images */
    BackendSurface surface;
    bool isScreen;
};

#endif /* IMAGE_H */
