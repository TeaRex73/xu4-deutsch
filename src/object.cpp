/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include "object.h"
#include "map.h"

std::unordered_set<Object *> Object::all_objects;

Object::Object(Type type)
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

Object::~Object()
{
    all_objects.erase(this);
}

void Object::cleanup()
{
    std::unordered_set<Object *>::iterator tmp;

    for (std::unordered_set<Object *>::iterator i = all_objects.begin();
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
    Map *map = getMap();
    std::pair<ObjectLocMap::iterator, ObjectLocMap::iterator> p =
        map->objectsByLocation.equal_range(prevCoords);
    for (ObjectLocMap::iterator i = p.first; i != p.second; /* nothing */) {
        if (i->second == this) {
            i = map->objectsByLocation.erase(i);
        } else {
            i++;
        }
    }
    map->objectsByLocation.insert(std::pair<Coords, Object *>(coords, this));
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
    screenTileUpdate(&game->mapArea, prevCoords, false);
    if (screenTileUpdate(&game->mapArea, coords, false)) {
        screenWait(1);
    }
}
