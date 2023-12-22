/*
 * $Id$
 */
/**
 * @file
 * @brief Declares interfaces for working with the screen
 *
 * This file declares interfaces for manipulating areas of the screen, or the
 * entire screen.  Functions like drawing images, tiles, and other items are
 * declared, as well as functions to draw pixels and rectangles and to update
 * areas of the screen.
 *
 * Most of the functions here are obsolete and are slowly being
 * migrated to the xxxView classes.
 *
 * @todo
 *  <ul>
 *      <li>migrate rest of text output logic to TextView</li>
 *      <li>migrate rest of dungeon drawing logic to DungeonView</li>
 *  </ul>
 */

#ifndef SCREEN_H
#define SCREEN_H

#include <atomic>
#include <vector>
#include <string>

#include "direction.h"
#include "types.h"
#include "u4file.h"

class Image;
class Map;
class Tile;
class TileView;
class Coords;

#if __GNUC__
#define PRINTF_LIKE(x, y) __attribute__((format(printf, (x), (y))))
#else
#define PRINTF_LIKE(x, y)
#endif

/*
 * bitmasks for LOS shadows
 */
#define ____H 0x01 // obscured along the horizontal face
#define ___C_ 0x02 // obscured at the center
#define __V__ 0x04 // obscured along the vertical face
#define _N___ 0x80 // start of new raster

#define ___CH 0x03
#define __VCH 0x07
#define __VC_ 0x06

#define _N__H 0x81
#define _N_CH 0x83
#define _NVCH 0x87
#define _NVC_ 0x86
#define _NV__ 0x84

typedef enum {
    MC_DEFAULT,
    MC_WEST,
    MC_NORTH,
    MC_EAST,
    MC_SOUTH
} MouseCursor;

struct MouseArea {
    int npoints;
    struct {
        int x, y;
    } point[4];
    MouseCursor cursor;
    int command[3];
};

#define SCR_CYCLE_PER_SECOND 4

void screenInit();
void screenRefreshTimerInit();
void screenDelete();
void screenReInit();
void screenLock();
void screenUnlock();
void screenWait(int numberOfAnimationFrames);
void screenIconify();
const std::vector<std::string> &screenGetGemLayoutNames();
const std::vector<std::string> &screenGetFilterNames();
const std::vector<std::string> &screenGetLineOfSightStyles();
void screenDrawImage(const std::string &name, int x = 0, int y = 0);
void screenDrawImageInMapArea(const std::string &name);
void screenCycle();
void screenEraseMapArea();
void screenEraseTextArea(int x, int y, int width, int height);
void screenGemUpdate();
void screenMessage(const char *fmt, ...) PRINTF_LIKE(1, 2);
void screenPrompt();
void screenRedrawMapArea();
void screenRedrawScreen();
void screenRedrawTextArea(int x, int y, int width, int height);
void screenScrollMessageArea();
void screenShake(int iterations);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(3, 4);
void screenTextColor(int color);
bool screenTileUpdate(
    TileView *view, const Coords &coords, bool redraw = true
); // whether screen was affected
void screenUpdate(TileView *view, bool showmap, bool blackout);
void screenUpdateCursor();
void screenUpdateMoons();
void screenUpdateWind();
void screenShowCursor();
void screenHideCursor();
void screenEnableCursor();
void screenDisableCursor();
void screenSetCursorPos(int x, int y);
void screenSetMouseCursor(MouseCursor cursor);
bool screenPointInMouseArea(int x, int y, MouseArea *area);
Image *screenScale(Image *src, int scale, int n, int filter);
Image *screenScaleDown(Image *src, int scale);
extern std::atomic_int screenCurrentCycle;
extern std::atomic_bool screenMoving;

#define SCR_CYCLE_MAX 16

#endif // ifndef SCREEN_H
