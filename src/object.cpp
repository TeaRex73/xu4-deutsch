/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include "object.h"
#include "context.h"
#include "map.h"
#include "player.h"

std::set<Object *> Object::all_objects;

Object::Object(Type type)
    :tile(0),
     prevTile(0),
     coords(),
     prevCoords(),
     movement_behavior(MOVEMENT_FIXED),
     objType(type),
     maps(),
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
    all_objects.insert(this);
    return *this;
}

Object::~Object()
{
    if(c && (c->lastShip == this)) {
        c->lastShip = nullptr;
    }
    all_objects.erase(this);
}

void Object::cleanup()
{
    std::set<Object *>::iterator tmp;

    for (std::set<Object *>::iterator i = all_objects.begin();
         i != all_objects.end();
         /* nothing */ ) {
        tmp = i; /* save iterator so deletion doesn't affect it */
        ++tmp;
        delete (*i);
        i = tmp;
    }
    all_objects.clear();
}

void Object::setCoords(Coords c)
{
    prevCoords = coords;
    coords = c;
}

bool Object::setDirection(Direction d)
{
    return tile.setDirection(d);
}

void Object::setMap(class Map *m)
{
    if (find(maps.begin(), maps.end(), m) == maps.end()) {
        maps.push_back(m);
    }
}

Map *Object::getMap()
{
    if (maps.empty()) {
        return nullptr;
    }
    return maps.back();
}

Direction Object::getLastDir() const
{
    Coords c = getCoords();
    Coords p = getPrevCoords();
    int x = c.x, y = c.y, px = p.x, py = p.y;
    if (x == px) {
        if (y < py) return DIR_NORTH;
        if (y > py) return DIR_SOUTH;
    }
    if (y == py) {
        if (x < px) return DIR_WEST;
        if (x > px) return DIR_EAST;
    }
    return DIR_NONE;
}

void Object::remove()
{
    unsigned int size = maps.size();
    for (unsigned int i = 0; i < size; i++) {
        if (i == size - 1) {
            maps[i]->removeObject(this);
        } else {
            maps[i]->removeObject(this, false);
        }
    }
}

#include "screen.h"
#include "game.h"

void Object::animateMovement()
{
    // TODO abstract movement - also make screen.h and game.h not required
    screenTileUpdate(&game->mapArea, prevCoords);
    if (screenTileUpdate(&game->mapArea, coords, false)) {
        screenWait(1);
    }
}
