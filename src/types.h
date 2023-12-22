/**
 * $Id$
 */

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "direction.h"

class Tile;
typedef unsigned short TileId;
typedef unsigned char MapId;

typedef enum {
    FAST,
    SLOW,
    VSLOW,
    VVSLOW
} TileSpeed;

typedef enum {
    EFFECT_NONE,
    EFFECT_FIRE,
    EFFECT_SLEEP,
    EFFECT_POISON,
    EFFECT_POISONFIELD,
    EFFECT_ELECTRICITY,
    EFFECT_LAVA
} TileEffect;

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL,
    ANIM_CAMPFIRE,
    ANIM_CITYFLAG,
    ANIM_CASTLEFLAG,
    ANIM_SHIPFLAG,
    ANIM_LCBFLAG,
    ANIM_FRAMES
} TileAnimationStyle;


/**
 * A MapTile is a specific instance of a Tile.
 */
class __attribute__((aligned (4))) MapTile {
public:
    MapTile()
        :id(0), frame(0), freezeAnimation(false)
    {
    }

    // cppcheck-suppress noExplicitConstructor //implicit intended
    MapTile(TileId i, unsigned char f = 0)
        :id(i), frame(f), freezeAnimation(false)
    {
    }

    MapTile(const MapTile &t)
        :id(t.id), frame(t.frame), freezeAnimation(t.freezeAnimation)
    {
    }

    MapTile &operator=(const MapTile &t)
    {
        if (this != &t) {
            id = t.id;
            frame = t.frame;
            freezeAnimation = t.freezeAnimation;
        }
        return *this;
    }

    TileId getId() const
    {
        return id;
    }

    unsigned char getFrame() const
    {
        return frame;
    }

    void setFrame(unsigned char f)
    {
        frame = f;
    }

    bool getFreezeAnimation() const
    {
        return freezeAnimation;
    }

    void setFreezeAnimation(bool f)
    {
        freezeAnimation = f;
    }

    bool operator==(const MapTile &m) const
    {
        return id == m.id;
    }

    bool operator==(const TileId &i) const
    {
        return id == i;
    }

    bool operator!=(const MapTile &m) const
    {
        return id != m.id;
    }

    bool operator!=(const TileId &i) const
    {
        return id != i;
    }

    bool operator<(const MapTile &m) const
    {
        return id < m.id; /* for std::less */
    }

    Direction getDirection() const;
    bool setDirection(Direction d);
    const Tile *getTileType() const;
 private:
    TileId id;
    unsigned char frame;
    bool freezeAnimation;
};

#endif // ifndef TYPEDEFS_H
