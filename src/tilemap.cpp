/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <vector>

#include "tilemap.h"
#include "tile.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "tileset.h"
#include "utils.h"

Debug dbg("debug/tilemap.txt", "TileMap");


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
    std::vector<ConfigElement> conf;
    /* FIXME: make sure tilesets are loaded by now */
    TRACE_LOCAL(dbg, "Unloading all tilemaps");
    unloadAll();
    /* open the filename for the tileset and parse it! */
    TRACE_LOCAL(dbg, "Loading tilemaps from config");
    conf = config->getElement("tilesets").getChildren();
    /* load all of the tilemaps */
    for (std::vector<ConfigElement>::const_iterator i = conf.cbegin();
         i != conf.cend();
         ++i) {
        if (i->getName() == "tilemap") {
            /* load the tilemap ! */
            load(*i);
        }
    }
}


/**
 * Delete all tilemaps
 */
void TileMap::unloadAll()
{
    TileIndexMapMap::const_iterator map;
    /* free all the memory for the tile maps */
    for (map = tileMaps.cbegin(); map != tileMaps.cend(); ++map) {
        delete map->second;
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
    TileMap *tm = new TileMap;
    std::string name = tilemapConf.getString("name");
    TRACE_LOCAL(dbg, std::string("Tilemap name is: ") + name);
    std::string tileset = tilemapConf.getString("tileset");
    int index = 0;
    std::vector<ConfigElement> children = tilemapConf.getChildren();
    for (std::vector<ConfigElement>::const_iterator i = children.cbegin();
         i != children.cend();
         ++i) {
        if (i->getName() != "mapping") {
            continue;
        }
        /* we assume tiles have already been loaded at this point,
           so let's do some translations! */
        int frames = 1;
        std::string tile = i->getString("tile");
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
        if (i->exists("index")) {
            index = i->getInt("index");
        }
        if (i->exists("frames")) {
            frames = i->getInt("frames");
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
    } else {
        return nullptr;
    }
}


/**
 * Translates a raw index to a MapTile.
 */
MapTile TileMap::translate(unsigned int index)
{
    return tilemap[index];
}

unsigned int TileMap::untranslate(MapTile tile) const
{
    unsigned int index = 0;
    TileMapMap::const_iterator i =
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
