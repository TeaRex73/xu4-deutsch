/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <vector>

#include "tilemap.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "tile.h"
#include "tileset.h"

static Debug dbg("debug/tilemap.txt", "TileMap");


/**
 * Static variables
 */
TileMap::TileIndexMapMap TileMap::tileMaps;


/**
 * Load all tilemaps from the specified xml file
 */
void TileMap::loadAll()
{
    const Config *config = Config::getInstance();
    /* FIXME: make sure tilesets are loaded by now */
    TRACE_LOCAL(dbg, "Unloading all tilemaps");
    unloadAll();
    /* open the filename for the tileset and parse it! */
    TRACE_LOCAL(dbg, "Loading tilemaps from config");
    const std::vector<ConfigElement> children =
        config->getElement("tilesets").getChildren();
    /* load all of the tilemaps */
    for (const auto &child: children) {
        if (child.getName() == "tilemap") {
            /* load the tilemap ! */
            load(child);
        }
    }
}


/**
 * Delete all tilemaps
 */
void TileMap::unloadAll()
{
    /* free all the memory for the tile maps */
    for (const auto &tileMap: tileMaps) {
        delete tileMap.second;
    }
    /* Clear the map so we don't attempt to delete the memory again
     * next time.
     */
    tileMaps.clear();
}


/**
 * Loads a tile map which translates between tile indices and tile
 * names.  Tile maps are useful to translate from dos tile indices to
 * xu4 tile ids.
 */
void TileMap::load(const ConfigElement &tilemapConf)
{
    auto *tm = new TileMap;
    const std::string name = tilemapConf.getString("name");
    TRACE_LOCAL(dbg, std::string("Tilemap name is: ") + name);
    const std::string tileset = tilemapConf.getString("tileset");
    int index = 0;
    const std::vector<ConfigElement> children = tilemapConf.getChildren();
    for (const auto &child: children) {
        if (child.getName() != "mapping") {
            continue;
        }
        /* we assume tiles have already been loaded at this point,
           so let's do some translations! */
        int frames = 1;
        std::string tile = child.getString("tile");
        TRACE_LOCAL(dbg, std::string("\tLoading '") + tile + "'");
        /* find the tile this references */
        const Tile *t = Tileset::get(tileset)->getByName(tile);
        if (!t) {
            errorFatal(
                "Error: tile '%s' from '%s' was not found in tileset %s",
                tile.c_str(),
                name.c_str(),
                tileset.c_str()
            );
        }
        if (child.exists("index")) {
            index = child.getInt("index");
        }
        if (child.exists("frames")) {
            frames = child.getInt("frames");
        }
        /* insert the tile into the tile map */
        for (int j = 0; j < frames; j++) {
            if (j < t->getFrames()) {
                tm->tilemap[index + j] =
                    MapTile(t->getId(), j);
            }
            /* frame fell out of the scope of the tile --
               frame is set to 0 */
            else {
                tm->tilemap[index + j] =
                    MapTile(t->getId(), 0);
            }
        }
        index += frames;
    }
    /* add the tilemap to our list */
    tileMaps[name] = tm;
} // TileMap::load


/**
 * Returns the Tile index map with the specified name
 */
TileMap *TileMap::get(const std::string &name)
{
    if (tileMaps.find(name) != tileMaps.end()) {
        return tileMaps[name];
    }
    return nullptr;
}


/**
 * Translates a raw index to a MapTile.
 */
MapTile TileMap::translate(const int index)
{
    return tilemap[index];
}

int TileMap::untranslate(const MapTile tile) const
{
    int index = 0;
    const auto i =
        std::find_if(
            tilemap.cbegin(),
            tilemap.cend(),
            [&](const TileMapMap::value_type &v) -> bool {
                return v.second == tile;
            }
        );
    if (i != tilemap.cend()) {
        index = i->first;
    }
    index += tile.getFrame();
    return index;
}
