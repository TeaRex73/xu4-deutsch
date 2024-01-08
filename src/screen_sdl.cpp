/*
 * $Id$
 */

#ifdef RASB_PI
#  define MY_WIDTH 416
#  ifdef PAL_TV
#    define MY_HEIGHT 288
#  else
#    define MY_HEIGHT 240
#  endif
#  define MY_TOPDIST ((MY_HEIGHT - 192) / 2)
#  define MY_LEFTDIST ((MY_WIDTH - 320) / 2)
#  define FIXUP
#else
#  define MY_WIDTH 320
#  define MY_HEIGHT 200
#  undef FIXUP
#endif

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>
#include <map>
#include <SDL.h>

#include "config.h"
#include "context.h"
#include "cursors.h"
#include "debug.h"
#include "dungeonview.h"
#include "error.h"
#include "event.h"
#include "image.h"
#include "imagemgr.h"
#include "intro.h"
#include "savegame.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "u4.h"
#include "u4_sdl.h"
#include "u4file.h"
#include "utils.h"

SDL_Cursor *cursors[5];
Scaler filterScaler;
SDL_Cursor *screenInitCursor(const char *const xpm[]);
extern bool verbose;
void screenRefreshThreadInit();
void screenRefreshThreadEnd();
void screenInit_sys();
void screenDelete_sys();
static int screenRefreshThreadFunction(void *);

static SDL_Surface *icon;

void screenInit_sys()
{
    SDL_Surface *screen, *window;
    /* start SDL */
    if (u4_SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        errorFatal("unable to init SDL: %s", SDL_GetError());
    }
    SDL_EnableUNICODE(1);
    SDL_SetGamma(
        settings.gamma / 100.0f,
        settings.gamma / 100.0f,
        settings.gamma / 100.0f
    );
    std::atexit(SDL_Quit);
    SDL_WM_SetCaption("Ultima IV", nullptr);
#ifdef ICON_FILE
    icon = SDL_LoadBMP(ICON_FILE);
    if (icon) {
        SDL_WM_SetIcon(icon, nullptr);
    }
#endif
    if (!(screen = SDL_SetVideoMode(
              MY_WIDTH * settings.scale,
              MY_HEIGHT * settings.scale,
              8,
              SDL_DOUBLEBUF
              | SDL_HWSURFACE
              | SDL_ANYFORMAT
              | (settings.fullscreen ? SDL_FULLSCREEN : 0)
          ))) {
        errorFatal("unable to set video: %s", SDL_GetError());
    }
    SDL_LockSurface(screen);
    window = SDL_CreateRGBSurfaceFrom(
        screen->pixels,
        MY_WIDTH,
        MY_HEIGHT,
        screen->format->BitsPerPixel,
        screen->pitch,
        screen->format->Rmask,
        screen->format->Gmask,
        screen->format->Bmask,
        screen->format->Amask
    );
    if (screen->format->palette) {
        SDL_SetColors(
            window,
            screen->format->palette->colors,
            0,
            screen->format->palette->ncolors
        );
    }
    SDL_FreeSurface(window);
    SDL_UnlockSurface(screen);
#ifdef FIXUP
    screen->w = 320 * settings.scale;
    screen->h = 200 * settings.scale;
    char *pix;
    pix = static_cast<char *>(screen->pixels);
    pix += (screen->pitch * MY_TOPDIST * settings.scale
            + screen->format->BytesPerPixel * MY_LEFTDIST * settings.scale);
    screen->pixels = pix;
#endif
    if (verbose) {
        char driver[32];
        std::printf(
            "screen initialized [screenInit()], using %s video driver\n",
            SDL_VideoDriverName(driver, sizeof(driver))
        );
    }
    /* enable or disable the mouse cursor */
    if (settings.mouseOptions.enabled) {
        SDL_ShowCursor(SDL_ENABLE);
        cursors[0] = SDL_GetCursor();
        cursors[1] = screenInitCursor(w_xpm);
        cursors[2] = screenInitCursor(n_xpm);
        cursors[3] = screenInitCursor(e_xpm);
        cursors[4] = screenInitCursor(s_xpm);
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }
    filterScaler = scalerGet(settings.filter);
    if (!filterScaler) {
        errorFatal("%s is not a valid filter", settings.filter.c_str());
    }
    screenRefreshThreadInit();
} // screenInit_sys

void screenDelete_sys()
{
    screenRefreshThreadEnd();
    SDL_FreeCursor(cursors[1]);
    SDL_FreeCursor(cursors[2]);
    SDL_FreeCursor(cursors[3]);
    SDL_FreeCursor(cursors[4]);
    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_FreeSurface(icon);
    icon = nullptr;
}


/**
 * Attempts to iconify the screen.
 */
void screenIconify()
{
    SDL_WM_IconifyWindow();
}

#if 0
void screenDeinterlaceCga(
    unsigned char *data, int width, int height, int tiles, int fudge
)
{
    unsigned char *tmp;
    int t, x, y;
    int tileheight = height / tiles;
    tmp = new unsigned char[width * tileheight / 4];
    for (t = 0; t < tiles; t++) {
        unsigned char *base;
        base = &(data[t * (width * tileheight / 4)]);
        for (y = 0; y < (tileheight / 2); y++) {
            for (x = 0; x < width; x += 4) {
                tmp[((y * 2) * width + x) / 4] = base[(y * width + x) / 4];
            }
        }
        for (y = tileheight / 2; y < tileheight; y++) {
            for (x = 0; x < width; x += 4) {
                tmp[(((y - (tileheight / 2)) * 2 + 1) * width + x) / 4] =
                    base[(y * width + x) / 4 + fudge];
            }
        }
        for (y = 0; y < tileheight; y++) {
            for (x = 0; x < width; x += 4) {
                base[(y * width + x) / 4] = tmp[(y * width + x) / 4];
            }
        }
    }
    delete[] tmp;
} // screenDeinterlaceCga


/**
 * Load an image from an ".pic" CGA image file.
 */
bool screenLoadImageCga(
    Image **image,
    int width,
    int height,
    U4FILE *file,
    CompressionType comp,
    int tiles
)
{
    Image *img;
    int x, y;
    unsigned char *compressed_data, *decompressed_data = nullptr;
    long inlen;
    long decompResult;
    inlen = u4flength(file);
    compressed_data = (Uint8 *)std::malloc(inlen);
    u4fread(compressed_data, 1, inlen, file);
    switch (comp) {
    case COMP_NONE:
        decompressed_data = compressed_data;
        decompResult = inlen;
        break;
    case COMP_RLE:
        decompResult = rleDecompressMemory(
            compressed_data, inlen, (void **)&decompressed_data
        );
        std::free(compressed_data);
        compressed_data = nullptr;
        break;
    case COMP_LZW:
        decompResult = decompress_u4_memory(
            compressed_data, inlen, (void **)&decompressed_data
        );
        std::free(compressed_data);
        compressed_data = nullptr;
        break;
    default:
        U4ASSERT(0, "invalid compression type %d", comp);
    }
    if (decompResult == -1) {
        if (decompressed_data) {
            std::free(decompressed_data);
        }
        compressed_data = nullptr;
        return false;
    }
    screenDeinterlaceCga(decompressed_data, width, height, tiles, 0);
    img = Image::create(width, height, true, Image::HARDWARE);
    if (!img) {
        if (decompressed_data) {
            std::free(decompressed_data);
        }
        compressed_data = nullptr;
        return false;
    }
    img->alphaOff();
    img->setPalette(egaPalette, 16);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 4) {
            img->putPixelIndex(
                x,
                y,
                decompressed_data[(y * width + x) / 4] >> 6
            );
            img->putPixelIndex(
                x + 1,
                y,
                (decompressed_data[(y * width + x) / 4] >> 4) & 0x03
            );
            img->putPixelIndex(
                x + 2,
                y,
                (decompressed_data[(y * width + x) / 4] >> 2) & 0x03
            );
            img->putPixelIndex(
                x + 3,
                y,
                (decompressed_data[(y * width + x) / 4]) & 0x03
            );
        }
    }
    std::free(decompressed_data);
    compressed_data = nullptr;
    (*image) = img;
    return true;
} // screenLoadImageCga
#endif // if 0


/**
 * Force a redraw.
 */
SDL_mutex *screenLockMutex = nullptr;
int frameDuration = 0;

void screenLock()
{
    SDL_mutexP(screenLockMutex);
}

void screenUnlock()
{
    SDL_mutexV(screenLockMutex);
}

void screenRedrawScreen()
{
    screenLock();
    SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, 0, 0);
    screenUnlock();
}

void screenRedrawTextArea(int x, int y, int width, int height)
{
    screenLock();
    SDL_UpdateRect(
        SDL_GetVideoSurface(),
        x * CHAR_WIDTH * settings.scale,
        y * CHAR_HEIGHT * settings.scale,
        width * CHAR_WIDTH * settings.scale,
        height * CHAR_HEIGHT * settings.scale
    );
    screenUnlock();
}

void screenWait(int numberOfAnimationFrames)
{
    SDL_Delay(numberOfAnimationFrames * frameDuration);
}

std::atomic_bool continueScreenRefresh(false);
SDL_Thread *screenRefreshThread = nullptr;

static int screenRefreshThreadFunction(void *)
{
    while (continueScreenRefresh) {
        SDL_Delay(frameDuration);
        screenRedrawScreen();
    }
    return 0;
}

void screenRefreshThreadInit()
{
    screenLockMutex = SDL_CreateMutex();
    frameDuration = 1000 / settings.screenAnimationFramesPerSecond;
    continueScreenRefresh = true;
    if (screenRefreshThread) {
        errorWarning("Screen refresh thread already exists.");
        return;
    }
    screenRefreshThread = SDL_CreateThread(
        screenRefreshThreadFunction, nullptr
    );
    if (!screenRefreshThread) {
        errorWarning("SDL Error: %s", SDL_GetError());
        return;
    }
}

void screenRefreshThreadEnd()
{
    continueScreenRefresh = false;
    SDL_WaitThread(screenRefreshThread, nullptr);
    screenRefreshThread = nullptr;
    SDL_UnlockMutex(screenLockMutex);
    SDL_DestroyMutex(screenLockMutex);
}


/**
 * Scale an image up.  The resulting image will be scale * the
 * original dimensions.  The original image is no longer deleted.
 * n is the number of tiles in the image; each tile is filtered
 * seperately. filter determines whether or not to filter the
 * resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter)
{
    static Scaler scalerPoint = scalerGet("Point");
    Image *dest = nullptr;
    bool isTransparent;
    unsigned int transparentIndex = 0;
    bool alpha = src->isAlphaOn();
    if (n == 0) {
        n = 1;
    }
    isTransparent = src->getTransparentIndex(transparentIndex);
    src->alphaOff();
    while (filter && filterScaler && (scale % 2 == 0)) {
        dest = (*filterScaler)(src, 2, n);
        src = dest;
        scale /= 2;
    }
    if ((scale == 3) && scaler3x(settings.filter)) {
        dest = (*filterScaler)(src, 3, n);
        src = dest;
        scale /= 3;
    }
    if (scale != 1) {
      dest = (*scalerPoint)(src, scale, n);
    }
    if (!dest) {
        dest = Image::duplicate(src);
    }
    if (isTransparent) {
        dest->setTransparentIndex(transparentIndex);
    }
    if (alpha) {
        src->alphaOn();
    }
    return dest;
} // screenScale


/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is no longer deleted.
 */
Image *screenScaleDown(Image *src, int scale)
{
    int x, y;
    Image *dest;
    bool isTransparent;
    unsigned int transparentIndex;
    bool alpha = src->isAlphaOn();
    isTransparent = src->getTransparentIndex(transparentIndex);
    src->alphaOff();
    dest = Image::create(
        src->width() / scale,
        src->height() / scale,
        src->isIndexed(),
        Image::HARDWARE
    );
    if (!dest) {
        return nullptr;
    }
    dest->alphaOff();
    if (dest->isIndexed()) {
        dest->setPaletteFromImage(src);
    }
    for (y = 0; y < src->height(); y += scale) {
        for (x = 0; x < src->width(); x += scale) {
            unsigned int index;
            src->getPixelIndex(x, y, index);
            dest->putPixelIndex(x / scale, y / scale, index);
        }
    }
    if (isTransparent) {
        dest->setTransparentIndex(transparentIndex);
    }
    if (alpha) {
        src->alphaOn();
    }
    return dest;
} // screenScaleDown


/**
 * Create an SDL cursor object from an xpm.  Derived from example in
 * SDL documentation project.
 */
#define CURSORSIZE 32

SDL_Cursor *screenInitCursor(const char *const xpm[])
{
    int i, row, col;
    Uint8 data[(CURSORSIZE / 8) * CURSORSIZE];
    Uint8 mask[(CURSORSIZE / 8) * CURSORSIZE];
    int hot_x, hot_y;
    i = -1;
    for (row = 0; row < CURSORSIZE; row++) {
        for (col = 0; col < CURSORSIZE; col++) {
            if (col % 8) {
                data[i] <<= 1;
                mask[i] <<= 1;
            } else {
                i++;
                data[i] = mask[i] = 0;
            }
            switch (xpm[4 + row][col]) {
            case 'X':
                data[i] |= 0x01;
                mask[i] |= 0x01;
                break;
            case '.':
                mask[i] |= 0x01;
                break;
            case ' ':
                break;
            }
        }
    }
    std::sscanf(xpm[4 + row], "%d,%d", &hot_x, &hot_y);
    return SDL_CreateCursor(data, mask, CURSORSIZE, CURSORSIZE, hot_x, hot_y);
} // screenInitCursor

void screenSetMouseCursor(MouseCursor cursor)
{
    static int current = 0;
    if (cursor != current) {
        SDL_SetCursor(cursors[cursor]);
        current = cursor;
    }
}
