/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <vector>

#include "tileset.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "tilemap.h"


/**
 * TileRule Class Implementation
 */
TileRuleMap TileRule::rules;


/**
 * Returns the tile rule with the given name, or nullptr if none could be found
 */
TileRule *TileRule::findByName(const std::string &name)
{
    TileRuleMap::const_iterator i = rules.find(name);
    if (i != rules.cend()) {
        return i->second;
    }
    return nullptr;
}

void TileRule::unloadAll()
{
    TileRuleMap::const_iterator i;
    for (i = rules.cbegin(); i != rules.cend(); ++i) {
        delete i->second;
    }
    rules.clear();
}


/**
 * Load tile information from xml.
 */
void TileRule::load()
{
    const Config *config = Config::getInstance();
    std::vector<ConfigElement> rules =
        config->getElement("tileRules").getChildren();
    for (std::vector<ConfigElement>::const_iterator i = rules.cbegin();
         i != rules.cend();
         ++i) {
        TileRule *rule = new TileRule;
        rule->initFromConf(*i);
        TileRule::rules[rule->name] = rule;
    }

    if (TileRule::findByName("default") == nullptr) {
        errorFatal("no 'default' rule found in tile rules");
    }
}


/**
 * Load properties for the current rule node
 */
bool TileRule::initFromConf(const ConfigElement &conf)
{
    unsigned int i;
    static const struct {
        const char *name;
        unsigned int mask;
    } booleanAttributes[] = {
        { "dispel", MASK_DISPEL },
        { "talkover", MASK_TALKOVER },
        { "door", MASK_DOOR },
        { "lockeddoor", MASK_LOCKEDDOOR },
        { "chest", MASK_CHEST },
        { "ship", MASK_SHIP },
        { "horse", MASK_HORSE },
        { "balloon", MASK_BALLOON },
        { "canattackover", MASK_ATTACKOVER },
        { "canlandballoon", MASK_CANLANDBALLOON },
        { "replacement", MASK_REPLACEMENT },
        { "foreground", MASK_FOREGROUND },
        { "onWaterOnlyReplacement", MASK_WATER_REPLACEMENT },
        { "livingthing", MASK_LIVING_THING },
        { "spawnslandmonster", MASK_SPAWNS_LAND_MONSTER },
        { "spawnsseamonster", MASK_SPAWNS_SEA_MONSTER }
    };
    static const struct {
        const char *name;
        unsigned int mask;
    } movementBooleanAttr[] = {
        { "swimable", MASK_SWIMABLE },
        { "sailable", MASK_SAILABLE },
        { "unflyable", MASK_UNFLYABLE },
        { "creatureunwalkable", MASK_CREATURE_UNWALKABLE },
        { "wontwanderon", MASK_WONTWANDERON }
    };
    static const char *speedEnumStrings[] = {
        "fast",
        "slow",
        "vslow",
        "vvslow",
        nullptr
    };
    static const char *effectsEnumStrings[] = {
        "none",
        "fire",
        "sleep",
        "poison",
        "poisonField",
        "electricity",
        "lava",
        nullptr
    };
    this->mask = 0;
    this->movementMask = 0;
    this->speed = FAST;
    this->effect = EFFECT_NONE;
    this->walkonDirs = MASK_DIR_ALL;
    this->walkoffDirs = MASK_DIR_ALL;
    this->name = conf.getString("name");
    for (i = 0;
         i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]);
         i++) {
        if (conf.getBool(booleanAttributes[i].name)) {
            this->mask |= booleanAttributes[i].mask;
        }
    }
    for (i = 0;
         i < sizeof(movementBooleanAttr) / sizeof(movementBooleanAttr[0]);
         i++) {
        if (conf.getBool(movementBooleanAttr[i].name)) {
            this->movementMask |= movementBooleanAttr[i].mask;
        }
    }
    std::string cantwalkon = conf.getString("cantwalkon");
    if (cantwalkon == "all") {
        this->walkonDirs = 0;
    } else if (cantwalkon == "west") {
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, this->walkonDirs);
    } else if (cantwalkon == "north") {
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, this->walkonDirs);
    } else if (cantwalkon == "east") {
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, this->walkonDirs);
    } else if (cantwalkon == "south") {
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, this->walkonDirs);
    } else if (cantwalkon == "advance") {
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, this->walkonDirs);
    } else if (cantwalkon == "retreat") {
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, this->walkonDirs);
    }
    std::string cantwalkoff = conf.getString("cantwalkoff");
    if (cantwalkoff == "all") {
        this->walkoffDirs = 0;
    } else if (cantwalkoff == "west") {
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, this->walkoffDirs);
    } else if (cantwalkoff == "north") {
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, this->walkoffDirs);
    } else if (cantwalkoff == "east") {
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, this->walkoffDirs);
    } else if (cantwalkoff == "south") {
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, this->walkoffDirs);
    } else if (cantwalkoff == "advance") {
        this->walkoffDirs =
            DIR_REMOVE_FROM_MASK(DIR_ADVANCE, this->walkoffDirs);
    } else if (cantwalkoff == "retreat") {
        this->walkoffDirs =
            DIR_REMOVE_FROM_MASK(DIR_RETREAT, this->walkoffDirs);
    }
    this->speed =
        static_cast<TileSpeed>(conf.getEnum("speed", speedEnumStrings));
    this->effect =
        static_cast<TileEffect>(conf.getEnum("effect", effectsEnumStrings));
    return true;
} // TileRule::initFromConf


/**
 * Tileset Class Implementation
 */

/* static member variables */
Tileset::TilesetMap Tileset::tilesets;


/**
 * Loads all tilesets using the filename
 * indicated by 'filename' as a definition
 */
void Tileset::loadAll()
{
    Debug dbg("debug/tileset.txt", "Tileset");
    const Config *config = Config::getInstance();
    std::vector<ConfigElement> conf;
    TRACE(dbg, "Unloading all tilesets");
    unloadAll();
    // get the config element for all tilesets
    TRACE_LOCAL(dbg, "Loading tilesets info from config");
    conf = config->getElement("tilesets").getChildren();
    // load tile rules
    TRACE_LOCAL(dbg, "Loading tile rules");
    if (!TileRule::rules.size()) {
        TileRule::load();
    }
    // load all of the tilesets
    for (std::vector<ConfigElement>::const_iterator i = conf.cbegin();
         i != conf.cend();
         ++i) {
        if (i->getName() == "tileset") {
            Tileset *tileset = new Tileset;
            tileset->load(*i);
            tilesets[tileset->name] = tileset;
        }
    }
    // load tile maps, including translations from index to id
    TRACE_LOCAL(dbg, "Loading tilemaps");
    TileMap::loadAll();
    TRACE(dbg, "Successfully Loaded Tilesets");
} // Tileset::loadAll


/**
 * Delete all tilesets
 */
void Tileset::unloadAll()
{
    TilesetMap::const_iterator i;
    // unload all tilemaps
    TileMap::unloadAll();
    TileRule::unloadAll();
    unloadAllImages();
    for (i = tilesets.cbegin(); i != tilesets.cend(); ++i) {
        i->second->unload();
        delete i->second;
    }
    tilesets.clear();
    Tile::resetNextId();
}


/**
 * Delete all tileset images
 */
void Tileset::unloadAllImages()
{
    TilesetMap::const_iterator i;
    for (i = tilesets.cbegin(); i != tilesets.cend(); ++i) {
        i->second->unloadImages();
    }
    Tile::resetNextId();
}


/**
 * Returns the tileset with the given name, if it exists
 */
Tileset *Tileset::get(const std::string &name)
{
    if (tilesets.find(name) != tilesets.end()) {
        return tilesets[name];
    } else {
        return nullptr;
    }
}


/**
 * Returns the tile that has the given name from any tileset, if there is one
 */
Tile *Tileset::findTileByName(const std::string &name)
{
    TilesetMap::const_iterator i;
    for (i = tilesets.cbegin(); i != tilesets.cend(); ++i) {
        Tile *t = i->second->getByName(name);
        if (t) {
            return t;
        }
    }
    return nullptr;
}

Tile *Tileset::findTileById(TileId id)
{
    TilesetMap::const_iterator i;
    for (i = tilesets.cbegin(); i != tilesets.cend(); ++i) {
        Tile *t = i->second->get(id);
        if (t) {
            return t;
        }
    }
    return nullptr;
}


/**
 * Loads a tileset.
 */
void Tileset::load(const ConfigElement &tilesetConf)
{
    Debug dbg("debug/tileset.txt", "Tileset", true);
    name = tilesetConf.getString("name");
    if (tilesetConf.exists("imageName")) {
        imageName = tilesetConf.getString("imageName");
    }
    if (tilesetConf.exists("extends")) {
        extends = Tileset::get(tilesetConf.getString("extends"));
    } else {
        extends = nullptr;
    }
    TRACE_LOCAL(dbg, "\tLoading Tiles...");
    int index = 0;
    std::vector<ConfigElement> children = tilesetConf.getChildren();
    for (std::vector<ConfigElement>::const_iterator i = children.cbegin();
         i != children.cend();
         ++i) {
        if (i->getName() != "tile") {
            continue;
        }
        Tile *tile = new Tile(this);
        tile->loadProperties(*i);
        TRACE_LOCAL(dbg, std::string("\t\tLoaded '") + tile->getName() + "'");
        /* add the tile to our tileset */
        tiles[tile->getId()] = tile;
        nameMap[tile->getName()] = tile;
        index += tile->getFrames();
    }
    totalFrames = index;
} // Tileset::load

void Tileset::unloadImages() const
{
    Tileset::TileIdMap::const_iterator i;
    /* free all the image memory and nullify so that reloading can
       automatically take place lazily */
    for (i = tiles.cbegin(); i != tiles.cend(); ++i) {
        i->second->deleteImage();
    }
}

/**
 * Unload the current tileset
 */
void Tileset::unload()
{
    Tileset::TileIdMap::const_iterator i;
    /* free all the memory for the tiles */
    for (i = tiles.cbegin(); i != tiles.cend(); ++i) {
        delete i->second;
    }
    tiles.clear();
    totalFrames = 0;
    imageName.erase();
}


/**
 * Returns the tile with the given id in the tileset
 */
Tile *Tileset::get(TileId id)
{
    if (tiles.find(id) != tiles.end()) {
        return tiles[id];
    } else if (extends) {
        return extends->get(id);
    }
    return nullptr;
}


/**
 * Returns the tile with the given name from the tileset, if it exists
 */
Tile *Tileset::getByName(const std::string &name)
{
    if (nameMap.find(name) != nameMap.end()) {
        return nameMap[name];
    } else if (extends) {
        return extends->getByName(name);
    } else {
        return nullptr;
    }
}


/**
 * Returns the image name for the tileset, if it exists
 */
std::string Tileset::getImageName() const
{
    if (imageName.empty() && extends) {
        return extends->getImageName();
    } else {
        return imageName;
    }
}


/**
 * Returns the number of tiles in the tileset
 */
unsigned int Tileset::numTiles() const
{
    return tiles.size();
}


/**
 * Returns the total number of frames in the tileset
 */
unsigned int Tileset::numFrames() const
{
    return totalFrames;
}
