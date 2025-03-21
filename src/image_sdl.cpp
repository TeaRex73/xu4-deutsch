/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <list>
#include <utility>

#include <cstdarg>

#include <SDL.h>

#include "debug.h"
#include "image.h"
#include "imagemgr.h"
#include "screen.h"
#include "settings.h"
#include "error.h"

Image::Image()
    :w(0),
     h(0),
     indexed(false),
     backgroundColor(),
     surface(nullptr),
     isScreen(false)
{
}


/**
 * Creates a new image.  Scale is stored to allow drawing using U4
 * (320x200) coordinates, regardless of the actual image scale.
 * Indexed is true for palette based images, or false for RGB images.
 * Image type determines whether to create a hardware (i.e. video ram)
 * or software (i.e. normal ram) image.
 */
Image *Image::create(int w, int h, bool indexed, Image::Type type)
{
    Uint32 rmask, gmask, bmask, amask;
    Uint32 flags;
    Image *im = new Image;
    im->w = w;
    im->h = h;
    im->indexed = indexed;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    if (type == Image::HARDWARE) {
        flags = SDL_HWSURFACE /* | SDL_SRCALPHA */;
    } else {
        flags = SDL_SWSURFACE /* | SDL_SRCALPHA */;
    }
    if (indexed) {
        im->surface = SDL_CreateRGBSurface(
            flags, w, h, 8, rmask, gmask, bmask, amask
        );
    } else {
        im->surface = SDL_CreateRGBSurface(
            flags, w, h, 32, rmask, gmask, bmask, amask
        );
    }
    if (!im->surface) {
        delete im;
        return nullptr;
    }
    return im;
} // Image::create


/**
 * Create a special purpose image the represents the whole screen.
 */
Image *Image::createScreenImage()
{
    Image *screen = new Image();

    screen->surface = SDL_GetVideoSurface();
    U4ASSERT(
        screen->surface != nullptr,
        "SDL_GetVideoSurface() returned a nullptr screen surface!"
    );
    screen->w = screen->surface->w;
    screen->h = screen->surface->h;
    screen->indexed = screen->surface->format->palette != nullptr;
    screen->isScreen = true;
    return screen;
}


/**
 * Creates a duplicate of another image
 */
Image *Image::duplicate(Image *image)
{
    bool alphaState = image->isAlphaOn();
    Image *im = create(image->width(), image->height(), false, SOFTWARE);
    /* Turn alpha off before blitting to non-screen surfaces */
    if (alphaState) {
        image->alphaOff();
    }
    image->drawOn(im, 0, 0);
    if (alphaState) {
        image->alphaOn();
    }
    im->backgroundColor = image->backgroundColor;
    return im;
}


/**
 * Frees the image.
 */
Image::~Image()
{
    if (!isScreen) {
        SDL_FreeSurface(surface);
    }
}


/**
 * Sets the palette
 */
void Image::setPalette(const RGBA *colors, unsigned int n_colors)
{
    U4ASSERT(indexed, "imageSetPalette called on non-indexed image");
    if (n_colors > 256) {
        errorFatal("n_colors > 256 in Image::setPalette");
    }
    SDL_Color *sdlcolors = new SDL_Color[n_colors];
    for (unsigned int i = 0; i < n_colors; i++) {
        sdlcolors[i].r = colors[i].r;
        sdlcolors[i].g = colors[i].g;
        sdlcolors[i].b = colors[i].b;
        sdlcolors[i].unused = 0;
    }
    SDL_SetColors(surface, sdlcolors, 0, n_colors);
    delete[] sdlcolors;
}


/**
 * Copies the palette from another image.
 */
void Image::setPaletteFromImage(const Image *src)
{
    U4ASSERT(
        indexed && src->indexed,
        "imageSetPaletteFromImage called on non-indexed image"
    );
    for (int i = 0; i < src->surface->format->palette->ncolors; i++) {
        surface->format->palette->colors[i] =
            src->surface->format->palette->colors[i];
    }
}


// returns the color of the specified palette index
RGBA Image::getPaletteColor(int index) const
{
    RGBA color = RGBA(0, 0, 0, 0);
    if (indexed) {
        color.r = surface->format->palette->colors[index].r;
        color.g = surface->format->palette->colors[index].g;
        color.b = surface->format->palette->colors[index].b;
        color.a = IM_OPAQUE;
    }
    return color;
}


/* returns the palette index of the specified RGB color */
int Image::getPaletteIndex(RGBA color) const
{
    if (!indexed) {
        return -1;
    }
    for (int i = 0; i < surface->format->palette->ncolors; i++) {
        if ((surface->format->palette->colors[i].r == color.r)
            && (surface->format->palette->colors[i].g == color.g)
            && (surface->format->palette->colors[i].b == color.b)) {
            return i;
        }
    }
    // return the proper palette index for the specified color
    return -1;
}

RGBA Image::setColor(
    std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a
)
{
    RGBA color = RGBA(r, g, b, a);
    return color;
}


/* sets the specified font colors */
bool Image::setFontColor(ColorFG fg, ColorBG bg)
{
    if (!setFontColorFG(fg)) {
        return false;
    }
    if (!setFontColorBG(bg)) {
        return false;
    }
    return true;
}


/* sets the specified font colors */
bool Image::setFontColorFG(ColorFG fg)
{
    switch (fg) {
    case FG_GREY:
        if (!setPaletteIndex(
                TEXT_FG_PRIMARY_INDEX, setColor(153, 153, 153)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SECONDARY_INDEX, setColor(102, 102, 102)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SHADOW_INDEX, setColor(51, 51, 51)
            )) {
            return false;
        }
        break;
    case FG_BLUE:
        if (!setPaletteIndex(
                TEXT_FG_PRIMARY_INDEX, setColor(102, 102, 255)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SECONDARY_INDEX, setColor(51, 51, 204)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SHADOW_INDEX, setColor(51, 51, 51)
            )) {
            return false;
        }
        break;
    case FG_PURPLE:
        if (!setPaletteIndex(
                TEXT_FG_PRIMARY_INDEX, setColor(255, 102, 255)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SECONDARY_INDEX, setColor(204, 51, 204)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SHADOW_INDEX, setColor(51, 51, 51)
            )) {
            return false;
        }
        break;
    case FG_GREEN:
        if (!setPaletteIndex(
                TEXT_FG_PRIMARY_INDEX, setColor(102, 255, 102)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SECONDARY_INDEX, setColor(0, 153, 0)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SHADOW_INDEX, setColor(51, 51, 51)
            )) {
            return false;
        }
        break;
    case FG_RED:
        if (!setPaletteIndex(
                TEXT_FG_PRIMARY_INDEX, setColor(255, 102, 102)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SECONDARY_INDEX, setColor(204, 51, 51)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SHADOW_INDEX, setColor(51, 51, 51)
            )) {
            return false;
        }
        break;
    case FG_YELLOW:
        if (!setPaletteIndex(
                TEXT_FG_PRIMARY_INDEX, setColor(255, 255, 51)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SECONDARY_INDEX, setColor(204, 153, 51)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SHADOW_INDEX, setColor(51, 51, 51)
            )) {
            return false;
        }
        break;
    default:
        if (!setPaletteIndex(
                TEXT_FG_PRIMARY_INDEX, setColor(255, 255, 255)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SECONDARY_INDEX, setColor(204, 204, 204)
            )) {
            return false;
        }
        if (!setPaletteIndex(
                TEXT_FG_SHADOW_INDEX, setColor(68, 68, 68)
            )) {
            return false;
        }
    } // switch
    return true;
} // Image::setFontColorFG


/* sets the specified font colors */
bool Image::setFontColorBG(ColorBG bg)
{
    switch (bg) {
    case BG_BRIGHT:
        if (!setPaletteIndex(TEXT_BG_INDEX, setColor(0, 0, 102))) {
            return false;
        }
        break;
    default:
        if (!setPaletteIndex(TEXT_BG_INDEX, setColor(0, 0, 0))) {
            return false;
        }
    }
    return true;
}


/* sets the specified palette index to the specified RGB color */
bool Image::setPaletteIndex(unsigned int index, RGBA color)
{
    if (!indexed) {
        return false;
    }
    surface->format->palette->colors[index].r = color.r;
    surface->format->palette->colors[index].g = color.g;
    surface->format->palette->colors[index].b = color.b;
    // success
    return true;
}

bool Image::getTransparentIndex(unsigned int &index) const
{
    if (!indexed || ((surface->flags & SDL_SRCCOLORKEY) == 0)) {
        return false;
    }
    index = surface->format->colorkey;
    return true;
}

void Image::initializeToBackgroundColor(RGBA backgroundColor)
{
    U4ASSERT(!indexed, "Indexed not supported");
    this->backgroundColor = backgroundColor;
    this->fillRect(
        0,
        0,
        this->w,
        this->h,
        backgroundColor.r,
        backgroundColor.g,
        backgroundColor.b,
        backgroundColor.a
    );
}

bool Image::isAlphaOn() const
{
    return surface->flags & SDL_SRCALPHA;
}

void Image::alphaOn()
{
    surface->flags |= SDL_SRCALPHA;
}

void Image::alphaOff()
{
    surface->flags &= ~SDL_SRCALPHA;
}

void Image::putPixel(
    int x,
    int y,
    std::uint8_t r,
    std::uint8_t g,
    std::uint8_t b,
    std::uint8_t a,
    bool anyway
) const
{
    putPixelIndex(
        x,
        y,
        SDL_MapRGBA(surface->format, Uint8(r), Uint8(g), Uint8(b), Uint8(a)),
        anyway
    );
}

void Image::makeBackgroundColorTransparent(int haloSize, int shadowOpacity)
{
    int bgColor = SDL_MapRGBA(
        surface->format,
        Uint8(backgroundColor.r),
        Uint8(backgroundColor.g),
        Uint8(backgroundColor.b),
        Uint8(backgroundColor.a)
    );
    performTransparencyHack(bgColor, 1, 0, haloSize, shadowOpacity);
}


// TODO Separate functionalities found in here
void Image::performTransparencyHack(
    unsigned int colorValue,
    unsigned int numFrames,
    unsigned int currentFrameIndex,
    unsigned int haloWidth,
    unsigned int hoibpd
)
{
    std::list<std::pair<unsigned int, unsigned int> > opaqueXYs;
    unsigned int x, y;
    Uint8 t_r, t_g, t_b;
    SDL_GetRGB(colorValue, surface->format, &t_r, &t_g, &t_b);
    unsigned int frameHeight = h / numFrames;
    // Min'd so that they never go out of range (>=h)
    unsigned int top = std::min(h, currentFrameIndex * frameHeight);
    unsigned int bottom = std::min(h, top + frameHeight);
    for (y = top; y < bottom; y++) {
        for (x = 0; x < w; x++) {
            std::uint8_t r, g, b, a;
            getPixel(x, y, r, g, b, a);
            if ((r == t_r) && (g == t_g) && (b == t_b)) {
                putPixel(x, y, r, g, b, IM_TRANSPARENT);
            } else {
                putPixel(x, y, r, g, b, a);
                if (haloWidth) {
                    opaqueXYs.push_back(
                        std::pair<unsigned int, unsigned int>(x, y)
                    );
                }
            }
        }
    }
    for (std::list<std::pair<unsigned int, unsigned int> >::iterator xy
             = opaqueXYs.begin();
         xy != opaqueXYs.end();
         ++xy) {
        int ox = xy->first;
        int oy = xy->second;
        int span = int(haloWidth);
        unsigned int x_start = std::max(0, ox - span);
        unsigned int x_finish = std::min(int(w), ox + span + 1);
        for (x = x_start; x < x_finish; ++x) {
            unsigned int y_start = std::max(int(top), oy - span);
            unsigned int y_finish = std::min(int(bottom), oy + span + 1);
            for (y = y_start; y < y_finish; ++y) {
                int divisor =
                    1
                    + span * 2
                    - std::abs(static_cast<int>(ox - x))
                    - std::abs(static_cast<int>(oy - y));
                std::uint8_t r, g, b, a;
                getPixel(x, y, r, g, b, a);
                if (a != IM_OPAQUE) {
                    putPixel(
                        x,
                        y,
                        r,
                        g,
                        b,
                        std::min(IM_OPAQUE, a + hoibpd / divisor)
                    );
                }
            }
        }
    }
} // Image::performTransparencyHack

void Image::setTransparentIndex(unsigned int index)
{
    if (indexed) {
        SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, index);
    } else {
        // errorWarning("Setting transparent index for non indexed");
    }
}


/**
 * Sets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::putPixelIndex(int x, int y, unsigned int index, bool anyway) const
{
    int bpp;
    Uint8 *p;
    Uint16 *p2;
    Uint32 *p4;
    if (!__builtin_expect(screenMoving, true) && isScreen && !anyway) {
        return;
    }
    bpp = surface->format->BytesPerPixel;
    switch (__builtin_expect(bpp, 1)) {
    case 1:
        p = static_cast<Uint8 *>(surface->pixels)
        + y * surface->pitch
        + x;
        *p = index;
        break;
    case 2:
        p2 = static_cast<Uint16 *>(surface->pixels)
            + (y * surface->pitch) / 2
            + x;
        *p2 = index;
        break;
    case 3:
        p = static_cast<Uint8 *>(surface->pixels)
            + y * surface->pitch
            + x * 3;
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (index >> 16) & 0xff;
            p[1] = (index >> 8) & 0xff;
            p[2] = index & 0xff;
        } else {
            p[0] = index & 0xff;
            p[1] = (index >> 8) & 0xff;
            p[2] = (index >> 16) & 0xff;
        }
        break;
    case 4:
        p4 = static_cast<Uint32 *>(surface->pixels)
            + (y * surface->pitch) / 4
            + x;
        *p4 = index;
        break;
    }
} // Image::putPixelIndex


/**
 * Fills a rectangle in the image with a given color.
 */
void Image::fillRect(
    int x,
    int y,
    int w,
    int h,
    std::uint8_t r,
    std::uint8_t g,
    std::uint8_t b,
    std::uint8_t a,
    bool anyway
) const
{
    SDL_Rect dest;
    Uint32 pixel;
    pixel = SDL_MapRGBA(
        surface->format, Uint8(r), Uint8(g), Uint8(b), Uint8(a)
    );
    dest.x = x;
    dest.y = y;
    dest.w = w;
    dest.h = h;
    if (__builtin_expect(screenMoving, true) || !isScreen || anyway) {
        SDL_FillRect(surface, &dest, pixel);
    }
}


/**
 * Gets the color of a single pixel.
 */
void Image::getPixel(
    int x,
    int y,
    std::uint8_t &r,
    std::uint8_t &g,
    std::uint8_t &b,
    std::uint8_t &a
) const
{
    unsigned int index = 0;
    Uint8 r1, g1, b1, a1;
    getPixelIndex(x, y, index);
    SDL_GetRGBA(index, surface->format, &r1, &g1, &b1, &a1);
    r = r1;
    g = g1;
    b = b1;
    a = a1;
}


/**
 * Gets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::getPixelIndex(int x, int y, unsigned int &index) const
{
    const Uint8 *p1, *p3;
    const Uint16 *p2;
    const Uint32 *p4;
    int bpp = surface->format->BytesPerPixel;
    switch (__builtin_expect(bpp, 1)) {
    case 1:
        p1 = static_cast<Uint8 *>(surface->pixels)
            + y * surface->pitch
            + x;
        index = *p1;
        break;
    case 2:
        p2 = static_cast<Uint16 *>(surface->pixels)
            + (y * surface->pitch) / 2
            + x;
        index = *p2;
        break;
    case 3:
        p3 = static_cast<Uint8 *>(surface->pixels)
            + y * surface->pitch
            + x * 3;
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            index = p3[0] << 16 | p3[1] << 8 | p3[2];
        } else {
            index = p3[0] | p3[1] << 8 | p3[2] << 16;
        }
        break;
    case 4:
        p4 = static_cast<Uint32 *>(surface->pixels)
            + (y * surface->pitch) / 4
            + x;
        index = *p4;
        break;
    default:
        index = 0;
    }
}


/**
 * Draws the image onto another image.
 */
void Image::drawOn(Image *d, int x, int y, bool anyway) const
{
    SDL_Rect r;
    SDL_Surface *destSurface;
    if (d == nullptr) {
        destSurface = SDL_GetVideoSurface();
    } else {
        destSurface = d->surface;
    }
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    if (__builtin_expect(screenMoving, true) || (d && !d->isScreen) || anyway) {
        SDL_BlitSurface(surface, nullptr, destSurface, &r);
    }
}


/**
 * Draws a piece of the image onto another image.
 */
void Image::drawSubRectOn(
    Image *d,
    int x,
    int y,
    int rx,
    int ry,
    int rw,
    int rh,
    bool anyway
) const
{
    SDL_Rect src, dest;
    SDL_Surface *destSurface;
    if (__builtin_expect(d == nullptr, true)) {
        destSurface = SDL_GetVideoSurface();
    } else {
        destSurface = d->surface;
    }
    src.x = rx;
    src.y = ry;
    src.w = rw;
    src.h = rh;
    dest.x = x;
    dest.y = y;
    /* dest w & h unused */
    if (__builtin_expect(screenMoving, true) || (d && !d->isScreen) || anyway) {
        SDL_BlitSurface(surface, &src, destSurface, &dest);
    }
} // Image::drawSubRectOn


/**
 * Draws a piece of the image onto another image, inverted.
 */
void Image::drawSubRectInvertedOn(
    Image *d,
    int x,
    int y,
    int rx,
    int ry,
    int rw,
    int rh,
    bool anyway
) const
{
    int i;
    SDL_Rect src, dest;
    SDL_Surface *destSurface;
    if (d == nullptr) {
        destSurface = SDL_GetVideoSurface();
    } else {
        destSurface = d->surface;
    }
    for (i = 0; i < rh; i++) {
        src.x = rx;
        src.y = ry + i;
        src.w = rw;
        src.h = 1;
        dest.x = x;
        dest.y = y + rh - i - 1;
        /* dest w & h unused */
        if (__builtin_expect(screenMoving, true) || (d && !d->isScreen) || anyway) {
            SDL_BlitSurface(surface, &src, destSurface, &dest);
        }
    }
} // Image::drawSubRectInvertedOn


/**
 * Dumps the image to a file.  The file is always saved in .bmp
 * format.  This is mainly used for debugging.
 */
void Image::save(const std::string &filename)
{
    SDL_SaveBMP(surface, filename.c_str());
}


void Image::drawHighlighted() const
{
    RGBA c;
    for (unsigned int i = 0; i < h; i++) {
        for (unsigned int j = 0; j < w; j++) {
            getPixel(j, i, c.r, c.g, c.b, c.a);
            putPixel(j, i, 0xff - c.r, 0xff - c.g, 0xff - c.b, c.a);
        }
    }
}
