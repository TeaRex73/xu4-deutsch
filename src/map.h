/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "coords.h"
#include "direction.h"
#include "music.h"
#include "object.h"
#include "savegame.h"
#include "types.h"

class AnnotationMgr;
class Location;
class Map;
class MapCoords;
class Tile;
class TileMap;
class Tileset;
class Portal;


#define MAP_IS_OOB(mapPtr, c)                               \
    (((c).x) < 0                                            \
     || ((c).x) >= (static_cast<int>((mapPtr)->width))      \
     || ((c).y) < 0                                         \
     || ((c).y) >= (static_cast<int>((mapPtr)->height))     \
     || ((c).z) < 0                                         \
     || ((c).z) >= (static_cast<int>((mapPtr)->levels)))

typedef std::vector<Portal *> PortalList;
typedef std::list<int> CompressedChunkList;
typedef std::vector<MapTile> MapData;

/* flags */
#define SHOW_AVATAR (1 << 0)
#define NO_LINE_OF_SIGHT (1 << 1)
#define FIRST_PERSON (1 << 2)

/* mapTileAt flags */
#define WITHOUT_OBJECTS 0
#define WITH_GROUND_OBJECTS 1
#define WITH_OBJECTS 2

// Coordinates to upper left of 4 active chunks
#define C2A(n) \
    ((static_cast<unsigned int>(n) & 0xFu) >= 8u ? \
     ((static_cast<unsigned int>(n) >> 4u) & 0xFu) : \
     (((static_cast<unsigned int>(n) >> 4u) - 1u) & 0xFu))

/**
 * MapCoords class
 */
class MapCoords:public Coords {
public:
    explicit MapCoords(
        const int initX = 0, const int initY = 0, const int initZ = 0
    )
        :Coords(initX, initY, initZ),
         active_x(C2A(initX)),
         active_y(C2A(initY))
    {
    }

    MapCoords(const MapCoords &a) = default;

    // cppcheck-suppress noExplicitConstructor // implicit intended
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    MapCoords(const Coords &a)
        :Coords(a.x, a.y, a.z),
         active_x(C2A(a.x)),
         active_y(C2A(a.y))
    {
    }

    MapCoords &operator=(const MapCoords &a)
    {
        if (&a != this) {
            x = a.x;
            y = a.y;
            z = a.z;
            active_x = a.active_x;
            active_y = a.active_y;
        }
        return *this;
    }

    MapCoords &operator=(const Coords &a)
    {
        if (&a != static_cast<Coords *>(this)) {
            x = a.x;
            y = a.y;
            z = a.z;
            active_x = C2A(a.x);
            active_y = C2A(a.y);
        }
        return *this;
    }

    bool operator==(const MapCoords &a) const
    {
        return static_cast<const Coords &>(*this) == a;
    }

    bool operator!=(const MapCoords &a) const
    {
        return static_cast<const Coords &>(*this) != a;
    }

    bool operator<(const MapCoords &a) const
    {
        return static_cast<const Coords &>(*this) < a;
    }

    MapCoords &wrap(const Map *map);
    MapCoords &putInBounds(const Map *map);
    MapCoords &move(Direction d, const Map *map = nullptr);
    MapCoords &move(int dx, int dy, const Map *map = nullptr);
    int getRelativeDirection(
        const MapCoords &mc, const Map *map = nullptr
    ) const;
    Direction pathTo(
        const MapCoords &mc,
        int valid_directions = MASK_DIR_ALL,
        bool towards = true,
        const Map *map = nullptr,
        Direction last = DIR_NONE
    ) const;
    Direction pathAway(
        const MapCoords &mc,
        int valid_directions = MASK_DIR_ALL,
        const Map *map = nullptr,
        Direction last = DIR_NONE
    ) const;
    int movementDistance(const MapCoords &mc, const Map *map) const;
    int distance(const MapCoords &mc, const Map *map) const;
    static MapCoords nowhere;
    unsigned int active_x, active_y;
};

/**
 * Map class
 */
class Map {
public:
    // disallow map copying: all maps should be created and accessed
    // through the MapMgr
    friend class MapCoords;
    Map(const Map &) = delete;
    Map(Map &&) = delete;
    Map &operator=(const Map &) = delete;
    Map &operator=(Map &&) = delete;

    enum Type {
        WORLD,
        CITY,
        SHRINE,
        COMBAT,
        DUNGEON
    };

    enum BorderBehavior {
        BORDER_WRAP,
        BORDER_EXIT2PARENT,
        BORDER_FIXED
    };

    class Source {
    public:
        Source()
            : type(WORLD)
        {
        }

        Source(std::string f, const Type t)
            :file_name(std::move(f)), type(t)
        {
        }

        std::string file_name;
        Type type;
    };

    Map();
    virtual ~Map();
    virtual std::string getName();

    Object *objectAt(const Coords &coords) const;
    const Portal *portalAt(const Coords &coords, int actionFlags) const;
    MapTile getTileFromData(const Coords &coords) const;
    MapTile tileAt(const Coords &coords, int withObjects) const;
    const Tile *tileTypeAt(const Coords &coords, int withObjects) const;
    bool isWorldMap() const;
    bool isCityMap() const;
    bool isDungeonMap() const;
    bool isShrineMap() const;
    bool isCombatMap() const;
    bool isEnclosed(const Coords &party) const;
    class Creature *addCreature(
        const Creature *creature, const Coords &coords
    );
    Object *addObject(
        MapTile tile, MapTile previousTile, const Coords &coords
    );
    Object *addObject(Object *obj, const Coords &coords);
    void removeObject(const Object *rem, bool deleteObject = true);
    ObjectDeque::iterator removeObject(
        ObjectDeque::iterator rem, bool deleteObject = true
    );
    void clearObjects();

    Creature *moveObjects(const MapCoords &avatar) const;
    void resetObjectAnimations() const;
    int getNumberOfCreatures(int level = -1) const;
    int getValidMoves(
        const MapCoords &from, MapTile transport, bool wanders = false
    ) const;
    static bool move(Object *obj, Direction d);
    void alertGuards() const;
    const MapCoords &getLabel(const std::string &name) const;
    // u4dos compatibility
    bool fillMonsterTable(const Location *loc);
    /* Translate from raw tile index */
    MapTile translateFromRawTile(int raw) const;
    /* Translate to raw tile index */
    int translateToRawTile(MapTile tile) const;

    MapId id;
    std::string file_name;
    Type type;
    int width, height, levels;
    int chunk_width, chunk_height;
    int offset;
    Source base_source;
    CompressedChunkList compressed_chunks;
    BorderBehavior border_behavior;
    PortalList portals;
    AnnotationMgr *annotations;
    int flags;
    Music::Type music;
    MapData data;
    ObjectDeque objects;
    std::map<std::string, MapCoords> labels;
    Tileset *tileset;
    TileMap *tilemap;
    // u4dos compatibility
    SaveGameMonsterRecord monster_table[MONSTER_TABLE_SIZE];

private:
    void findWalkability(const Coords &coords, int *path_data) const;
};

#endif // MAP_H
