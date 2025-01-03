/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>

#include "u4.h"

#include "map.h"

#include "annotation.h"
#include "context.h"
#include "debug.h"
#include "direction.h"
#include "location.h"
#include "movement.h"
#include "object.h"
#include "person.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "tileset.h"
#include "tilemap.h"
#include "types.h"
#include "utils.h"
#include "settings.h"

/**
 * MapCoords Class Implementation
 */
MapCoords MapCoords::nowhere(-1, -1, -1);

MapCoords &MapCoords::wrap(const Map *map)
{
    if (map && (map->border_behavior == Map::BORDER_WRAP)) {
        while (x < 0) {
            x += map->width;
        }
        while (y < 0) {
            y += map->height;
        }
        while (x >= static_cast<int>(map->width)) {
            x -= map->width;
        }
        while (y >= static_cast<int>(map->height)) {
            y -= map->height;
        }
    }
    return *this;
}

MapCoords &MapCoords::putInBounds(const Map *map)
{
    if (map) {
        if (x < 0) {
            x = 0;
        }
        if (x >= static_cast<int>(map->width)) {
            x = map->width - 1;
        }
        if (y < 0) {
            y = 0;
        }
        if (y >= static_cast<int>(map->height)) {
            y = map->height - 1;
        }
        if (z < 0) {
            z = 0;
        }
        if (z >= static_cast<int>(map->levels)) {
            z = map->levels - 1;
        }
    }
    return *this;
} // MapCoords::putInBounds

MapCoords &MapCoords::move(Direction d, const Map *map)
{
    switch (d) {
    case DIR_NORTH:
        y--;
        break;
    case DIR_EAST:
        x++;
        break;
    case DIR_SOUTH:
        y++;
        break;
    case DIR_WEST:
        x--;
        break;
    default:
        break;
    }
    // Wrap the coordinates if necessary
    wrap(map);
    return *this;
}

MapCoords &MapCoords::move(int dx, int dy, const Map *map)
{
    x += dx;
    y += dy;
    // Wrap the coordinates if necessary
    wrap(map);
    return *this;
}


/**
 * Returns a mask of directions that indicate where one point is relative
 * to another.  For instance, if the object at (x, y) is
 * northeast of (c.x, c.y), then this function returns
 * (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST))
 * This function also takes into account map boundaries and adjusts
 * itself accordingly. If the two coordinates are not on the same z-plane,
 * then this function return DIR_NONE.
 */
int MapCoords::getRelativeDirection(const MapCoords &c, const Map *map) const
{
    int dx, dy, dirmask;
    dirmask = DIR_NONE;
    if (z != c.z) {
        return dirmask;
    }
    /* adjust our coordinates to find the closest path */
    if (map && (map->border_behavior == Map::BORDER_WRAP)) {
        MapCoords me = *this;
        if (std::abs(me.x - c.x)
            > std::abs(static_cast<int>(me.x + map->width - c.x))) {
            me.x += map->width;
        } else if (std::abs(me.x - c.x)
                   > std::abs(static_cast<int>(me.x - map->width - c.x))) {
            me.x -= map->width;
        }
        if (std::abs(me.y - c.y)
            > std::abs(static_cast<int>(me.y + map->width - c.y))) {
            me.y += map->height;
        } else if (std::abs(me.y - c.y)
                   > std::abs(static_cast<int>(me.y - map->width - c.y))) {
            me.y -= map->height;
        }
        dx = me.x - c.x;
        dy = me.y - c.y;
    } else {
        dx = x - c.x;
        dy = y - c.y;
    }
    /* add x directions that lead towards to_x to the mask */
    if (dx < 0) {
        dirmask |= MASK_DIR(DIR_EAST);
    } else if (dx > 0) {
        dirmask |= MASK_DIR(DIR_WEST);
    }
    /* add y directions that lead towards to_y to the mask */
    if (dy < 0) {
        dirmask |= MASK_DIR(DIR_SOUTH);
    } else if (dy > 0) {
        dirmask |= MASK_DIR(DIR_NORTH);
    }
    /* return the result */
    return dirmask;
} // MapCoords::getRelativeDirection


/**
 * Finds the appropriate direction to travel to get from one point to
 * another.  This algorithm will avoid getting trapped behind simple
 * obstacles, but still fails with anything mildly complicated.
 * This function also takes into account map boundaries and adjusts
 * itself accordingly, provided the 'map' parameter is passed
 */
Direction MapCoords::pathTo(
    const MapCoords &c,
    int valid_directions,
    bool towards,
    const Map *map,
    Direction last
) const
{
    int directionsToObject;
    /* find the directions that lead [to/away from] our target */
    directionsToObject =
        towards ? getRelativeDirection(c, map) : ~getRelativeDirection(c, map);
    /* make sure we eliminate impossible options */
    directionsToObject &= valid_directions;
    /* get the new direction to move */
    if (directionsToObject > DIR_NONE) {
        return dirRandomDir(directionsToObject, last);
    }
    /* there are no valid directions that lead to our target, just move
       wherever we can! */
    else {
        return dirRandomDir(valid_directions, last);
    }
}


/**
 * Finds the appropriate direction to travel to move away from one point
 */
Direction MapCoords::pathAway(
    const MapCoords &c, int valid_directions, const Map *map, Direction last
) const
{
    return pathTo(c, valid_directions, false, map, last);
}


/**
 * Finds the movement distance (not using diagonals) from point a to point b
 * on a map, taking into account map boundaries and such.  If the two coords
 * are not on the same z-plane, the function returns -1.
 */
int MapCoords::movementDistance(const MapCoords &c, const Map *map) const
{
    int dirmask = DIR_NONE;
    int dist = 0;
    MapCoords me = *this;
    if (z != c.z) return -1;
    /* get the direction(s) to the coordinates */
    dirmask = getRelativeDirection(c, map);
    while ((me.x != c.x) || (me.y != c.y)) {
        if (me.x != c.x) {
            if (dirmask & MASK_DIR_WEST) {
                me.move(DIR_WEST, map);
            } else {
                me.move(DIR_EAST, map);
            }
            dist++;
        }
        if (me.y != c.y) {
            if (dirmask & MASK_DIR_NORTH) {
                me.move(DIR_NORTH, map);
            } else {
                me.move(DIR_SOUTH, map);
            }
            dist++;
        }
    }
    return dist;
} // MapCoords::movementDistance


/**
 * Finds the distance (using diagonals) from point a to point b on a map
 * If the two coordinates are not on the same z-plane, then this function
 * adds 256 per level. This function also takes into account map boundaries.
 */
int MapCoords::distance(const MapCoords &c, const Map *map) const
{
    int dist = movementDistance(c, map);
    if (dist < 0) return dist;
    /* calculate how many fewer movements there would have been */
    dist -=
        (std::abs(x - c.x) < std::abs(y - c.y)) ?
        (std::abs(x - c.x)) :
        (std::abs(y - c.y));
    return dist;
}


/**
 * Map Class Implementation
 */
Map::Map()
    :id(0),
     fname(),
     type(WORLD),
     width(0),
     height(0),
     levels(1),
     chunk_width(0),
     chunk_height(0),
     offset(0),
     baseSource(),
     compressed_chunks(),
     border_behavior(BORDER_WRAP),
     portals(),
     annotations(new AnnotationMgr()),
     flags(0),
     music(Music::Type::NONE),
     data(),
     objects(),
     labels(),
     tileset(nullptr),
     tilemap(nullptr),
     monsterTable()
{
}

Map::~Map()
{
    for (PortalList::const_iterator i = portals.cbegin();
         i != portals.cend();
         ++i) {
        delete *i;
    }
    delete annotations;
}

std::string Map::getName()
{
    return baseSource.fname;
}


/**
 * Returns the object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns nullptr.
 */
Object *Map::objectAt(const Coords &coords) const
{
    /* FIXME: return a list instead of one object */
    ObjectDeque::const_iterator i;
    Object *objAt = nullptr;
    for (i = objects.cbegin(); i != objects.cend(); ++i) {
        Object *obj = *i;
        if (__builtin_expect(obj->getCoords() == coords, false)) {
            /* get the most visible object */
            if (!objAt) {
                objAt = obj;
            }
            else if (objAt->getType() < obj->getType()) {
                objAt = obj;
            }
            /* give priority to objects that have the focus */
            else if ((!objAt->hasFocus()) && (obj->hasFocus())) {
                objAt = obj;
            }
        }
    }
    return objAt;
} // Map::objectAt


/**
 * Returns the portal for the correspoding action(s) given.
 * If there is no portal that corresponds to the actions flagged
 * by 'actionFlags' at the given (x,y,z) coords, it returns nullptr.
 */
const Portal *Map::portalAt(const Coords &coords, int actionFlags) const
{
    PortalList::const_iterator i = std::find_if(
        portals.cbegin(),
        portals.cend(),
        [&](const Portal *v) -> bool {
            return (v->coords == coords) && (v->trigger_action & actionFlags);
        }
    );
    if (i != portals.cend()) {
        return *i;
    } else {
        return nullptr;
    }
}


/**
 * Returns the raw tile for the given (x,y,z) coords for the given map
 */
MapTile Map::getTileFromData(const Coords &coords) const
{
    static MapTile blank = 0;
    if (MAP_IS_OOB(this, coords)) {
        return blank;
    }
    int index = coords.x + (coords.y * width) + (width * height * coords.z);
    return data[index];
}


/**
 * Returns the current ground tile at the given point on a map.  Visual-only
 * annotations like moongates and attack icons are ignored.  Any walkable tiles
 * are taken into account (treasure chests, ships, balloon, etc.)
 */
MapTile Map::tileAt(const Coords &coords, int withObjects) const
{
    /* FIXME: this should return a list of tiles, with the most visible
       at the front */
    std::list<const Annotation *> a = annotations->ptrsToAllAt(coords);
    const Object *obj = objectAt(coords);
    MapTile tile = getTileFromData(coords);
    /* FIXME: this only returns the first valid annotation it can find */
    std::list<const Annotation *>::const_iterator i = std::find_if(
        a.cbegin(),
        a.cend(),
        [&](const Annotation *v) -> bool {
            return !(v->isVisualOnly());
        }
    );
    if (i != a.cend()) {
        return (*i)->getTile();
    }
    if ((withObjects == WITH_OBJECTS) && obj) {
        tile = obj->getTile();
    } else if ((withObjects == WITH_GROUND_OBJECTS)
               && obj
               && obj->getTile().getTileType()->isWalkable()) {
        tile = obj->getTile();
    }
    return tile;
} // Map::tileAt

const Tile *Map::tileTypeAt(const Coords &coords, int withObjects) const
{
    return tileAt(coords, withObjects).getTileType();
}


/**
 * Returns true if the given map is the world map
 */
bool Map::isWorldMap() const
{
    return type == WORLD;
}


/**
 * Returns true if the map is enclosed (to see if gem layouts should
 * cut themselves off)
 */
bool Map::isEnclosed(const Coords &party) const
{
    unsigned int x, y;
    int *path_data;
    if (border_behavior != BORDER_WRAP) {
        return true;
    }
    path_data = new int[width * height];
    for (unsigned int i = 0; i < width * height; i++) {
        path_data[i] = -1;
    }
    // Determine what's walkable (1), and what's border-walkable (2)
    findWalkability(party, path_data);
    // Find two connecting pathways where the avatar can reach both
    // without wrapping
    for (x = 0; x < width; x++) {
        int index = x;
        if ((path_data[index] == 2)
            && (path_data[index + ((height - 1) * width)] == 2)) {
            delete[] path_data;
            return false;
        }
    }
    for (y = 0; y < width; y++) {
        int index = (y * width);
        if ((path_data[index] == 2) && (path_data[index + width - 1] == 2)) {
            delete[] path_data;
            return false;
        }
    }
    delete[] path_data;
    return true;
} // Map::isEnclosed

void Map::findWalkability(const Coords &coords, int *path_data) const
{
    const Tile *mt = tileTypeAt(coords, WITHOUT_OBJECTS);
    int index = coords.x + (coords.y * width);
    if (mt->isWalkable()) {
        bool isBorderTile = (coords.x == 0)
            || (coords.x == static_cast<int>(width - 1))
            || (coords.y == 0)
            || (coords.y == static_cast<int>(height - 1));
        path_data[index] = isBorderTile ? 2 : 1;
        if ((coords.x > 0)
            && (path_data[coords.x - 1 + (coords.y * width)] < 0)) {
            findWalkability(
                Coords(coords.x - 1, coords.y, coords.z), path_data
            );
        }
        if ((coords.x < static_cast<int>(width - 1))
            && (path_data[coords.x + 1 + (coords.y * width)] < 0)) {
            findWalkability(
                Coords(coords.x + 1, coords.y, coords.z), path_data
            );
        }
        if ((coords.y > 0)
            && (path_data[coords.x + ((coords.y - 1) * width)] < 0)) {
            findWalkability(
                Coords(coords.x, coords.y - 1, coords.z), path_data
            );
        }
        if ((coords.y < static_cast<int>(height - 1))
            && (path_data[coords.x + ((coords.y + 1) * width)] < 0)) {
            findWalkability(
                Coords(coords.x, coords.y + 1, coords.z), path_data
            );
        }
    } else {
        path_data[index] = 0;
    }
} // Map::findWalkability


/**
 * Adds a creature object to the given map
 */
Creature *Map::addCreature(const Creature *creature, const Coords &coords)
{
    /* make a copy of the creature before placing it */
    Creature *m = new Creature(*creature);

    m->setInitialHp();
    m->setStatus(STAT_GOOD);
    m->setMap(this);
    m->setCoords(coords);
    m->setPrevCoords(coords);
    /* initialize the creature before placing it */
    if (m->isStationary()) {
        m->setMovementBehavior(MOVEMENT_FIXED);
    } else if (m->wanders() || type == Map::DUNGEON) {
        // U4DOS: All Dungeon creatures wander, except stationary ones
        m->setMovementBehavior(MOVEMENT_WANDER);
    } else {
        m->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
    }
    /* hide camouflaged creatures from view during combat */
    if (m->camouflages() && (type == COMBAT)) {
        m->setVisible(false);
    }
    /* place the creature on the map */
    objects.push_back(m);
    // m->setCoords(coords);
    // m->setPrevCoords(coords);
    return m;
} // Map::addCreature


/**
 * Adds an object to the given map
 */
Object *Map::addObject(Object *obj, const Coords &)
{
    objects.push_front(obj);
    obj->setCoords(obj->getCoords());
    obj->setPrevCoords(obj->getCoords());
    return obj;
}

Object *Map::addObject(
    MapTile tile, MapTile prevtile, const Coords &coords
)
{
    Object *obj = new Object;

    obj->setTile(tile);
    obj->setPrevTile(prevtile);
    obj->setMap(this);
    obj->setCoords(coords);
    obj->setPrevCoords(coords);
    objects.push_front(obj);
    // obj->setCoords(coords);
    // obj->setPrevCoords(coords);
    return obj;
}


/**
 * Removes an object from the map
 */
// This function should only be used when not iterating through an
// ObjectDeque, as the iterator will be invalidated and the
// results will be unpredictable.  Instead, use the function
// below.
void Map::removeObject(const Object *rem, bool deleteObject)
{
    ObjectDeque::const_iterator i = std::find(
        objects.cbegin(),
        objects.cend(),
        rem
    );
    if (i == objects.cend()) return;
    /* Party members persist through different maps,
       so don't delete them! */
    if (deleteObject && !isPartyMember(*i)) {
        delete *i;
    }
    objects.erase(i);
}

ObjectDeque::iterator Map::removeObject(
    ObjectDeque::iterator rem, bool deleteObject
)
{
    /* Party members persist through different maps, so don't delete them! */
    if (deleteObject && !isPartyMember(*rem)) {
        delete *rem;
    }
    return objects.erase(rem);
}


/**
 * Moves all of the objects on the given map.
 * Returns an attacking object if there is a creature attacking.
 * Also performs special creature actions and creature effects.
 */
Creature *Map::moveObjects(const MapCoords &avatar) const
{
    ObjectDeque::const_iterator i;
    Creature *attacker = nullptr;
    for (i = objects.cbegin(); i != objects.cend(); ++i) {
        Creature *m = dynamic_cast<Creature *>(*i);
        if (m) {
            /* check if the object is an attacking creature and not
               just a normal, docile person in town or an inanimate object */
            if (((m->getType() == Object::PERSON)
                 && (m->getMovementBehavior() == MOVEMENT_ATTACK_AVATAR))
                || ((m->getType() == Object::CREATURE) && m->willAttack())) {
                MapCoords o_coords = m->getCoords();
                /* don't move objects that aren't on the same level as us */
                if (o_coords.z != avatar.z) {
                    continue;
                }
                if (o_coords.movementDistance(avatar, this) <= 1) {
                    attacker = m;
                    continue;
                }
            }
            /* Before moving, Enact any special effects of the creature
               (such as storms eating objects, whirlpools teleporting,
               etc.) */
            m->specialEffect();
            /* Perform any special actions (such as pirate ships firing
               cannons, sea serpents' fireblast attack, etc.) */
            if (!m->specialAction()) {
                if (moveObject(this, m, avatar)) {
                    m->animateMovement();
                    /* After moving, Enact any special effects of the
                       creature (such as storms eating objects, whirlpools
                       teleporting, etc.) */
                    m->specialEffect();
                }
            }
        }
    }
    return attacker;
} // Map::moveObjects


/**
 * Resets object animations to a value that is acceptable for
 * savegame compatibility with u4dos.
 */
void Map::resetObjectAnimations() const
{
    ObjectDeque::const_iterator i;
    for (i = objects.cbegin(); i != objects.cend(); ++i) {
        Object *obj = *i;
        if (obj->getType() == Object::CREATURE) {
            obj->setPrevTile(
                creatureMgr->getByTile(obj->getTile())->getTile()
            );
        }
    }
}

/**
 * Removes all objects from the given map, deleting them
 */
void Map::clearObjects()
{
    objects.clear();
}


/**
 * Returns the number of creatures on the given map level,
   or all levels if level == -1
 */
int Map::getNumberOfCreatures(int level) const
{
    ObjectDeque::const_iterator i;
    int n = 0;
    for (i = objects.cbegin(); i != objects.cend(); ++i) {
        const Object *obj = *i;
        if (obj->getType() == Object::CREATURE) {
            if (level == -1 || obj->getCoords().z == level) {
                n++;
            }
        }
    }
    return n;
}


/**
 * Returns a mask of valid moves for the given transport on the given map
 */
int Map::getValidMoves(
    const MapCoords &from, MapTile transport, bool wanders
) const
{
    int retval;
    Direction d;
    const Object *obj;
    const Creature *m, *to_m;
    // get the creature object, if it exists (the one that's moving)
    m = creatureMgr->getByTile(transport);
    bool isAvatar = (c->location->coords == from);
    if (m && m->canMoveOntoPlayer()) {
        isAvatar = false;
    }
    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d = static_cast<Direction>(d + 1)) {
        MapCoords coords = from;
        bool ontoAvatar = false;
        bool ontoCreature = false;
        // Move the coordinates in the current direction and test it
        coords.move(d, this);
        // you can always walk off the edge of the map
        if (MAP_IS_OOB(this, coords)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }
        obj = objectAt(coords);
        // see if it's trying to move onto the avatar
        if ((flags & SHOW_AVATAR) && (coords == c->location->coords)) {
            ontoAvatar = true;
        }
        // see if it's trying to move onto a person or creature
        else if (obj && (obj->getType() != Object::UNKNOWN)) {
            ontoCreature = true;
        }
        // get the destination tile
        MapTile tile;
        if (ontoAvatar) {
            tile = c->party->getTransport();
        } else if (ontoCreature) {
            tile = obj->getTile();
        } else {
            tile = tileAt(coords, WITH_OBJECTS);
        }
        MapTile prev_tile = tileAt(from, WITHOUT_OBJECTS);
        // get the other creature object, if it exists (the one that's
        // being moved onto)
        to_m = dynamic_cast<const Creature *>(obj);
        // move on if unable to move onto the avatar or another creature
        // some creatures/persons have the same
        // tile as the avatar, so we have to adjust
        if (m && !isAvatar) {
            // If moving onto the avatar, the creature must be able to
            // move onto the player. If moving onto another creature, it must
            // be able to move onto other creatures, and the creature must be
            // able to have others move onto it.  If either of these
            // conditions are not met, the creature cannot move onto another.
            if ((ontoAvatar && m->canMoveOntoPlayer())
                || (ontoCreature && m->canMoveOntoCreatures())) {
                // Ignore all objects, and just consider terrain
                tile = tileAt(coords, WITHOUT_OBJECTS);
            }
            if ((ontoAvatar && !m->canMoveOntoPlayer())
                || (ontoCreature
                    && ((!m->canMoveOntoCreatures()
                         && !to_m->canMoveOntoCreatures())
                        || (m->isForceOfNature()
                            && to_m->isForceOfNature())))) {
                continue;
            }
        }
        // avatar movement
        if (isAvatar) {
            // if the transport is a ship, check sailable
            if (transport.getTileType()->isShip()
                && tile.getTileType()->isSailable()) {
                retval = DIR_ADD_TO_MASK(d, retval);
            }
            // if it is a balloon, check flyable
            else if (transport.getTileType()->isBalloon()
                     && tile.getTileType()->isFlyable()) {
                retval = DIR_ADD_TO_MASK(d, retval);
            }
            // avatar or horseback: check walkable
            else if ((transport == tileset->getByName("avatar")->getId())
                     || transport.getTileType()->isHorse()) {
                if (tile.getTileType()->canWalkOn(d)
                    && (!transport.getTileType()->isHorse()
                        || tile.getTileType()->isCreatureWalkable())
                    && prev_tile.getTileType()->canWalkOff(d)) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                }
            }
        }
        // creature movement
        else if (m) {
            // flying creatures
            if (tile.getTileType()->isFlyable() && m->flies()) {
                // FIXME: flying creatures behave differently on the world map?
                if (isWorldMap()) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                } else if (tile.getTileType()->isWalkable()
                           || tile.getTileType()->isSwimable()
                           || tile.getTileType()->isSailable()) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                }
            }
            // swimming creatures and sailing creatures
            else if (tile.getTileType()->isSwimable()
                     || tile.getTileType()->isSailable()
                     || tile.getTileType()->isShip()) {
                if (m->swims() && tile.getTileType()->isSwimable()) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                }
                if (m->sails() && tile.getTileType()->isSailable()) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                }
                if (m->canMoveOntoPlayer() && tile.getTileType()->isShip()) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                }
            }
            // ghosts and other incorporeal creatures
            // u4apple2: they can't go through dungeon walls
            else if (m->isIncorporeal() && type != Map::DUNGEON) {
                // can move anywhere but onto water, unless of course the
                // creature can swim (but that has been dealt with above)
                retval = DIR_ADD_TO_MASK(d, retval);
            }
            // walking creatures
            else if (m->walks()) {
                if (tile.getTileType()->canWalkOn(d)
                    && prev_tile.getTileType()->canWalkOff(d)
                    && tile.getTileType()->isCreatureWalkable()
                    && (!wanders || tile.getTileType()->willWanderOn())) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                }
            }
            // Creatures that can move onto player
            else if (ontoAvatar && m->canMoveOntoPlayer()) {
                // tile should be transport
                if (m->flies() || tile.getTileType()->isShip()) {
                    retval = DIR_ADD_TO_MASK(d, retval);
                }
            }
        }
    }
    return retval;
} // Map::getValidMoves

bool Map::move(Object *obj, Direction d)
{
    MapCoords new_coords = obj->getCoords();
    if (new_coords.move(d) != obj->getCoords()) {
        obj->setCoords(new_coords);
        return true;
    }
    return false;
}


/**
 * Alerts the guards that the avatar is doing something bad
 */
void Map::alertGuards() const
{
    ObjectDeque::const_iterator i;
    /* switch all the guards to attack mode */
    for (i = objects.cbegin(); i != objects.cend(); ++i) {
        const Creature *m = creatureMgr->getByTile((*i)->getTile());
        if (m && ((m->getId() == GUARD_ID)
                  || (m->getId() == LORDBRITISH_ID))) {
            (*i)->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
        }
    }
}

const MapCoords &Map::getLabel(const std::string &name) const
{
    std::map<std::string, MapCoords>::const_iterator i =
        labels.find(name);
    if (i == labels.cend()) {
        return MapCoords::nowhere;
    }
    return i->second;
}

// helper function for Map::fillMonsterTable()
static bool isCloser(const Object *a, const Object *b)
{
    const MapCoords &ma = a->getCoords();
    const MapCoords &mb = b->getCoords();
    const Coords &coords = c->location->coords;
    if ((ma.z != coords.z) || (mb.z != coords.z)) {
        return std::abs(ma.z - coords.z) < std::abs(mb.z - coords.z);
    }
    return ma.distance(coords) < mb.distance(coords);
}

bool Map::fillMonsterTable()
{
    ObjectDeque::const_iterator current;
    ObjectDeque monsters;
    ObjectDeque other_creatures;
    ObjectDeque inanimate_objects;
    Object empty;
    int nForcesOfNature = 0;
    int nCreatures = 0;
    int nObjects = 0;
    int i;
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        monsterTable[i] = {};
    }
    /**
     * First, categorize all the objects we have
     */
    /* CHANGE: lastShip, if it exists, is always the first
       object in the inanimate objects section of the table */
    if (c->lastShip != nullptr) {
        if (c->lastShip->getMap() == this) {
            nObjects++;
            inanimate_objects.push_back(c->lastShip);
        }
    }
    for (current = objects.cbegin(); current != objects.cend(); ++current) {
        Object *obj = *current;
        if (obj == c->lastShip && obj->getMap() == this) {
            continue;
        }
        /* moving objects first */
        if ((obj->getType() == Object::CREATURE)
            /* && (obj->getMovementBehavior() != MOVEMENT_FIXED) */ ) {
            const Creature *m = dynamic_cast<const Creature *>(obj);
            /* whirlpools and storms are separated from other moving objects */
            if ((m->getId() == WHIRLPOOL_ID) || (m->getId() == STORM_ID)) {
                nForcesOfNature++;
                monsters.push_back(obj);
            } else {
                nCreatures++;
                other_creatures.push_back(obj);
            }
        } else {
            nObjects++;
            inanimate_objects.push_back(obj);
        }
    }
    /* limit forces of nature */
    /* sort first so that, if any, those furthest away get thrown out */
    std::sort(monsters.begin(), monsters.end(), &isCloser);
    while(nForcesOfNature > MONSTERTABLE_FORCESOFNATURE_SIZE) {
        monsters.pop_back();
        nForcesOfNature--;
    }
    nCreatures += nForcesOfNature;
    /**
     * Add other monsters to our whirlpools and storms
     */
    /* sort first, see above */
    std::sort(other_creatures.begin(), other_creatures.end(), &isCloser);
    while (other_creatures.size()) {
        monsters.push_back(other_creatures.front());
        other_creatures.pop_front();
    }
    /* limit monsters */
    int limitForCreatures =
        type == Map::DUNGEON ?
        MONSTERTABLE_SIZE :
        MONSTERTABLE_CREATURES_SIZE;

    while (nCreatures > limitForCreatures) {
        monsters.pop_back();
        nCreatures--;
    }
    /**
     * Add empty objects to our list to fill things up
     */
    while (monsters.size() < static_cast<unsigned int>(limitForCreatures)) {
        monsters.push_back(&empty);
    }
    /**
     * Finally, add inanimate objects (not in dungeon - there can't be any)
     */
    if (type != Map::DUNGEON) {
        /* sort first, see above, but leave lastShip as first one
           if it exists */
        if (
            inanimate_objects.size()
            && inanimate_objects[0] == c->lastShip
            && inanimate_objects[0]->getMap() == this
        ) {
            std::sort(
                std::next(inanimate_objects.begin()),
                inanimate_objects.end(),
                &isCloser
            );
        } else {
            std::sort(
                inanimate_objects.begin(), inanimate_objects.end(), &isCloser
            );
        }
        while (inanimate_objects.size()) {
            monsters.push_back(inanimate_objects.front());
            inanimate_objects.pop_front();
        }
        /* limit objects */
        while (nObjects > MONSTERTABLE_OBJECTS_SIZE) {
            monsters.pop_back();
            nObjects--;
        }
    } // if (type != Map::DUNGEON)
    /**
     * Fill in the blanks
     */
    while (monsters.size() < MONSTERTABLE_SIZE) {
        monsters.push_back(&empty);
    }
    /**
     * Fill in our monster table
     */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        const Coords &co = monsters[i]->getCoords(),
            &prevco = monsters[i]->getPrevCoords();
        monsterTable[i].tile =
            TileMap::get("base")->untranslate(monsters[i]->getTile());
        monsterTable[i].x = co.x;
        monsterTable[i].y = co.y;
        monsterTable[i].prevTile =
            TileMap::get("base")->untranslate(monsters[i]->getPrevTile());
        monsterTable[i].prevx = prevco.x;
        monsterTable[i].prevy = prevco.y;
        monsterTable[i].z = (type == Map::DUNGEON) ? co.z : 0;
        monsterTable[i].unused = 0;
    }
    return true;
} // Map::fillMonsterTable

MapTile Map::tfrti(int raw) const
{
    U4ASSERT(tilemap != nullptr, "tilemap hasn't been set");
    return tilemap->translate(raw);
}

unsigned int Map::ttrti(MapTile tile) const
{
    U4ASSERT(tilemap != nullptr, "tilemap hasn't been set");
    return tilemap->untranslate(tile);
}
