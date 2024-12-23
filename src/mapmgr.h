/*
 * $Id$
 */

#ifndef MAPMGR_H
#define MAPMGR_H

#include <vector>
#include <utility>

#include "map.h"

class City;
class ConfigElement;
class Debug;
class Dungeon;
struct PersonRole;
struct Portal;
class Shrine;


/*
 * The map manager is responsible for loading and keeping track of the
 * various maps.
 */
#define MAP_NONE 255
#define MAP_WORLD 0
#define MAP_CASTLE_OF_LORD_BRITISH 1
#define MAP_LYCAEUM 2
#define MAP_EMPATH_ABBEY 3
#define MAP_SERPENTS_HOLD 4
#define MAP_MOONGLOW 5
#define MAP_BRITAIN 6
#define MAP_JHELOM 7
#define MAP_YEW 8
#define MAP_MINOC 9
#define MAP_TRINSIC 10
#define MAP_SKARABRAE 11
#define MAP_MAGINCIA 12
#define MAP_PAWS 13
#define MAP_BUCCANEERS_DEN 14
#define MAP_VESPER 15
#define MAP_COVE 16
#define MAP_DECEIT 17
#define MAP_DESPISE 18
#define MAP_DESTARD 19
#define MAP_WRONG 20
#define MAP_COVETOUS 21
#define MAP_SHAME 22
#define MAP_HYTHLOTH 23
#define MAP_ABYSS 24
#define MAP_SHRINE_HONESTY 25
#define MAP_SHRINE_COMPASSION 26
#define MAP_SHRINE_VALOR 27
#define MAP_SHRINE_JUSTICE 28
#define MAP_SHRINE_SACRIFICE 29
#define MAP_SHRINE_HONOR 30
#define MAP_SHRINE_SPIRITUALITY 31
#define MAP_SHRINE_HUMILITY 32
#define MAP_BRICK_CON 33
#define MAP_BRIDGE_CON 34
#define MAP_BRUSH_CON 35
#define MAP_CAMP_CON 36
#define MAP_DNG0_CON 37
#define MAP_DNG1_CON 38
#define MAP_DNG2_CON 39
#define MAP_DNG3_CON 40
#define MAP_DNG4_CON 41
#define MAP_DNG5_CON 42
#define MAP_DNG6_CON 43
#define MAP_DUNGEON_CON 44
#define MAP_FOREST_CON 45
#define MAP_GRASS_CON 46
#define MAP_HILL_CON 47
#define MAP_INN_CON 48
#define MAP_MARSH_CON 49
#define MAP_SHIPSEA_CON 50
#define MAP_SHIPSHIP_CON 51
#define MAP_SHIPSHOR_CON 52
#define MAP_SHORE_CON 53
#define MAP_SHORSHIP_CON 54
#define MAP_CAMP_DNG 55


/**
 * The map manager singleton that keeps track of all the maps.
 */
class MapMgr {
public:
    static MapMgr *getInstance();
    static void destroy();
    Map *get(MapId id);
    static Map *initMap(Map::Type type);
    void unloadMap(MapId id);

private:
    MapMgr();
    MapMgr(const MapMgr &) = delete;
    MapMgr(MapMgr &&) = delete;
    MapMgr &operator=(const MapMgr &) = delete;
    MapMgr &operator=(MapMgr &&) = delete;
    ~MapMgr();
    void registerMap(Map *map);
    Map *initMapFromConf(const ConfigElement &mapConf);
    static void initCityFromConf(const ConfigElement &cityConf, City *city);
    static PersonRole *initPersonRoleFromConf(
        const ConfigElement &personRoleConf
    );
    static Portal *initPortalFromConf(const ConfigElement &portalConf);
    static void initShrineFromConf(
        const ConfigElement &shrineConf, Shrine *shrine
    );
    static void initDungeonFromConf(
        const ConfigElement &dungeonConf, Dungeon *dungeon
    );
    void initDungeonRoom(Dungeon *dng, int room);
    static void createMoongateFromConf(const ConfigElement &moongateConf);
    static int initCompressedChunkFromConf(
        const ConfigElement &compressedChunkConf
    );
    static std::pair<std::string, MapCoords> initLabelFromConf(
        const ConfigElement &labelConf
    );
    static MapMgr *instance;
    std::vector<Map *> mapList;
    Debug *logger;
};

#define mapMgr (MapMgr::getInstance())

#endif /* MAPMGR_H */
