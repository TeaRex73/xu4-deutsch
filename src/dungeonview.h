/*
 * $Id$
 */

#ifndef DUNGEONVIEW_H
#define DUNGEONVIEW_H

#include <vector>

#include "direction.h"
#include "tileview.h"
#include "types.h"

class Context;
class DungeonView;
class Tile;


typedef enum {
    DNG_GRAPHIC_NONE,
    DNG_GRAPHIC_WALL,
    DNG_GRAPHIC_LADDER_UP,
    DNG_GRAPHIC_LADDER_DOWN,
    DNG_GRAPHIC_LADDER_UP_DOWN,
    DNG_GRAPHIC_DOOR,
    DNG_GRAPHIC_DNG_TILE,
    DNG_GRAPHIC_BASE_TILE
} DungeonGraphicType;

std::vector<MapTile> dungeonViewGetTiles(int fwd, int side);
DungeonGraphicType dungeonViewTilesToGraphic(
    const std::vector<MapTile> &tiles
);

#define DungeonViewer (*DungeonView::getInstance())


/**
 * @todo
 * <ul>
 *      <li>move the rest of the dungeon drawing logic here from
 *      screen_sdl</li>
 * </ul>
 */
class DungeonView:public TileView {
    DungeonView(int x, int y, int columns, int rows);
    bool screen3dDungeonViewEnabled;
public:
    static DungeonView *instance;
    static DungeonView *getInstance();
    static void cleanup();
    void drawInDungeon(
        Tile *tile,
        int x_offset,
        int distance,
        Direction orientation,
        bool tiledWall
    ) const;
    static int graphicIndex(
        int x_offset,
        int distance,
        Direction orientation,
        DungeonGraphicType type
    );
    static void drawTile(
        Tile *tile, int x_offset, int distance, Direction orientation
    );
    static void drawWall(
        int x_offset,
        int distance,
        Direction orientation,
        DungeonGraphicType type
    );

    void display(const Context *ctx, TileView *view) const;
    static DungeonGraphicType tilesToGraphic(
        const std::vector<MapTile> &tiles
    );

    bool toggle3DDungeonView()
    {
        return screen3dDungeonViewEnabled = !screen3dDungeonViewEnabled;
    }

    static std::vector<MapTile> getTiles(int fwd, int side);
};

#endif /* DUNGEONVIEW_H */
