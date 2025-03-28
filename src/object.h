/*
 * $Id$
 */

#ifndef OBJECT_H
#define OBJECT_H

#include <cstdlib>
#include <deque>
#include <set>
#include "coords.h"
#include "direction.h"
#include "debug.h"
#include "tile.h"
#include "types.h"

typedef std::deque<class Object *> ObjectDeque;

typedef enum {
    MOVEMENT_FIXED,
    MOVEMENT_WANDER,
    MOVEMENT_FOLLOW_AVATAR,
    MOVEMENT_ATTACK_AVATAR
} ObjectMovementBehavior;

class Object {
public:
    enum Type {
        UNKNOWN,
        CREATURE,
        PERSON,
        PARTYMEMBER
    };

    explicit Object(Type type = UNKNOWN);
    Object(const Object &o);
    Object &operator=(const Object &o);
    virtual ~Object();
    static void cleanup();

    MapTile getTile() const
    {
        return tile;
    }

    MapTile getPrevTile() const
    {
        return prevTile;
    }

    const Coords &getCoords() const
    {
        return coords;
    }

    const Coords &getPrevCoords() const
    {
        return prevCoords;
    }

    Direction getLastDir() const;

    ObjectMovementBehavior getMovementBehavior() const
    {
        return movement_behavior;
    }

    Type getType() const
    {
        return objType;
    }

    bool hasFocus() const
    {
        return focused;
    }

    bool isVisible() const
    {
        return visible;
    }

    bool isAnimated() const
    {
        return animated;
    }

    void setTile(MapTile t)
    {
        tile = t;
    }

    void setTile(const Tile *t)
    {
        tile = t->getId();
    }

    void setPrevTile(MapTile t)
    {
        prevTile = t;
    }

    void setCoords(const Coords &c);

    void setPrevCoords(const Coords &c)
    {
        prevCoords = c;
    }

    void setMovementBehavior(ObjectMovementBehavior b)
    {
        movement_behavior = b;
    }

    void setType(Type t)
    {
        objType = t;
    }

    void setFocus(bool f = true)
    {
        focused = f;
    }

    void setVisible(bool v = true)
    {
        visible = v;
    }

    void setAnimated(bool a = true)
    {
        animated = a;
    }

    void setMap(class Map *m);
    Map *getMap();
    void remove(); /**< remove self from any maps that it's part of */
    bool setDirection(Direction d);
    void animateMovement() const;

protected:
    MapTile tile, prevTile;
    Coords coords, prevCoords;
    ObjectMovementBehavior movement_behavior;
    Type objType;
    std::deque<class Map *> maps; /**< maps that this object is part of */
    bool focused;
    bool visible;
    bool animated;
    static std::set<Object *> all_objects;
};

#endif // ifndef OBJECT_H
