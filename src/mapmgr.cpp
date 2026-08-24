/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "mapmgr.h"

#include "city.h"
#include "combat.h"
#include "config.h"
#include "context.h"
#include "coords.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "item.h"
#include "map.h"
#include "maploader.h"
#include "moongate.h"
#include "music.h"
#include "person.h"
#include "portal.h"
#include "shrine.h"
#include "tilemap.h"
#include "tileset.h"
#include "types.h"

enum Virtue: unsigned char;


MapMgr *MapMgr::instance = nullptr;

MapMgr *MapMgr::getInstance()
{
    if (__builtin_expect(instance == nullptr, false)) {
        instance = new MapMgr();
    }
    return instance;
}

void MapMgr::destroy()
{
    delete instance;
    instance = nullptr;
}

MapMgr::MapMgr()
    :logger(new Debug("debug/mapmgr.txt", "MapMgr"))
{
    TRACE(*logger, "creating MapMgr");
    const Config *config = Config::getInstance();
    const std::vector<ConfigElement> mapElements =
        config->getElement("maps").getChildren();
    for (const auto &mapElement: mapElements) {
        Map *map = initMapFromConf(mapElement);
        /* map actually gets loaded later, when it's needed */
        registerMap(map);
    }
}

MapMgr::~MapMgr()
{
    for (const auto *map: mapList) {
        delete map;
    }
    delete logger;
}

void MapMgr::unloadMap(const MapId id)
{
    delete mapList[id];
    const Config *config = Config::getInstance();
    const std::vector<ConfigElement> mapElements =
        config->getElement("maps").getChildren();
    const auto mapElement = std::find_if(
        mapElements.cbegin(),
        mapElements.cend(),
        [&](const ConfigElement &v) -> bool {
            return id == static_cast<MapId>(v.getInt("id"));
        }
    );
    if (mapElement != mapElements.cend()) {
        Map *map = initMapFromConf(*mapElement);
        mapList[id] = map;
    }
}

Map *MapMgr::initMap(const Map::Type type)
{
    Map *map;
    switch (type) {
    case Map::WORLD:
        map = new Map;
        break;
    case Map::COMBAT:
        map = new CombatMap;
        break;
    case Map::SHRINE:
        map = new Shrine;
        break;
    case Map::DUNGEON:
        map = new Dungeon;
        break;
    case Map::CITY:
        map = new City;
        break;
    default:
        map = nullptr;
        errorFatal("Error: invalid map type used");
        break;
    }
    return map;
}

Map *MapMgr::get(const MapId id) const
{
    /* if the map hasn't been loaded yet, load it! */
    if (mapList[id]->data.empty()) {
        MapLoader *loader = MapLoader::getLoader(mapList[id]->type);
        if (loader == nullptr) {
            errorFatal("can't load map of type \"%d\"", mapList[id]->type);
        }
        TRACE_LOCAL(
            *logger,
            std::string("loading map data for map \'")
            + mapList[id]->file_name
            + "\'"
        );
        loader->load(mapList[id]);
    }
    return mapList[id];
}

void MapMgr::registerMap(Map *map)
{
    if (mapList.size() <= map->id) {
        mapList.resize(map->id + 1, nullptr);
    }
    if (mapList[map->id] != nullptr) {
        errorFatal("Error: A map with id '%d' already exists", map->id);
    }
    mapList[map->id] = map;
}

Map *MapMgr::initMapFromConf(const ConfigElement &mapConf) const
{
    static const char *mapTypeEnumStrings[] = {
        "world",
        "city",
        "shrine",
        "combat",
        "dungeon",
        nullptr
    };
    static const char *borderBehaviorEnumStrings[] = {
        "wrap",
        "exit",
        "fixed",
        nullptr
    };
    Map *map = initMap(
        static_cast<Map::Type>(mapConf.getEnum("type", mapTypeEnumStrings))
    );
    if (!map) {
        return nullptr;
    }
    map->id = static_cast<MapId>(mapConf.getInt("id"));
    map->type = static_cast<Map::Type>(
        mapConf.getEnum("type", mapTypeEnumStrings)
    );
    map->file_name = mapConf.getString("fname");
    map->width = mapConf.getInt("width");
    map->height = mapConf.getInt("height");
    map->levels = mapConf.getInt("levels");
    map->chunk_width = mapConf.getInt("chunkwidth");
    map->chunk_height = mapConf.getInt("chunkheight");
    map->offset = mapConf.getInt("offset");
    map->border_behavior = static_cast<Map::BorderBehavior>(
        mapConf.getEnum("borderbehavior", borderBehaviorEnumStrings)
    );
    if (map->isCombatMap()) {
        CombatMap *cm = getCombatMap(map);
        cm->setContextual(mapConf.getBool("contextual"));
    }
    TRACE_LOCAL(
        *logger,
        std::string("loading configuration for map \'") + map->file_name + "\'"
    );
    if (mapConf.getBool("showavatar")) {
        map->flags |= SHOW_AVATAR;
    }
    if (mapConf.getBool("nolineofsight")) {
        map->flags |= NO_LINE_OF_SIGHT;
    }
    if (mapConf.getBool("firstperson")) {
        map->flags |= FIRST_PERSON;
    }
    map->music = static_cast<Music::Type>(mapConf.getInt("music"));
    map->tileset = Tileset::get(mapConf.getString("tileset"));
    map->tilemap = TileMap::get(mapConf.getString("tilemap"));
    const std::vector<ConfigElement> children = mapConf.getChildren();
    for (const auto &child: children) {
        if (child.getName() == "city") {
            auto *city = dynamic_cast<City *>(map);
            initCityFromConf(child, city);
        } else if (child.getName() == "shrine") {
            auto *shrine = dynamic_cast<Shrine *>(map);
            initShrineFromConf(child, shrine);
        } else if (child.getName() == "dungeon") {
            auto *dungeon = dynamic_cast<Dungeon *>(map);
            initDungeonFromConf(child, dungeon);
        } else if (child.getName() == "portal") {
            map->portals.push_back(initPortalFromConf(child));
        } else if (child.getName() == "moongate") {
            createMoongateFromConf(child);
        } else if (child.getName() == "compressedchunk") {
            map->compressed_chunks.push_back(
                initCompressedChunkFromConf(child)
            );
        } else if (child.getName() == "label") {
            map->labels.insert(initLabelFromConf(child));
        }
    }
    return map;
} // MapMgr::initMapFromConf

void MapMgr::initCityFromConf(const ConfigElement &cityConf, City *city)
{
    city->name = cityConf.getString("name");
    city->cityType = cityConf.getString("type");
    city->tlkFileName = cityConf.getString("tlk_fname");
    const std::vector<ConfigElement> children = cityConf.getChildren();
    for (const auto &child: children) {
        if (child.getName() == "personrole") {
            city->personroles.push_back(initPersonRoleFromConf(child));
        }
    }
}

PersonRole *MapMgr::initPersonRoleFromConf(const ConfigElement &personRoleConf)
{
    static const char *roleEnumStrings[] = {
        "companion",
        "weaponsvendor",
        "armorvendor",
        "foodvendor",
        "tavernkeeper",
        "reagentsvendor",
        "healer",
        "innkeeper",
        "guildvendor",
        "horsevendor",
        "lordbritish",
        "hawkwind",
        nullptr
    };
    auto *personrole = new PersonRole;
    personrole->role =
        personRoleConf.getEnum("role", roleEnumStrings) + NPC_TALKER_COMPANION;
    personrole->id = personRoleConf.getInt("id");
    return personrole;
}

Portal *MapMgr::initPortalFromConf(const ConfigElement &portalConf)
{
    auto *portal = new Portal;
    portal->portalConditionsMet = nullptr;
    portal->retroActiveDest = nullptr;
    portal->coords = MapCoords(
        portalConf.getInt("x"),
        portalConf.getInt("y"),
        portalConf.getInt("z", 0)
    );
    portal->destid = static_cast<MapId>(portalConf.getInt("destmapid"));
    portal->start.x =
        static_cast<unsigned short>(portalConf.getInt("startx"));
    portal->start.y =
        static_cast<unsigned short>(portalConf.getInt("starty"));
    portal->start.z =
        static_cast<unsigned short>(portalConf.getInt("startlevel", 0));
    std::string prop = portalConf.getString("action");
    if (prop == "none") {
        portal->trigger_action = ACTION_NONE;
    } else if (prop == "enter") {
        portal->trigger_action = ACTION_ENTER;
    } else if (prop == "klimb") {
        portal->trigger_action = ACTION_KLIMB;
    } else if (prop == "descend") {
        portal->trigger_action = ACTION_DESCEND;
    } else if (prop == "exit_north") {
        portal->trigger_action = ACTION_EXIT_NORTH;
    } else if (prop == "exit_east") {
        portal->trigger_action = ACTION_EXIT_EAST;
    } else if (prop == "exit_south") {
        portal->trigger_action = ACTION_EXIT_SOUTH;
    } else if (prop == "exit_west") {
        portal->trigger_action = ACTION_EXIT_WEST;
    } else {
        errorFatal("unknown trigger_action: %s", prop.c_str());
    }
    prop = portalConf.getString("condition");
    if (!prop.empty()) {
        if (prop == "shrine") {
            portal->portalConditionsMet = &shrineCanEnter;
        } else if (prop == "abyss") {
            portal->portalConditionsMet = &isAbyssOpened;
        } else {
            errorFatal("unknown portalConditionsMet: %s", prop.c_str());
        }
    }
    portal->saveLocation = portalConf.getBool("savelocation");
    portal->message = portalConf.getString("message");
    prop = portalConf.getString("transport");
    if (prop == "foot") {
        portal->portalTransportRequisites = TRANSPORT_FOOT;
    } else if (prop == "footorhorse") {
        portal->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
    } else {
        errorFatal("unknown transport: %s", prop.c_str());
    }
    portal->exitPortal = portalConf.getBool("exits");
    const std::vector<ConfigElement> children = portalConf.getChildren();
    for (const auto &child: children) {
        if (child.getName() == "retroActiveDest") {
            portal->retroActiveDest = new PortalDestination;
            portal->retroActiveDest->coords =
                MapCoords(
                    child.getInt("x"),
                    child.getInt("y"),
                    child.getInt("z",0)
                );
            portal->retroActiveDest->mapid =
                static_cast<MapId>(child.getInt("mapid"));
        }
    }
    return portal;
} // MapMgr::initPortalFromConf

void MapMgr::initShrineFromConf(
    const ConfigElement &shrineConf, Shrine *shrine
)
{
    static const char *virtues[] = {
        "HONESTY",
        "COMPASSION",
        "VALOR",
        "JUSTICE",
        "SACRIFICE",
        "HONOR",
        "SPIRITUALITY",
        "HUMILITY",
        nullptr
    };
    shrine->setVirtue(
        static_cast<Virtue>(shrineConf.getEnum("virtue", virtues))
    );
    shrine->setMantra(shrineConf.getString("mantra"));
}

void MapMgr::initDungeonFromConf(
    const ConfigElement &dungeonConf, Dungeon *dungeon
)
{
    dungeon->n_rooms = dungeonConf.getInt("rooms");
    dungeon->rooms = nullptr;
    dungeon->roomMaps = nullptr;
    dungeon->name = dungeonConf.getString("name");
}

void MapMgr::createMoongateFromConf(const ConfigElement &moongateConf)
{
    const int phase = moongateConf.getInt("phase");
    const Coords coords(moongateConf.getInt("x"), moongateConf.getInt("y"));
    moongateAdd(phase, coords);
}

int MapMgr::initCompressedChunkFromConf(
    const ConfigElement &compressedChunkConf
)
{
    return compressedChunkConf.getInt("index");
}

std::pair<std::string, MapCoords> MapMgr::initLabelFromConf(
    const ConfigElement &labelConf
)
{
    return {
        labelConf.getString("name"),
        MapCoords(
            labelConf.getInt("x"),
            labelConf.getInt("y"),
            labelConf.getInt("z", 0)
        )
    };
}
