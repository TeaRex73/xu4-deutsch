/*
 * $Id$
 */

#ifndef OBJECT_H
#define OBJECT_H

#include <atomic>
#include <cstdio>
#include <deque>
#include <unordered_set>

#include "coords.h"
#include "debug.h"
#include "direction.h"
#include "tile.h"
#include "types.h"


template <typename T>
class Registered {
protected:
    Registered()
    {
        if (cleaning) return;
        registrees.insert(static_cast<T *>(this));
    }

    Registered(const Registered &)
    {
        if (cleaning) return;
        registrees.insert(static_cast<T *>(this));
    }

    Registered(Registered &&) noexcept
    {
        if (cleaning) return;
        registrees.insert(static_cast<T *>(this));
    }

    Registered &operator=(const Registered &) = default;
    Registered &operator=(Registered &&) noexcept = default;

    ~Registered()
    {
        if (cleaning) return;
        const bool found =
            static_cast<bool>(registrees.erase(static_cast<T *>(this)));
        U4ASSERT(found, "Tried to delete non-existing Object\n");
    }

public:
    static void cleanup()
    {
        if (cleaning.exchange(true)) return;
        for (const auto *reg: registrees) {
            delete reg;
        }
        registrees.clear();
        cleaning = false;
    }

private:
    static std::unordered_set<T *> registrees;
    static std::atomic_bool cleaning;
};

template <typename T>
std::unordered_set<T *> Registered<T>::registrees {
    [] {
        std::unordered_set<T *> tmp;
        tmp.reserve(512);
        return tmp;
    }()
};

template <typename T>
std::atomic_bool Registered<T>::cleaning {false};


typedef std::deque<class Object *> ObjectDeque;

typedef enum {
    MOVEMENT_FIXED,
    MOVEMENT_WANDER,
    MOVEMENT_FOLLOW_AVATAR,
    MOVEMENT_ATTACK_AVATAR
} ObjectMovementBehavior;

class Object: public Registered<Object> {
public:
    enum Type {
        UNKNOWN,
        CREATURE,
        PERSON,
        PARTY_MEMBER
    };

    explicit Object(Type type = UNKNOWN);

    Object(const Object &) = default;
    Object(Object &&) noexcept = default;
    Object &operator=(const Object &) = default;
    Object &operator=(Object &&) noexcept = default;
    // ReSharper disable once CppHidingFunction
    virtual ~Object();

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

    void setTile(const MapTile t)
    {
        tile = t;
    }

    void setTile(const Tile *t)
    {
        tile = t->getId();
    }

    void setPrevTile(const MapTile t)
    {
        prevTile = t;
    }

    void setCoords(const Coords &co);

    void setPrevCoords(const Coords &pc)
    {
        prevCoords = pc;
    }

    void setMovementBehavior(const ObjectMovementBehavior b)
    {
        movement_behavior = b;
    }

    void setType(const Type t)
    {
        objType = t;
    }

    void setFocus(const bool f = true)
    {
        focused = f;
    }

    void setVisible(const bool v = true)
    {
        visible = v;
    }

    void setAnimated(const bool a = true)
    {
        animated = a;
    }

    void setMap(class Map *m);
    Map *getMap() const;
    void remove() const; /**< remove self from any maps that it's part of */
    bool setDirection(Direction d);
    void animateMovement() const;

protected:
    std::deque<Map *> maps; /**< maps that this object is part of */
    Coords coords, prevCoords;
    MapTile tile = 0, prevTile = 0;
    ObjectMovementBehavior movement_behavior = MOVEMENT_FIXED;
    Type objType = UNKNOWN;
    bool focused = false;
    bool visible = true;
    bool animated = true;
};

#endif // OBJECT_H
