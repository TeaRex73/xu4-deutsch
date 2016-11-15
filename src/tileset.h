/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include <unordered_map>
#include "types.h"



class ConfigElement;
class Tile;

typedef std::map<std::string, class TileRule *> TileRuleMap;


/**
 * TileRule class
 */
class TileRule {
public:
    static TileRule *findByName(const std::string &name);
    static void load();
    static void unloadAll();
    static TileRuleMap rules; // A map of rule names to rules
    bool initFromConf(const ConfigElement &tileRuleConf);
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
    typedef std::unordered_map<TileId, Tile *> TileIdMap;
    typedef std::map<std::string, Tile *> TileStrMap;
    static void loadAll();
    static void unloadAll();
    static void unloadAllImages();
    static Tileset *get(const std::string &name);
    static Tile *findTileByName(const std::string &name);
    static Tile *findTileById(TileId id);
    void load(const ConfigElement &tilesetConf);
    void unload();
    void unloadImages();
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
