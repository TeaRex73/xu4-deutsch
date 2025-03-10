/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <list>
#include <map>
#include <set>

#include "location.h"

#include "annotation.h"
#include "context.h"
#include "combat.h"
#include "creature.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "map.h"
#include "object.h"
#include "savegame.h"
#include "settings.h"
#include "tileset.h"

Location *locationPush(Location *stack, Location *loc);
Location *locationPop(Location **stack);


/**
 * Add a new location to the stack, or
 * start a new stack if 'prev' is nullptr
 */
Location::Location(
    const MapCoords &coords,
    Map *map,
    int viewmode,
    LocationContext ctx,
    TurnCompleter *turnCompleter,
    Location *prev
)
    :coords(coords),
     map(map),
     viewMode(viewmode),
     context(ctx),
     turnCompleter(turnCompleter),
     prev(prev)
{
    if (this->context != CTX_WORLDMAP) {
        this->coords.active_x = 0;
        this->coords.active_y = 0;
    }
}

Location::~Location()
{
    delete prev;
}

/**
 * Return the entire stack of objects at the given location.
 */
std::vector<MapTile> Location::tilesAt(const MapCoords &coords, bool &focus)
{
    std::vector<MapTile> tiles;
    std::list<const Annotation *> a = map->annotations->ptrsToAllAt(coords);
    std::list<const Annotation *>::const_iterator i;
    const Object *obj = map->objectAt(coords);
    const Creature *m = dynamic_cast<const Creature *>(obj);
    focus = false;
    bool avatar = this->coords == coords;
    /* Do not return objects for VIEW_GEM mode,
       show only the avatar and tiles */
    if ((viewMode == VIEW_GEM)
        && (!settings.enhancements
            || !settings.enhancementsOptions.peerShowsObjects)) {
        // When viewing a gem, always show the avatar regardless
        // of whether or not it is shown in our normal view
        if (avatar) {
            tiles.push_back(c->party->getTransport());
        } else {
            tiles.push_back(map->tileAt(coords, WITHOUT_OBJECTS));
        }
        return tiles;
    }
    /* Add the avatar to gem view */
    if (avatar && (viewMode == VIEW_GEM)) {
        tiles.push_back(c->party->getTransport());
    }
    /* Add visual-only annotations to the list */
    for (i = a.cbegin(); i != a.cend(); ++i) {
        if ((*i)->isVisualOnly()) {
            tiles.push_back((*i)->getTile());
            /* If this is the first cover-up annotation,
             * everything underneath it will be invisible,
             * so stop here
             */
            if ((*i)->isCoverUp()) {
                return tiles;
            }
        }
    }
    /* then forces of nature, because they must appear on top of the avatar */
    if (obj && obj->isVisible() && m && m->isForceOfNature()) {
        focus = focus || obj->hasFocus();
        MapTile visibleCreatureAndObjectTile = obj->getTile();
        // Sleeping creatures and persons have their animation frozen
        if (m->isAsleep()) {
            visibleCreatureAndObjectTile.setFreezeAnimation(true);
        }
        tiles.push_back(visibleCreatureAndObjectTile);
    }
    /* then the avatar is drawn */
    if ((map->flags & SHOW_AVATAR)
        && avatar) {
        tiles.push_back(c->party->getTransport());
    }
    /* then camouflaged creatures that have a disguise */
    if (obj
        && (obj->getType() == Object::CREATURE)
        && !obj->isVisible()
        && (!m->getCamouflageTile().empty())) {
        focus = focus || obj->hasFocus();
        tiles.push_back(
            map->tileset->getByName(m->getCamouflageTile())->getId()
        );
    }
    /* then visible creatures and objects except forces of nature */
    else if (obj && obj->isVisible() && (!m || !m->isForceOfNature())) {
        focus = focus || obj->hasFocus();
        MapTile visibleCreatureAndObjectTile = obj->getTile();
        // Sleeping creatures and persons have their animation frozen
        if (m && m->isAsleep()) {
            visibleCreatureAndObjectTile.setFreezeAnimation(true);
        }
        tiles.push_back(visibleCreatureAndObjectTile);
    }
    /* then permanent annotations */
    for (i = a.cbegin(); i != a.cend(); ++i) {
        if (!(*i)->isVisualOnly()) {
            tiles.push_back((*i)->getTile());
            /* If this is the first cover-up annotation,
             * everything underneath it will be invisible,
             * so stop here
             */
            if ((*i)->isCoverUp()) {
                return tiles;
            }
        }
    }
    /* finally the base tile */
    MapTile tileFromMapData = map->getTileFromData(coords);
    const Tile *tileType = tileFromMapData.getTileType();
    if (tileType->isLivingObject()) {
        // This animation should be frozen because a living
        // object represented on the map data is usually a statue
        // of a monster or something
        tileFromMapData.setFreezeAnimation(true);
    }
    tiles.push_back(tileFromMapData);
    /* But if the base tile requires a background, we must find it */
    if (tileType->isLandForeground()
        || tileType->isWaterForeground()
        || tileType->isLivingObject()) {
        tiles.push_back(getReplacementTile(coords, tileType));
    }
    return tiles;
} // Location::tilesAt


/**
 * Finds a valid replacement tile for the given location, using
 * surrounding tiles as guidelines to choose the new tile.  The new
 * tile will only be chosen if it is marked as a valid replacement
 * (or waterReplacement) tile in tiles.xml.  If a valid replacement
 * cannot be found, it returns a "best guess" tile.
 */
TileId Location::getReplacementTile(
    const MapCoords &atCoords, const Tile *forTile
)
{
    std::map<TileId, int> validMapTileCount;
    const static int dirs[][2] = {
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }
    };
    const static int dirs_per_step = sizeof(dirs) / sizeof(*dirs);
    int loop_count = 0;
    std::set<MapCoords> searched;
    std::list<MapCoords> searchQueue;
    // Pathfinding to closest traversable tile with appropriate
    // replacement properties.
    // For tiles marked water-replaceable, pathfinding includes swimmables.
    searchQueue.push_back(atCoords);
    do {
        MapCoords currentStep = searchQueue.front();
        searchQueue.pop_front();
        searched.insert(currentStep);
        for (int i = 0; i < dirs_per_step; i++) {
            MapCoords newStep(currentStep);
            newStep.move(dirs[i][0], dirs[i][1], map);
            Tile const *tileType =
                map->tileTypeAt(newStep, WITHOUT_OBJECTS);
            if (!tileType->isOpaque()) {
                if (searched.find(newStep) == searched.end()) {
                    searchQueue.push_back(newStep);
                }
            }
            if ((tileType->isReplacement()
                 && (forTile->isLandForeground()
                     || forTile->isLivingObject()))
                || (tileType->isWaterReplacement()
                    && forTile->isWaterForeground())) {
                std::map<TileId, int>::iterator validCount =
                    validMapTileCount.find(tileType->getId());
                if (validCount == validMapTileCount.end()) {
                    validMapTileCount[tileType->getId()] = 1;
                } else {
                    validMapTileCount[tileType->getId()]++;
                }
            }
        }
        if (validMapTileCount.size() > 0) {
            std::map<TileId, int>::iterator itr =
                validMapTileCount.begin();
            TileId winner = itr->first;
            int score = itr->second;
            while (++itr != validMapTileCount.end()) {
                if (score < itr->second) {
                    score = itr->second;
                    winner = itr->first;
                }
            }
            return winner;
        }
        /* loop_count is an ugly hack to temporarily fix infinite loop */
    } while (++loop_count < 128
             && searchQueue.size() > 0
             && searchQueue.size() < 64);
    /* couldn't find a tile, give it the classic default */
    if (isDungeon(map)) {
        return map->tileset->getByName("brick_floor")->getId();
    } else {
        return map->tileset->getByName("grass")->getId();
    }
} // Location::getReplacementTile


/**
 * Returns the current coordinates of the location given:
 *     If in combat - returns the coordinates of party member with focus
 *     If elsewhere - returns the coordinates of the avatar
 */
void Location::getCurrentPosition(MapCoords *coords)
{
    if (context & CTX_COMBAT) {
        CombatController *cc =
            dynamic_cast<CombatController *>(eventHandler->getController());
        PartyMemberVector *party = cc->getParty();
        *coords = (*party)[cc->getFocus()]->getCoords();
    } else {
        *coords = this->coords;
    }
}

MoveResult Location::move(Direction dir, bool userEvent)
{
    MoveEvent event(dir, userEvent);
    switch (map->type) {
    case Map::DUNGEON:
        moveAvatarInDungeon(event);
        break;
    case Map::COMBAT:
        movePartyMember(event);
        break;
    default:
        moveAvatar(event);
        break;
    }
    setChanged();
    notifyObservers(event);
    return event.result;
}


/**
 * Pop a location from the stack and free the memory
 */
void locationFree(Location **stack)
{
    delete locationPop(stack);
}


/**
 * Push a location onto the stack
 */
Location *locationPush(Location *stack, Location *loc)
{
    loc->prev = stack;
    return loc;
}


/**
 * Pop a location off the stack
 */
Location *locationPop(Location **stack)
{
    Location *loc = *stack;
    *stack = (*stack)->prev;
    loc->prev = nullptr;
    return loc;
}
