/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include <map>
#include "types.h"



class ConfigElement;
class Tile;

typedef std::map<std::string, class TileRule *> TileRuleMap;


/**
 * TileRule class
 */
class TileRule {
public:
    TileRule()
        :name(),
         mask(0),
         movementMask(0),
         speed(FAST),
         effect(EFFECT_NONE),
         walkonDirs(0),
         walkoffDirs(0)
    {
    }

    static TileRule *findByName(const std::string &name);
    static void load();
    static void unloadAll();
    static TileRuleMap rules; // A map of rule names to rules
    bool initFromConf(const ConfigElement &conf);
    std::string name;
    unsigned short mask;
    unsigned short movementMask;
    TileSpeed speed;
    TileEffect effect;
    int walkonDirs;
    int walkoffDirs;
};


/**
 * Tileset class
 */
class Tileset {
public:
    typedef std::map<std::string, Tileset *> TilesetMap;
    typedef std::map<TileId, Tile *> TileIdMap;
    typedef std::map<std::string, Tile *> TileStrMap;

    Tileset()
        :name(),
         tiles(),
         totalFrames(0),
         imageName(),
         extends(nullptr),
         nameMap()
    {
    }

    Tileset(const Tileset &) = delete;
    Tileset(Tileset &&) = delete;
    Tileset &operator=(const Tileset &) = delete;
    Tileset &operator=(Tileset &&) = delete;

    static void loadAll();
    static void unloadAll();
    static void unloadAllImages();
    static Tileset *get(const std::string &name);
    static Tile *findTileByName(const std::string &name);
    static Tile *findTileById(TileId id);
    void load(const ConfigElement &tilesetConf);
    void unload();
    void unloadImages() const;
    Tile *get(TileId id);
    Tile *getByName(const std::string &name);
    std::string getImageName() const;
    unsigned int numTiles() const;
    unsigned int numFrames() const;

private:
    static TilesetMap tilesets;
    std::string name;
    TileIdMap tiles;
    unsigned int totalFrames;
    std::string imageName;
    Tileset *extends;
    TileStrMap nameMap;
};

#endif // ifndef TILESET_H
