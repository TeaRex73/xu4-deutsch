/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>

#include "object.h"

#include "context.h"
#include "direction.h"
#include "game.h"
#include "map.h"
#include "screen.h"

Object::Object(const Type type)
    : objType(type)
{
}

Object::~Object()
{
    if(c && c->lastShip == this) {
        c->lastShip = nullptr;
    }
}

void Object::setCoords(const Coords &co)
{
    prevCoords = coords;
    coords = co;
}

bool Object::setDirection(const Direction d)
{
    return tile.setDirection(d);
}

void Object::setMap(Map *m)
{
    if (std::find(maps.cbegin(), maps.cend(), m) == maps.cend()) {
        maps.push_back(m);
    }
}

Map *Object::getMap() const
{
    if (maps.empty()) {
        return nullptr;
    }
    return maps.back();
}

Direction Object::getLastDir() const
{
    const MapCoords prev = prevCoords;
    const int dir_mask = prev.getRelativeDirection(coords, getMap());
    switch (dir_mask) {
    case MASK_DIR_NORTH:
        return DIR_NORTH;
    case MASK_DIR_SOUTH:
        return DIR_SOUTH;
    case MASK_DIR_EAST:
        return DIR_EAST;
    case MASK_DIR_WEST:
        return DIR_WEST;
    default: // no movement or not in cardinal direction
        return DIR_NONE;
    }
}

void Object::remove() const
{
    const unsigned int size = maps.size();
    for (unsigned int i = 0; i < size; i++) {
        if (i == size - 1) {
            maps[i]->removeObject(this);
        } else {
            maps[i]->removeObject(this, false);
        }
    }
}

void Object::animateMovement() const
{
    // TODO abstract movement - also make screen.h and game.h not required
    screenTileUpdate(&game->mapArea, prevCoords);
    if (screenTileUpdate(&game->mapArea, coords, false)) {
        screenWait(1);
    }
}
