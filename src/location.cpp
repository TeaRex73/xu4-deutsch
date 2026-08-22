/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "location.h"

#include "annotation.h"
#include "combat.h"
#include "context.h"
#include "creature.h"
#include "direction.h"
#include "event.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "object.h"
#include "player.h"
#include "settings.h"
#include "tile.h"
#include "tileset.h"
#include "types.h"


/* FIXME: locationPush is never used, and locationPop only
   in locationFree. Are they good for anything? */
#if 0
static Location *locationPush(Location *stack, Location *loc);
#endif
static Location *locationPop(Location **stack);

/**
 * Add a new location to the stack, or
 * start a new stack if 'prev' is nullptr
 */
Location::Location(
    const MapCoords &coords,
    Map *map,
    const int viewMode,
    const LocationContext ctx,
    TurnCompleter *turnCompleter,
    Location *prev
)
    :coords(coords),
     map(map),
     viewMode(viewMode),
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
std::vector<MapTile> Location::tilesAt(
    const MapCoords &objectCoords, bool &focus
) const
{
    std::vector<MapTile> tiles;
    const std::list<const Annotation *> a =
        map->annotations->ptrsToAllAt(objectCoords);
    std::list<const Annotation *>::const_iterator i;
    const Object *obj = map->objectAt(objectCoords);
    const auto *m = dynamic_cast<const Creature *>(obj);
    focus = false;
    const bool avatar = this->coords == objectCoords;
    /* Do not return objects for VIEW_GEM mode,
       show only the avatar and tiles */
    if (viewMode == VIEW_GEM
        && !settings.enhancementsOptions.peerShowsObjects) {
        // When viewing a gem, always show the avatar regardless
        // of whether or not it is shown in our normal view
        if (avatar) {
            tiles.push_back(c->party->getTransport());
        } else {
            tiles.push_back(map->tileAt(objectCoords, WITHOUT_OBJECTS));
        }
        return tiles;
    }
    /* Add the avatar to gem view */
    if (avatar && viewMode == VIEW_GEM) {
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
    if (map->flags & SHOW_AVATAR
        && avatar) {
        tiles.push_back(c->party->getTransport());
    }
    /* then camouflaged creatures that have a disguise */
    if (obj
        && obj->getType() == Object::CREATURE
        && !obj->isVisible()
        && !m->getCamouflageTile().empty()) {
        focus = focus || obj->hasFocus();
        tiles.emplace_back(
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
    MapTile tileFromMapData = map->getTileFromData(objectCoords);
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
        tiles.emplace_back(getReplacementTile(objectCoords, tileType));
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
) const
{
    std::map<TileId, int> validMapTileCount;
    static constexpr int dirs[][2] = {
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }
    };
    int loop_count = 0;
    std::set<MapCoords> searched;
    std::list<MapCoords> searchQueue;
    // Pathfinding to closest traversable tile with appropriate
    // replacement properties.
    // For tiles marked water-replaceable, pathfinding includes
    // swimmable tiles.
    searchQueue.push_back(atCoords);
    do {
        MapCoords currentStep = searchQueue.front();
        searchQueue.pop_front();
        searched.insert(currentStep);
        for (const auto *dir: dirs) {
            MapCoords newStep(currentStep);
            newStep.move(dir[0], dir[1], map);
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
                auto validCount = validMapTileCount.find(tileType->getId());
                if (validCount == validMapTileCount.end()) {
                    validMapTileCount[tileType->getId()] = 1;
                } else {
                    validMapTileCount[tileType->getId()]++;
                }
            }
        }
        if (!validMapTileCount.empty()) {
            auto itr = validMapTileCount.begin();
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
             && !searchQueue.empty()
             && searchQueue.size() < 64);
    /* couldn't find a tile, give it the classic default */
    if (map->isDungeonMap()) {
        return map->tileset->getByName("brick_floor")->getId();
    }
    return map->tileset->getByName("grass")->getId();
} // Location::getReplacementTile


/**
 * Returns the current coordinates of the location given:
 *     If in combat - returns the coordinates of party member with focus
 *     If elsewhere - returns the coordinates of the avatar
 */
MapCoords Location::getCurrentPosition() const
{
    if (context & CTX_COMBAT) {
        auto *cc =
            dynamic_cast<CombatController *>(eventHandler->getController());
        const PartyMemberVector *party = cc->getParty();
        return (*party)[cc->getFocus()]->getCoords();
    }
    return this->coords;
}

MoveResult Location::move(const Direction dir, const bool userEvent)
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


#if 0
/**
 * Push a location onto the stack
 */
static Location *locationPush(Location *stack, Location *loc)
{
    loc->prev = stack;
    return loc;
}
#endif

/**
 * Pop a location off the stack
 */
static Location *locationPop(Location **stack)
{
    Location *loc = *stack;
    *stack = (*stack)->prev;
    loc->prev = nullptr;
    return loc;
}
