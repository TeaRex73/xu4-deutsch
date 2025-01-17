/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "debug.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "screen.h"
#include "tile.h"
#include "tileanim.h"
#include "tileset.h"
#include "tileview.h"
#include "u4.h"
#include "error.h"

TileView::TileView(int x, int y, int columns, int rows)
    :View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT),
     columns(columns),
     rows(rows),
#if 0
     tileWidth(TILE_WIDTH),
     tileHeight(TILE_HEIGHT),
#endif
     tileset(Tileset::get("base")),
     animated(
         Image::create(
             SCALED(tileWidth), SCALED(tileHeight), false, Image::SOFTWARE
         )
     )
{
    animated->alphaOff();
}

TileView::TileView(
    int x, int y, int columns, int rows, const std::string &tileset
)
    :View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT),
     columns(columns),
     rows(rows),
#if 0
     tileWidth(TILE_WIDTH),
     tileHeight(TILE_HEIGHT),
#endif
     tileset(Tileset::get(tileset)),
     animated(
         Image::create(
             SCALED(tileWidth), SCALED(tileHeight), false, Image::SOFTWARE
         )
     )
{
    animated->alphaOff();
}

TileView::~TileView()
{
    delete animated;
}

void TileView::reinit()
{
    View::reinit();
    tileset = Tileset::get("base");
    // Scratchpad needs to be re-inited if we rescale...
    delete animated;
    animated = Image::create(
        SCALED(tileWidth), SCALED(tileHeight), false, Image::SOFTWARE
    );
    animated->alphaOff();
}

void TileView::loadTile(MapTile mapTile)
{
    // This attempts to preload tiles in advance
    Tile *tile = tileset->get(mapTile.getId());
    if (tile) {
        tile->getImage();
    }
    // But may fail if the tiles don't exist directly in the expected
    // imagesets
}

void TileView::drawTile(MapTile mapTile, bool focus, int x, int y)
{
    Tile *tile = tileset->get(mapTile.getId());
    const Image *image = tile->getImage();

    U4ASSERT(x < columns, "x value of %d out of range", x);
    U4ASSERT(y < rows, "y value of %d out of range", y);

    // Blank scratch pad
    animated->fillRect(
        0, 0, SCALED(tileWidth), SCALED(tileHeight), 0, 0, 0, 255
    );
#if 0 // disabled - causes ugly flashing in intro animation on slow systems
    // Draw blackness on the tile.
    animated->drawSubRect(
        SCALED(x * tileWidth + this->x),
        SCALED(y * tileHeight + this->y),
        0,
        0,
        SCALED(tileWidth),
        SCALED(tileHeight)
    );
#endif
    // draw the tile to the screen
    if (tile->getAnim()) {
        // First, create our animated version of the tile
        tile->getAnim()->draw(animated, tile, mapTile, DIR_NONE);
        // Then draw it to the screen
        animated->drawSubRect(
            SCALED(x * tileWidth + this->x),
            SCALED(y * tileHeight + this->y),
            0,
            0,
            SCALED(tileWidth),
            SCALED(tileHeight)
        );
    } else {
        image->drawSubRect(
            SCALED(x * tileWidth + this->x),
            SCALED(y * tileHeight + this->y),
            0,
            SCALED(tileHeight * mapTile.getFrame()),
            SCALED(tileWidth), SCALED(tileHeight)
        );
    }
    // draw the focus around the tile if it has the focus
    if (focus) {
        drawFocus(x, y);
    }
} // TileView::drawTile

void TileView::drawTile(
    const std::vector<MapTile> &tiles, bool focus, int x, int y
    )
{
    U4ASSERT(x < columns, "x value of %d out of range", x);
    U4ASSERT(y < rows, "y value of %d out of range", y);
    animated->fillRect(
        0, 0, SCALED(tileWidth), SCALED(tileHeight), 0, 0, 0, 255
    );
    animated->drawSubRect(
        SCALED(x * tileWidth + this->x),
        SCALED(y * tileHeight + this->y),
        0,
        0,
        SCALED(tileWidth),
        SCALED(tileHeight)
    );
    // int layer = 0;
    for (std::vector<MapTile>::const_reverse_iterator t = tiles.crbegin();
         t != tiles.crend();
         ++t) {
        MapTile frontTile = *t;
        Tile *frontTileType = tileset->get(frontTile.getId());
        if (!frontTileType) {
            // TODO, this leads to an error.
            // It happens after graphics mode changes.
            return;
        }
        const Image *image = frontTileType->getImage();
        // draw the tile to the screen
        if (frontTileType->getAnim()) {
            // First, create our animated version of the tile
            frontTileType->getAnim()->draw(
                animated, frontTileType, frontTile, DIR_NONE
            );
        } else {
            if (!image) {
                return;
                // This is a problem
                //FIXME, error message it.
            }
            image->drawSubRectOn(
                animated,
                0,
                0,
                0,
                SCALED(tileHeight * frontTile.getFrame()),
                SCALED(tileWidth),
                SCALED(tileHeight)
            );
        }
        // Then draw it to the screen
        animated->drawSubRect(
            SCALED(x * tileWidth + this->x),
            SCALED(y * tileHeight + this->y),
            0,
            0,
            SCALED(tileWidth),
            SCALED(tileHeight)
        );
    }
    // draw the focus around the tile if it has the focus
    if (focus) {
        drawFocus(x, y);
    }
} // TileView::drawTile


/**
 * Draw a focus rectangle around the tile
 */
void TileView::drawFocus(int x, int y) const
{
    U4ASSERT(x < columns, "x value of %d out of range", x);
    U4ASSERT(y < rows, "y value of %d out of range", y);
    /*
     * draw the focus rectangle around the tile
     */
    if ((screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND) % 2) {
        /* left edge */
        screen->fillRect(
            SCALED(x * tileWidth + this->x),
            SCALED(y * tileHeight + this->y),
            SCALED(2),
            SCALED(tileHeight),
            0xff,
            0xff,
            0xff
        );
        /* top edge */
        screen->fillRect(
            SCALED(x * tileWidth + this->x),
            SCALED(y * tileHeight + this->y),
            SCALED(tileWidth),
            SCALED(2),
            0xff,
            0xff,
            0xff
        );
        /* right edge */
        screen->fillRect(
            SCALED((x + 1) * tileWidth + this->x - 2),
            SCALED(y * tileHeight + this->y),
            SCALED(2),
            SCALED(tileHeight),
            0xff,
            0xff,
            0xff
        );
        /* bottom edge */
        screen->fillRect(
            SCALED(x * tileWidth + this->x),
            SCALED((y + 1) * tileHeight + this->y - 2),
            SCALED(tileWidth),
            SCALED(2),
            0xff,
            0xff,
            0xff
        );
    }
}

void TileView::setTileset(Tileset *tileset)
{
    this->tileset = tileset;
}
