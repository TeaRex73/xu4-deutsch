/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <string>
#include <vector>

#include "dungeonview.h"

#include "context.h"
#include "debug.h"
#include "direction.h"
#include "dungeon.h"
#include "image.h"
#include "imagemgr.h"
#include "location.h"
#include "map.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "tileanim.h"
#include "tileset.h"
#include "u4.h"
#include "view.h"


DungeonView::DungeonView(
    const int x, const int y, const int columns, const int rows
)
    :TileView(x, y, columns, rows), screen3dDungeonViewEnabled(true)
{
}

DungeonView *DungeonView::instance(nullptr);

DungeonView *DungeonView::getInstance()
{
    if (__builtin_expect(!instance, false)) {
        instance = new DungeonView(
            BORDER_WIDTH,
            BORDER_HEIGHT + 4 * settings.scale,
            VIEWPORT_W,
            VIEWPORT_H
        );
    }
    return instance;
}

void DungeonView::cleanup()
{
    delete instance;
}

void DungeonView::display(const Context *ctx, TileView *view) const
{
    /* 1st-person perspective */
    if (screen3dDungeonViewEnabled) {
        screenEraseMapArea();
        if (ctx->party->getTorchDuration() > 0) {
            for (int y = 3; y >= 0; y--) {
                // FIXME: Maybe this should be in a loop
                std::vector<MapTile> tiles = getTiles(y, -1);
                DungeonGraphicType type = tilesToGraphic(tiles);
                drawWall(
                    -1,
                    y,
                    static_cast<Direction>(ctx->saveGame->orientation),
                    type
                );
                tiles = getTiles(y, 1);
                type = tilesToGraphic(tiles);
                drawWall(
                    1,
                    y,
                    static_cast<Direction>(ctx->saveGame->orientation),
                    type
                );
                tiles = getTiles(y, 0);
                type = tilesToGraphic(tiles);
                drawWall(
                    0,
                    y,
                    static_cast<Direction>(ctx->saveGame->orientation),
                    type
                );
                // This only checks that the tile at
                // y==3 is opaque
                if (y == 3 && !tiles.front().getTileType()->isOpaque()) {
                    // Note: This shouldn't go above 4, unless we check
                    // opaque tiles each step of the way.
                    constexpr int furthest_nw_vis = 4;
                    for (int y_obj = furthest_nw_vis; y_obj > y; y_obj--) {
                        std::vector<MapTile> dts = getTiles(y_obj, 0);
                        const DungeonGraphicType dt = tilesToGraphic(dts);
                        if (dt == DNG_GRAPHIC_DNG_TILE
                            || dt == DNG_GRAPHIC_BASE_TILE) {
                            drawTile(
                                ctx->location->map->tileset
                                ->get(dts.front().getId()),
                                0,
                                y_obj,
                                static_cast<Direction>(
                                    ctx->saveGame->orientation
                                )
                            );
                        }
                    }
                }
                if (type == DNG_GRAPHIC_DNG_TILE
                    || type == DNG_GRAPHIC_BASE_TILE) {
                    drawTile(
                        ctx->location->map->tileset->get(tiles.front().getId()),
                        0,
                        y,
                        static_cast<Direction>(ctx->saveGame->orientation)
                    );
                }
            }
        }
    }
    /* 3rd-person perspective */
    else {
        static MapTile black =
            ctx->location->map->tileset->getByName("black")->getId();
        static MapTile avatar =
            ctx->location->map->tileset->getByName("avatar")->getId();
        for (int y = 0; y < VIEWPORT_H; y++) {
            for (int x = 0; x < VIEWPORT_W; x++) {
                        std::vector<MapTile> tiles =
                            getTiles(
                                VIEWPORT_H / 2 - y,
                                x - VIEWPORT_W / 2
                            );
                /* Only show blackness if there is no light */
                if (ctx->party->getTorchDuration() <= 0) {
                    view->drawTile(black, false, x, y);
                } else if (x == VIEWPORT_W / 2 && y == VIEWPORT_H / 2) {
                    view->drawTile(avatar, false, x, y);
                } else {
                    view->drawTile(tiles, false, x, y);
                }
            }
        }
    }
} // DungeonView::display

void DungeonView::drawInDungeon(
    Tile *tile,
    int,
    const int distance,
    const Direction orientation,
    const bool tiledWall
) const
{
    Image *scaled;
    const static int n_scale_vga[] = { 12, 8, 4, 2, 1 };
    const static int n_scale_ega[] = { 8, 4, 2, 1, 0 };
    const int l_scale_vga[] = { 22, 18, 10, 4, 1 };
    const int l_scale_ega[] = { 22, 14, 6, 3, 1 };
    const int *l_scale;
    const int *n_scale;
    int offset_multiplier = 0;
    int offset_adj = 0;
    if (settings.videoType != "EGA") {
        l_scale = &l_scale_vga[0];
        n_scale = &n_scale_vga[0];
        offset_multiplier = 1;
        offset_adj = 2;
    } else {
        l_scale = &l_scale_ega[0];
        n_scale = &n_scale_ega[0];
        offset_adj = 1;
        offset_multiplier = 4;
    }
    const int *d_scale = tiledWall ? l_scale : n_scale;
    // Clear scratchpad and set a background color
    animated->initializeToBackgroundColor();
    // Put tile on animated scratchpad
    if (tile->getAnim()) {
        const MapTile mt = tile->getId();
        tile->getAnim()->draw(animated, tile, mt, orientation);
    } else {
        tile->getImage()->drawOn(animated, 0, 0);
    }
    animated->makeBackgroundColorTransparent();
    // This process involving the background color is only required
    // for drawing in the dungeon.
    // It will not play well with semi-transparent graphics.
    /* scale is based on distance; 1 means half size, 2 regular,
       4 means scale by 2x, etc. */
    if (d_scale[distance] == 0) {
        return;
    }
    if (d_scale[distance] == 1) {
        scaled = screenScaleDown(animated, 2);
    } else {
        scaled = screenScale(animated, d_scale[distance] / 2, 1, 0);
    }
    if (tiledWall) {
        const int i_x = static_cast<int>(
            SCALED((VIEWPORT_W * tileWidth / 2.0) + this->x)
            - (scaled->width() >> 1)
        );
        const int i_y = static_cast<int>(
            SCALED((VIEWPORT_H * tileHeight / 2.0) + this->y)
            - (scaled->height() >> 1)
        );
        const int f_x = i_x + scaled->width();
        const int f_y = i_y + scaled->height();
        const int d_x = animated->width();
        const int d_y = animated->height();
        for (int x = i_x; x < f_x; x += d_x) {
            for (int y = i_y; y < f_y; y += d_y) {
                animated->drawSubRectOn(
                    screen, x, y, 0, 0, f_x - x, f_y - y
                );
            }
        }
    } else {
        const int y_offset = std::max(
            0, (d_scale[distance] - offset_adj) * offset_multiplier
        );
        const int x = static_cast<int>(
            SCALED((VIEWPORT_W * tileWidth / 2.0) + this->x)
            - (scaled->width() >> 1)
        );
        const int y = static_cast<int>(
            SCALED((VIEWPORT_H * tileHeight / 2.0) + this->y + y_offset)
            - (scaled->height() >> 3)
        );
        scaled->drawSubRectOn(
            screen,
            x,
            y,
            0,
            0,
            SCALED(tileWidth * VIEWPORT_W + this->x) - x,
            SCALED(tileHeight * VIEWPORT_H + this->y) - y
        );
    }
    delete scaled;
} // DungeonView::drawInDungeon

int DungeonView::graphicIndex(
    const int x_offset,
    const int distance,
    const Direction orientation,
    const DungeonGraphicType type
)
{
    int index = 0;
    if (type == DNG_GRAPHIC_LADDER_UP && x_offset == 0) {
        return 48
            + distance * 2
            + (
                DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ?
                1 :
                0
            );
    }
    if (type == DNG_GRAPHIC_LADDER_DOWN && x_offset == 0) {
        return 56
            + distance * 2
            + (
                DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ?
                1 :
                0
            );
    }
    if (type == DNG_GRAPHIC_LADDER_UP_DOWN && x_offset == 0) {
        return 64
            + distance * 2
            + (
                DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ?
                1 :
                0
            );
    }

    /* FIXME */
    if (type != DNG_GRAPHIC_WALL && type != DNG_GRAPHIC_DOOR) {
        return -1;
    }
    if (type == DNG_GRAPHIC_DOOR) {
        index += 24;
    }
    index += (x_offset + 1) * 2;
    index += distance * 6;
    if (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH)) {
        index++;
    }
    return index;
} // DungeonView::graphicIndex

void DungeonView::drawTile(
    Tile *tile,
    const int x_offset,
    const int distance,
    const Direction orientation
)
{
    // Draw the tile to the screen
    DungeonViewer.drawInDungeon(
        tile, x_offset, distance, orientation, tile->isTiledInDungeon()
    );
}

std::vector<MapTile> DungeonView::getTiles(const int fwd, const int side)
{
    MapCoords coords = c->location->coords;
    switch (c->saveGame->orientation) {
    case DIR_WEST:
        coords.x -= fwd;
        coords.y -= side;
        break;
    case DIR_NORTH:
        coords.x += side;
        coords.y -= fwd;
        break;
    case DIR_EAST:
        coords.x += fwd;
        coords.y += side;
        break;
    case DIR_SOUTH:
        coords.x -= side;
        coords.y += fwd;
        break;
    case DIR_ADVANCE:
    case DIR_RETREAT:
    default:
        U4ASSERT(0, "Invalid dungeon orientation");
    }
    // Wrap the coordinates if necessary
    coords.wrap(c->location->map);
    bool focus;
    return c->location->tilesAt(coords, focus);
} // DungeonView::getTiles

DungeonGraphicType DungeonView::tilesToGraphic(
    const std::vector<MapTile> &tiles
)
{
    const MapTile tile = tiles.front();
    static const MapTile corridor =
        c->location->map->tileset->getByName("brick_floor")->getId();
    static const MapTile up_ladder =
        c->location->map->tileset->getByName("up_ladder")->getId();
    static const MapTile down_ladder =
        c->location->map->tileset->getByName("down_ladder")->getId();
    static const MapTile updown_ladder =
        c->location->map->tileset->getByName("up_down_ladder")->getId();
    /*
     * check if the dungeon tile has an annotation or object on top
     * (always displayed as a tile, unless a ladder)
     */
    if (tiles.size() > 1) {
        if (tile.getId() == up_ladder.getId()) {
            return DNG_GRAPHIC_LADDER_UP;
        }
        if (tile.getId() == down_ladder.getId()) {
            return DNG_GRAPHIC_LADDER_DOWN;
        }
        if (tile.getId() == updown_ladder.getId()) {
            return DNG_GRAPHIC_LADDER_UP_DOWN;
        }
        if (tile.getId() == corridor.getId()) {
            return DNG_GRAPHIC_NONE;
        }
        return DNG_GRAPHIC_BASE_TILE;
    }
    /*
     * if not an annotation or object, then the tile is a dungeon
     * token
     */
    const Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    const DungeonToken token = dungeon->tokenForTile(tile);
    switch (token) {
    case DUNGEON_TRAP:
    case DUNGEON_CORRIDOR:
        return DNG_GRAPHIC_NONE;
    case DUNGEON_WALL:
    case DUNGEON_SECRET_DOOR:
        return DNG_GRAPHIC_WALL;
    case DUNGEON_ROOM:
    case DUNGEON_DOOR:
        return DNG_GRAPHIC_DOOR;
    case DUNGEON_LADDER_UP:
        return DNG_GRAPHIC_LADDER_UP;
    case DUNGEON_LADDER_DOWN:
        return DNG_GRAPHIC_LADDER_DOWN;
    case DUNGEON_LADDER_UPDOWN:
        return DNG_GRAPHIC_LADDER_UP_DOWN;
    default:
        return DNG_GRAPHIC_DNG_TILE;
    }
} // DungeonView::tilesToGraphic

const struct {
    const char *subimage;
    int ega_x2, ega_y2;
    int vga_x2, vga_y2;
    const char *subimage2;
} dngGraphicInfo[] = {
    {
        .subimage = "dung0_lft_ew",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_lft_ns",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_mid_ew",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_mid_ns",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_rgt_ew",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_rgt_ns",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_lft_ew",
        .ega_x2 = 0,
        .ega_y2 = 32,
        .vga_x2 = 0,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ew"
    },
    {
        .subimage = "dung1_lft_ns",
        .ega_x2 = 0,
        .ega_y2 = 32,
        .vga_x2 = 0,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ns"
    },
    {
        .subimage = "dung1_mid_ew",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_mid_ns",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_rgt_ew",
        .ega_x2 = 144,
        .ega_y2 = 32,
        .vga_x2 = 160,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ew"
    },
    {
        .subimage = "dung1_rgt_ns",
        .ega_x2 = 144,
        .ega_y2 = 32,
        .vga_x2 = 160,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ns"
    },
    {
        .subimage = "dung2_lft_ew",
        .ega_x2 = 0,
        .ega_y2 = 64,
        .vga_x2 = 0,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ew"
    },
    {
        .subimage = "dung2_lft_ns",
        .ega_x2 = 0,
        .ega_y2 = 64,
        .vga_x2 = 0,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ns"
    },
    {
        .subimage = "dung2_mid_ew",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_mid_ns",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_rgt_ew",
        .ega_x2 = 112,
        .ega_y2 = 64,
        .vga_x2 = 128,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ew"
    },
    {
        .subimage = "dung2_rgt_ns",
        .ega_x2 = 112,
        .ega_y2 = 64,
        .vga_x2 = 128,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ns"
    },
    {
        .subimage = "dung3_lft_ew",
        .ega_x2 = 0,
        .ega_y2 = 80,
        .vga_x2 = 48,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ew"
    },
    {
        .subimage = "dung3_lft_ns",
        .ega_x2 = 0,
        .ega_y2 = 80,
        .vga_x2 = 48,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ns"
    },
    {
        .subimage = "dung3_mid_ew",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_mid_ns",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_rgt_ew",
        .ega_x2 = 96,
        .ega_y2 = 80,
        .vga_x2 = 104,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ew"
    },
    {
        .subimage = "dung3_rgt_ns",
        .ega_x2 = 96,
        .ega_y2 = 80,
        .vga_x2 = 104,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ns"
    },
    {
        .subimage = "dung0_lft_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_lft_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_mid_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_mid_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_rgt_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_rgt_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_lft_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 32,
        .vga_x2 = 0,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ew"
    },
    {
        .subimage = "dung1_lft_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 32,
        .vga_x2 = 0,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ns"
    },
    {
        .subimage = "dung1_mid_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_mid_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_rgt_ew_door",
        .ega_x2 = 144,
        .ega_y2 = 32,
        .vga_x2 = 160,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ew"
    },
    {
        .subimage = "dung1_rgt_ns_door",
        .ega_x2 = 144,
        .ega_y2 = 32,
        .vga_x2 = 160,
        .vga_y2 = 8,
        .subimage2 = "dung1_xxx_ns"
    },
    {
        .subimage = "dung2_lft_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 64,
        .vga_x2 = 0,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ew"
    },
    {
        .subimage = "dung2_lft_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 64,
        .vga_x2 = 0,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ns"
    },
    {
        .subimage = "dung2_mid_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_mid_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_rgt_ew_door",
        .ega_x2 = 112,
        .ega_y2 = 64,
        .vga_x2 = 128,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ew"
    },
    {
        .subimage = "dung2_rgt_ns_door",
        .ega_x2 = 112,
        .ega_y2 = 64,
        .vga_x2 = 128,
        .vga_y2 = 48,
        .subimage2 = "dung2_xxx_ns"
    },
    {
        .subimage = "dung3_lft_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 80,
        .vga_x2 = 48,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ew"
    },
    {
        .subimage = "dung3_lft_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 80,
        .vga_x2 = 48,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ns"
    },
    {
        .subimage = "dung3_mid_ew_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_mid_ns_door",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_rgt_ew_door",
        .ega_x2 = 96,
        .ega_y2 = 80,
        .vga_x2 = 104,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ew"
    },
    {
        .subimage = "dung3_rgt_ns_door",
        .ega_x2 = 96,
        .ega_y2 = 80,
        .vga_x2 = 104,
        .vga_y2 = 72,
        .subimage2 = "dung3_xxx_ns"
    },
    {
        .subimage = "dung0_ladderup",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_ladderup_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_ladderup",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_ladderup_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_ladderup",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_ladderup_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_ladderup",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_ladderup_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_ladderdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_ladderdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_ladderdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_ladderdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_ladderdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_ladderdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_ladderdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_ladderdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_ladderupdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung0_ladderupdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_ladderupdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung1_ladderupdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_ladderupdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung2_ladderupdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_ladderupdown",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    },
    {
        .subimage = "dung3_ladderupdown_side",
        .ega_x2 = 0,
        .ega_y2 = 0,
        .vga_x2 = 0,
        .vga_y2 = 0,
        .subimage2 = nullptr
    }
};

void DungeonView::drawWall(
    const int x_offset,
    const int distance,
    const Direction orientation,
    const DungeonGraphicType type
)
{
    const int index = graphicIndex(x_offset, distance, orientation, type);
    if (index == -1 || distance >= 4) {
        return;
    }
    int x = 0, y = 0;
    const SubImage *subimage =
        imageMgr->getSubImage(dngGraphicInfo[index].subimage);
    if (subimage) {
        x = subimage->x;
        y = subimage->y;
    }
    screenDrawImage(
        dngGraphicInfo[index].subimage,
        (BORDER_WIDTH + x) * settings.scale,
        (BORDER_HEIGHT + y + 4) * settings.scale
    );
    if (dngGraphicInfo[index].subimage2 != nullptr) {
        // FIXME: subimage2 is a horrible hack, needs to be cleaned up
        if (settings.videoType == "EGA") {
            screenDrawImage(
                dngGraphicInfo[index].subimage2,
                (8 + dngGraphicInfo[index].ega_x2) * settings.scale,
                (8 + dngGraphicInfo[index].ega_y2 + 4) * settings.scale
            );
        } else {
            screenDrawImage(
                dngGraphicInfo[index].subimage2,
                (8 + dngGraphicInfo[index].vga_x2) * settings.scale,
                (8 + dngGraphicInfo[index].vga_y2 + 4) * settings.scale
            );
        }
    }
} // DungeonView::drawWall
