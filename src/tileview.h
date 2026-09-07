/*
 * $Id$
 */

#ifndef TILEVIEW_H
#define TILEVIEW_H

#include <string>
#include <vector>

#include "u4.h"
#include "view.h"

class Image;
class MapTile;
class Tileset;


/**
 * A view of a grid of tiles.  Used to draw Maps.
 * @todo
 * <ul>
 *      <li>use for gem view</li>
 *      <li>initialize from a Layout?</li>
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
    ~TileView() override;

    void reinit() override;
    void drawTile(MapTile mapTile, bool focus, int x, int y) const;
    void drawTile(
        const std::vector<MapTile> &tiles, bool focus, int x, int y
    ) const;
    void drawFocus(int x, int y) const;
    void loadTile(MapTile mapTile) const;
    void setTileset(Tileset *ts);

protected:
    int columns, rows;
    static constexpr int tileWidth = TILE_WIDTH;
    static constexpr int tileHeight = TILE_HEIGHT;
    Tileset *tileset;
    Image *animated; /**< a scratchpad image for drawing animations */
};

#endif // TILEVIEW_H
