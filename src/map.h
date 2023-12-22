/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "coords.h"
#include "direction.h"
#include "music.h"
#include "object.h"
#include "savegame.h"
#include "types.h"
#include "u4file.h"



#define MAP_IS_OOB(mapptr, c)                               \
    (((c).x) < 0                                            \
     || ((c).x) >= (static_cast<int>((mapptr)->width))      \
     || ((c).y) < 0                                         \
     || ((c).y) >= (static_cast<int>((mapptr)->height))     \
     || ((c).z) < 0                                         \
     || ((c).z) >= (static_cast<int>((mapptr)->levels)))

class AnnotationMgr;
class Map;
class Object;
class Person;
class Creature;
class TileMap;
class Tileset;
struct Portal;

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
    explicit MapCoords(int initx = 0, int inity = 0, int initz = 0)
        :Coords(initx, inity, initz),
         active_x(C2A(initx)),
         active_y(C2A(inity))
    {
    }

    MapCoords(const MapCoords &a)
        :Coords(a.x, a.y, a.z),
         active_x(a.active_x),
         active_y(a.active_y)
    {
    }

    // cppcheck-suppress noExplicitConstructor // implicit intended
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
        return static_cast<Coords>(*this) == static_cast<Coords>(a);
    }

    bool operator!=(const MapCoords &a) const
    {
        return static_cast<Coords>(*this) != static_cast<Coords>(a);
    }

    bool operator<(const MapCoords &a) const
    {
        return static_cast<Coords>(*this) < static_cast<Coords>(a);
    }

    MapCoords &wrap(const class Map *map);
    MapCoords &putInBounds(const class Map *map);
    MapCoords &move(Direction d, const class Map *map = nullptr);
    MapCoords &move(int dx, int dy, const class Map *map = nullptr);
    int getRelativeDirection(
        const MapCoords &c, const class Map *map = nullptr
    ) const;
    Direction pathTo(
        const MapCoords &c,
        int valid_dirs = MASK_DIR_ALL,
        bool towards = true,
        const class Map *map = nullptr,
        Direction last = DIR_NONE
    ) const;
    Direction pathAway(
        const MapCoords &c,
        int valid_dirs = MASK_DIR_ALL,
        const class Map *map = nullptr,
        Direction last = DIR_NONE
    ) const;
    int movementDistance(
        const MapCoords &c, const class Map *map = nullptr
    ) const;
    int distance(const MapCoords &c, const class Map *map = nullptr) const;
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
            :fname(), type(WORLD)
        {
        }

        Source(const std::string &f, Type t)
            :fname(f), type(t)
        {
        }

        std::string fname;
        Type type;
    };

    Map();
    virtual ~Map();
    virtual std::string getName() const;
    Object *objectAt(const Coords &coords) const;
    const Portal *portalAt(const Coords &coords, int actionFlags) const;
    MapTile getTileFromData(const Coords &coords) const;
    MapTile tileAt(const Coords &coords, int withObjects) const;
    const Tile *tileTypeAt(const Coords &coords, int withObjects) const;
    bool isWorldMap() const;
    bool isEnclosed(const Coords &party) const;
    class Creature *addCreature(const class Creature *m, const Coords &coords);
    Object *addObject(
        MapTile tile, MapTile prevtile, const Coords &coords
    );
    Object *addObject(Object *obj, const Coords &coords);
    void removeObject(const Object *rem, bool deleteObject = true);
    ObjectDeque::iterator removeObject(
        ObjectDeque::iterator rem, bool deleteObject = true
    );
    void clearObjects();
    class Creature *moveObjects(const MapCoords &avatar) const;
    void resetObjectAnimations() const;
    int getNumberOfCreatures(int level = -1) const;
    int getValidMoves(
        const MapCoords &from, MapTile transport, bool wanders = false
    ) const;
    static bool move(Object *obj, Direction d);
    void alertGuards() const;
    const MapCoords &getLabel(const std::string &name) const;
    // u4dos compatibility
    bool fillMonsterTable();
    /* Translate from raw tile index */
    MapTile tfrti(int raw) const;
    /* Translate to raw tile index */
    unsigned int ttrti(MapTile tile) const;

public:
    MapId id;
    std::string fname;
    Type type;
    unsigned int width, height, levels;
    unsigned int chunk_width, chunk_height;
    unsigned int offset;
    Source baseSource;
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
    SaveGameMonsterRecord monsterTable[MONSTERTABLE_SIZE];

private:
    void findWalkability(const Coords &coords, int *path_data) const;
};

#endif // ifndef MAP_H
