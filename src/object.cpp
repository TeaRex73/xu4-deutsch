/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <set>

#include "object.h"

#include "context.h"
#include "direction.h"
#include "game.h"
#include "map.h"
#include "screen.h"


std::set<Object *> Object::all_objects;

Object::Object(const Type type)
    :tile(0),
     prevTile(0),
     movement_behavior(MOVEMENT_FIXED),
     objType(type),
     focused(false),
     visible(true),
     animated(true)
{
    all_objects.insert(this);
}

Object::Object(const Object &o)
    :tile(o.tile),
     prevTile(o.prevTile),
     coords(o.coords),
     prevCoords(o.prevCoords),
     movement_behavior(o.movement_behavior),
     objType(o.objType),
     maps(o.maps),
     focused(o.focused),
     visible(o.visible),
     animated(o.animated)
{
    all_objects.insert(this);
}

Object &Object::operator=(const Object &o)
{
    if (&o != this) {
        tile = o.tile;
        prevTile = o.prevTile;
        coords = o.coords;
        prevCoords = o.prevCoords;
        movement_behavior = o.movement_behavior;
        objType = o.objType;
        maps = o.maps;
        focused = o.focused;
        visible = o.visible;
        animated = o.animated;
    }
    return *this;
}

Object::~Object()
{
    if(c && c->lastShip == this) {
        c->lastShip = nullptr;
    }
    all_objects.erase(this);
}

void Object::cleanup()
{
    for (auto i = all_objects.begin();
         i != all_objects.end();
         /* nothing */ ) {
        auto tmp = i; /* save iterator so deletion doesn't affect it */
        ++tmp;
        delete *i;
        i = tmp;
    }
    all_objects.clear();
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
