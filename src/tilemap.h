#/**
 * $Id$
 */

#ifndef TILEMAP_H
#define TILEMAP_H

#include <map>
#include <string>
#include "types.h"

class ConfigElement;


/**
 * A tilemap maps the raw bytes in a map file to MapTiles.
 */
class TileMap {
public:
    typedef std::map<std::string, TileMap *> TileIndexMapMap;
    typedef std::map<unsigned int, MapTile> TileMapMap;
    TileMap()
        :tilemap()
    {
    }

    MapTile translate(unsigned int index);
    unsigned int untranslate(MapTile tile) const;
    static void loadAll();
    static void unloadAll();
    static TileMap *get(const std::string &name);

private:
    static void load(const ConfigElement &tilemapConf);
    static TileIndexMapMap tileMaps;
    TileMapMap tilemap;
};

#endif // ifndef TILEMAP_H
