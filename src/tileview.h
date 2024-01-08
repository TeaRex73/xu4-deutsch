/*
 * $Id$
 */

#ifndef TILEVIEW_H
#define TILEVIEW_H

#include <vector>

#include "view.h"
#include "u4.h"

class Tile;
class Tileset;
class MapTile;


/**
 * A view of a grid of tiles.  Used to draw Maps.
 * @todo
 * <ul>
 *      <li>use for gem view</li>
 *      <li>intialize from a Layout?</li>
 * </ul>
 */
class TileView:public View {
public:
    TileView(int x, int y, int columns, int rows);
    TileView(int x, int y, int columns, int rows, const std::string &tileset);
    TileView(const TileView &) = delete;
    TileView(TileView &&) = delete;
    TileView &operator=(const TileView &) = delete;
    TileView &operator=(TileView &&) = delete;
    virtual ~TileView();
    virtual void reinit() override;
    void drawTile(MapTile mapTile, bool focus, int x, int y);
    void drawTile(
        const std::vector<MapTile> &tiles, bool focus, int x, int y
    );
    void drawFocus(int x, int y) const;
    void loadTile(MapTile mapTile);
    void setTileset(Tileset *tileset);

protected:
    int columns, rows;
    static const int tileWidth = TILE_WIDTH;
    static const int tileHeight = TILE_HEIGHT;
    Tileset *tileset;
    Image *animated; /**< a scratchpad image for drawing animations */
};

#endif /* TILEVIEW_H */
