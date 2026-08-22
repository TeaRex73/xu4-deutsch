/**
 * $Id$
 */

#ifndef TYPES_H
#define TYPES_H

#include "direction.h"

class Tile;
typedef unsigned short TileId;
typedef unsigned char MapId;

typedef enum {
    FAST,
    SLOW,
    VERY_SLOW,
    VERY_VERY_SLOW
} TileSpeed;

typedef enum {
    EFFECT_NONE,
    EFFECT_FIRE,
    EFFECT_SLEEP,
    EFFECT_SWAMP,
    EFFECT_POISON,
    EFFECT_ELECTRICITY,
    EFFECT_LAVA
} TileEffect;

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL,
    ANIM_CAMPFIRE,
    ANIM_CITY_FLAG,
    ANIM_CASTLE_FLAG,
    ANIM_SHIP_FLAG,
    ANIM_LCB_FLAG,
    ANIM_FRAMES
} TileAnimationStyle;


/**
 * A MapTile is a specific instance of a Tile.
 */
class alignas(int) MapTile {
public:
    MapTile()
        :id(0), frame(0), freezeAnimation(false)
    {
    }

    // cppcheck-suppress noExplicitConstructor //implicit intended
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    MapTile(const TileId i, const unsigned char f = 0)
        :id(i), frame(f), freezeAnimation(false)
    {
    }

    MapTile(const MapTile &t) = default;

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

    void setFrame(const unsigned char f)
    {
        frame = f;
    }

    bool getFreezeAnimation() const
    {
        return freezeAnimation;
    }

    void setFreezeAnimation(const bool f)
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

#endif // TYPES_H
