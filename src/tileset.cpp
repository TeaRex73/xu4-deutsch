/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include <vector>

#include "tileset.h"

#include "config.h"
#include "debug.h"
#include "direction.h"
#include "error.h"
#include "tile.h"
#include "tilemap.h"
#include "types.h"


/**
 * TileRule Class Implementation
 */
TileRuleMap TileRule::rules;


/**
 * Returns the tile rule with the given name, or nullptr if none could be found
 */
TileRule *TileRule::findByName(const std::string &name)
{
    const TileRuleMap::const_iterator i = rules.find(name);
    if (i != rules.cend()) {
        return i->second;
    }
    return nullptr;
}

void TileRule::unloadAll()
{
    for (const auto &rule: rules) {
        delete rule.second;
    }
    rules.clear();
}


/**
 * Load tile information from xml.
 */
void TileRule::load()
{
    const Config *config = Config::getInstance();
    const std::vector<ConfigElement> children =
        config->getElement("tileRules").getChildren();
    for (const auto &child: children) {
        auto *rule = new TileRule;
        rule->initFromConf(child);
        rules[rule->name] = rule;
    }

    if (findByName("default") == nullptr) {
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
        { .name = "dispel", .mask = MASK_DISPEL},
        { .name = "talkover", .mask = MASK_TALK_OVER},
        { .name = "door", .mask = MASK_DOOR},
        { .name = "lockeddoor", .mask = MASK_LOCKED_DOOR},
        { .name = "chest", .mask = MASK_CHEST},
        { .name = "ship", .mask = MASK_SHIP},
        { .name = "horse", .mask = MASK_HORSE},
        { .name = "balloon", .mask = MASK_BALLOON},
        { .name = "canattackover", .mask = MASK_ATTACK_OVER},
        { .name = "canlandballoon", .mask = MASK_CAN_LAND_BALLOON},
        { .name = "replacement", .mask = MASK_REPLACEMENT},
        { .name = "foreground", .mask = MASK_FOREGROUND},
        { .name = "onWaterOnlyReplacement", .mask = MASK_WATER_REPLACEMENT},
        { .name = "livingthing", .mask = MASK_LIVING_THING},
        { .name = "spawnslandmonster", .mask = MASK_SPAWNS_LAND_MONSTER},
        { .name = "spawnsseamonster", .mask = MASK_SPAWNS_SEA_MONSTER}
    };
    static const struct {
        const char *name;
        unsigned int mask;
    } movementBooleanAttr[] = {
        { .name = "swimmable", .mask = MASK_SWIMMABLE},
        { .name = "sailable", .mask = MASK_SAILABLE},
        { .name = "unflyable", .mask = MASK_UNFLYABLE},
        { .name = "creatureunwalkable", .mask = MASK_CREATURE_UNWALKABLE},
        { .name = "wontwanderon", .mask = MASK_WONT_WANDER_ON}
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
    this->walkOnDirs = MASK_DIR_ALL;
    this->walkOffDirs = MASK_DIR_ALL;
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
    const std::string cantWalkOn = conf.getString("cantwalkon");
    if (cantWalkOn == "all") {
        this->walkOnDirs = 0;
    } else if (cantWalkOn == "west") {
        this->walkOnDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, this->walkOnDirs);
    } else if (cantWalkOn == "north") {
        this->walkOnDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, this->walkOnDirs);
    } else if (cantWalkOn == "east") {
        this->walkOnDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, this->walkOnDirs);
    } else if (cantWalkOn == "south") {
        this->walkOnDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, this->walkOnDirs);
    } else if (cantWalkOn == "advance") {
        this->walkOnDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, this->walkOnDirs);
    } else if (cantWalkOn == "retreat") {
        this->walkOnDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, this->walkOnDirs);
    }
    const std::string cantWalkOff = conf.getString("cantwalkoff");
    if (cantWalkOff == "all") {
        this->walkOffDirs = 0;
    } else if (cantWalkOff == "west") {
        this->walkOffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, this->walkOffDirs);
    } else if (cantWalkOff == "north") {
        this->walkOffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, this->walkOffDirs);
    } else if (cantWalkOff == "east") {
        this->walkOffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, this->walkOffDirs);
    } else if (cantWalkOff == "south") {
        this->walkOffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, this->walkOffDirs);
    } else if (cantWalkOff == "advance") {
        this->walkOffDirs =
            DIR_REMOVE_FROM_MASK(DIR_ADVANCE, this->walkOffDirs);
    } else if (cantWalkOff == "retreat") {
        this->walkOffDirs =
            DIR_REMOVE_FROM_MASK(DIR_RETREAT, this->walkOffDirs);
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
    TRACE(dbg, "Unloading all tilesets");
    unloadAll();
    // get the config element for all tilesets
    TRACE_LOCAL(dbg, "Loading tilesets info from config");
    const std::vector<ConfigElement> children =
        config->getElement("tilesets").getChildren();
    // load tile rules
    TRACE_LOCAL(dbg, "Loading tile rules");
    if (TileRule::rules.empty()) {
        TileRule::load();
    }
    // load all of the tilesets
    for (const auto &child: children) {
        if (child.getName() == "tileset") {
            auto *tileset = new Tileset;
            tileset->load(child);
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
    // unload all tilemaps
    TileMap::unloadAll();
    TileRule::unloadAll();
    unloadAllImages();
    for (const auto& tileset: tilesets) {
        tileset.second->unload();
        delete tileset.second;
    }
    tilesets.clear();
    Tile::resetNextId();
}


/**
 * Delete all tileset images
 */
void Tileset::unloadAllImages()
{
    for (const auto &tileset: tilesets) {
        tileset.second->unloadImages();
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
    }
    return nullptr;
}


/**
 * Returns the tile that has the given name from any tileset, if there is one
 */
Tile *Tileset::findTileByName(const std::string &name)
{
    for (const auto &tileset: tilesets) {
        Tile *t = tileset.second->getByName(name);
        if (t) {
            return t;
        }
    }
    return nullptr;
}

Tile *Tileset::findTileById(const TileId id)
{
    for (const auto &tileset: tilesets) {
        Tile *t = tileset.second->get(id);
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
        extends = get(tilesetConf.getString("extends"));
    } else {
        extends = nullptr;
    }
    TRACE_LOCAL(dbg, "\tLoading Tiles...");
    int index = 0;
    const std::vector<ConfigElement> children = tilesetConf.getChildren();
    for (const auto &child: children) {
        if (child.getName() != "tile") {
            continue;
        }
        auto *tile = new Tile(this);
        tile->loadProperties(child);
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
    /* free all the image memory and nullify so that reloading can
       automatically take place lazily */
    for (auto &tile: tiles) {
        tile.second->deleteImage();
    }
}

/**
 * Unload the current tileset
 */
void Tileset::unload()
{
    /* free all the memory for the tiles */
    for (const auto &tile: tiles) {
        delete tile.second;
    }
    tiles.clear();
    totalFrames = 0;
    imageName.erase();
}


/**
 * Returns the tile with the given id in the tileset
 */
Tile *Tileset::get(const TileId id)
{
    if (tiles.find(id) != tiles.end()) {
        return tiles[id];
    }
    if (extends) {
        return extends->get(id);
    }
    return nullptr;
}


/**
 * Returns the tile with the given name from the tileset, if it exists
 */
Tile *Tileset::getByName(const std::string &nameToGet)
{
    if (nameMap.find(nameToGet) != nameMap.end()) {
        return nameMap[nameToGet];
    }
    if (extends) {
        return extends->getByName(nameToGet);
    }
    return nullptr;
}


/**
 * Returns the image name for the tileset, if it exists
 */
std::string Tileset::getImageName() const
{
    if (imageName.empty() && extends) {
        return extends->getImageName();
    }
    return imageName;
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
