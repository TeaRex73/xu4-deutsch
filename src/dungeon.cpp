/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include "dungeon.h"

#include "annotation.h"
#include "context.h"
#include "debug.h"
#include "game.h"
#include "item.h"
#include "location.h"
#include "mapmgr.h"
#include "player.h"
#include "screen.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"


/**
 * Returns true if 'map' points to a dungeon map
 */
bool isDungeon(Map *punknown)
{
    Dungeon *pd;
    if ((pd = dynamic_cast<Dungeon *>(punknown)) != nullptr) {
        return true;
    } else {
        return false;
    }
}


Dungeon::~Dungeon()
{
    if (roomMaps) {
        for (unsigned int i = 0; i < n_rooms; i++) {
            delete roomMaps[i];
        }
    }
    delete[] rooms;
    delete[] roomMaps;
}

/**
 * Returns the name of the dungeon
 */
std::string Dungeon::getName()
{
    return name;
}


/**
 * Returns the dungeon token associated with the given dungeon tile
 */
DungeonToken Dungeon::tokenForTile(MapTile tile)
{
    typedef std::pair<std::string, int> sipair;
    const static sipair tileNames[] = {
        sipair("brick_floor", DUNGEON_CORRIDOR),
        sipair("up_ladder", DUNGEON_LADDER_UP),
        sipair("down_ladder", DUNGEON_LADDER_DOWN),
        sipair("up_down_ladder", DUNGEON_LADDER_UPDOWN),
        sipair("chest", DUNGEON_CHEST),
        sipair("magic_orb", DUNGEON_MAGIC_ORB),
        sipair("wind_trap", DUNGEON_TRAP),
        sipair("falling_rocks_trap", DUNGEON_TRAP),
        sipair("pit_trap", DUNGEON_TRAP),
        sipair("fountain_normal", DUNGEON_FOUNTAIN),
        sipair("fountain_healing", DUNGEON_FOUNTAIN),
        sipair("fountain_acid", DUNGEON_FOUNTAIN),
        sipair("fountain_cure", DUNGEON_FOUNTAIN),
        sipair("fountain_poison", DUNGEON_FOUNTAIN),
        sipair("dungeon_poison_field", DUNGEON_FIELD),
        sipair("dungeon_energy_field", DUNGEON_FIELD),
        sipair("dungeon_fire_field", DUNGEON_FIELD),
        sipair("dungeon_sleep_field", DUNGEON_FIELD),
        sipair("dungeon_altar", DUNGEON_ALTAR),
        sipair("dungeon_door", DUNGEON_DOOR),
        sipair("dungeon_room_0", DUNGEON_ROOM),
        sipair("dungeon_room_1", DUNGEON_ROOM),
        sipair("dungeon_room_2", DUNGEON_ROOM),
        sipair("dungeon_room_3", DUNGEON_ROOM),
        sipair("dungeon_room_4", DUNGEON_ROOM),
        sipair("dungeon_room_5", DUNGEON_ROOM),
        sipair("dungeon_room_6", DUNGEON_ROOM),
        sipair("dungeon_room_7", DUNGEON_ROOM),
        sipair("dungeon_room_8", DUNGEON_ROOM),
        sipair("dungeon_room_9", DUNGEON_ROOM),
        sipair("dungeon_room_a", DUNGEON_ROOM),
        sipair("dungeon_room_b", DUNGEON_ROOM),
        sipair("dungeon_room_c", DUNGEON_ROOM),
        sipair("dungeon_room_d", DUNGEON_ROOM),
        sipair("dungeon_room_e", DUNGEON_ROOM),
        sipair("dungeon_room_f", DUNGEON_ROOM),
        sipair("secret_door", DUNGEON_SECRET_DOOR),
        sipair("brick_wall", DUNGEON_WALL),
        sipair("", 0)
    };
    int i;
    Tile *t = tileset->get(tile.getId());
    for (i = 0; !tileNames[i].first.empty(); i++) {
        if (t->getName() == tileNames[i].first) {
            return DungeonToken(tileNames[i].second);
        }
    }
    return DungeonToken(0);
}


/**
 * Returns the dungeon token for the current location
 */
DungeonToken Dungeon::currentToken()
{
    return tokenAt(c->location->coords);
}


/**
 * Return the dungeon sub-token associated with the given dungeon tile.
 *
 */


/**
 * Returns the dungeon sub-token for the current location
 */
unsigned char Dungeon::currentSubToken()
{
    return subTokenAt(c->location->coords);
}


/**
 * Returns the dungeon token for the given coordinates
 */
DungeonToken Dungeon::tokenAt(MapCoords coords)
{
    return tokenForTile(*getTileFromData(coords));
}


/**
 * Returns the dungeon sub-token for the given coordinates.  The
 * subtoken is encoded in the lower bits of the map raw data.  For
 * instance, for the raw value 0x91, returns FOUNTAIN_HEALING NOTE:
 * This function will always need type-casting to the token type
 * necessary
 */
unsigned char Dungeon::subTokenAt(MapCoords coords)
{
    int index = coords.x + (coords.y * width) + (width * height * coords.z);
    return dataSubTokens[index];
}


/**
 * Handles 's'earching while in dungeons
 */
void dungeonSearch(void)
{
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    DungeonToken token = dungeon->currentToken();
    Annotation::List a = dungeon->annotations->allAt(c->location->coords);
    const ItemLocation *item;
    if (a.size() > 0) {
        token = DUNGEON_CORRIDOR;
    }
    screenMessage("Nachschauen...\n");
    switch (token) {
    case DUNGEON_MAGIC_ORB:
        dungeonTouchOrb();
        break;
    case DUNGEON_FOUNTAIN:
        dungeonDrinkFountain();
        break;
    default:
        /* see if there is an item at the current location
           (stones on altars, etc.) */
        item = itemAtLocation(dungeon, c->location->coords);
        if (item) {
            if ((*item->isItemInInventory != nullptr)
                && (*item->isItemInInventory)(item->data)) {
                screenMessage("%cHIER IST NICHTS!%c\n", FG_GREY, FG_WHITE);
            } else {
                if (item->name) {
                    screenMessage(
                        "DU FINDEST...\n%s!\n", uppercase(item->name).c_str()
                    );
                }
                (*item->putItemInInventory)(item->data);
            }
        } else {
            screenMessage("%cHIER IST NICHTS!%c\n", FG_GREY, FG_WHITE);
        }
        break;
    } // switch
} // dungeonSearch


/**
 * Drink from the fountain at the current location
 */
void dungeonDrinkFountain()
{
    screenMessage("Du findest eine Quelle.\nWer trinkt-");
    int player = gameGetPlayer(false, false, false);
    if (player == -1) {
        return;
    }
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    FountainType type = static_cast<FountainType>(dungeon->currentSubToken());
    switch (type) {
    case FOUNTAIN_NORMAL:
        screenMessage("\nHmm... Keine Wirkung!\n");
        break;
    case FOUNTAIN_HEALING:
        if (c->party->member(player)->heal(HT_FULLHEAL)) {
            screenMessage("\nAhh... Erfrischend!\n");
        } else {
            screenMessage("\nHmm... Keine Wirkung!\n");
        }
        break;
    case FOUNTAIN_ACID:
        c->party->member(player)->applyDamage(100);
        screenMessage("\nB{h... Scheu~lich!\n");
        break;
    case FOUNTAIN_CURE:
        if (c->party->member(player)->heal(HT_CURE)) {
            screenMessage("\nMmm... K|stlich!\n");
        } else {
            screenMessage("\nHmm... Keine Wirkung!\n");
        }
        break;
    case FOUNTAIN_POISON:
        if (c->party->member(player)->getStatus() != STAT_POISONED) {
            soundPlay(SOUND_POISON_DAMAGE);
            c->party->member(player)->applyEffect(EFFECT_POISON);
            /* 100 damage to drinker also */
            c->party->member(player)->applyDamage(100);
            screenMessage("\nArchh... R|chel... Japs...\n");
        } else {
            screenMessage("\nHmm... Keine Wirkung!\n");
        }
        break;
    default:
        ASSERT(
            0,
            "Invalid call to dungeonDrinkFountain: no fountain at current "
            "location"
        );
    } // switch
} // dungeonDrinkFountain


/**
 * Touch the magical ball at the current location
 */
void dungeonTouchOrb()
{
    screenMessage("Du findest eine magische Kugel.\nWer ber}hrt-");
    int player = gameGetPlayer(false, false, false);
    if (player == -1) {
        return;
    }
    int stats = 0;
    int damage = 0;
    /* Get current position and find a replacement tile for it */
    Tile *orb_tile = c->location->map->tileset->getByName("magic_orb");
    MapTile replacementTile(
        c->location->getReplacementTile(c->location->coords, orb_tile)
    );
    switch (c->location->map->id) {
    case MAP_DECEIT:
        stats = STATSBONUS_INT;
        break;
    case MAP_DESPISE:
        stats = STATSBONUS_DEX;
        break;
    case MAP_DESTARD:
        stats = STATSBONUS_STR;
        break;
    case MAP_WRONG:
        stats = STATSBONUS_INT | STATSBONUS_DEX;
        break;
    case MAP_COVETOUS:
        stats = STATSBONUS_DEX | STATSBONUS_STR;
        break;
    case MAP_SHAME:
        stats = STATSBONUS_INT | STATSBONUS_STR;
        break;
    case MAP_HYTHLOTH:
        stats = STATSBONUS_INT | STATSBONUS_DEX | STATSBONUS_STR;
        break;
    default:
        break;
    }
    screenMessage("\n");
    /* give stats bonuses */
    if (stats & STATSBONUS_STR) {
        screenMessage("Str + 5\n");
        AdjustValueMax(c->saveGame->players[player].str, 5, 50);
        damage += 200;
    }
    if (stats & STATSBONUS_DEX) {
        screenMessage("Ges + 5\n");
        AdjustValueMax(c->saveGame->players[player].dex, 5, 50);
        damage += 200;
    }
    if (stats & STATSBONUS_INT) {
        screenMessage("Int + 5\n");
        AdjustValueMax(c->saveGame->players[player].intel, 5, 50);
        damage += 200;
    }
    /* deal damage to the party member who touched the orb */
    soundPlay(SOUND_PC_STRUCK, false);
    c->party->member(player)->applyDamage(damage);
    /* remove the orb from the map */
    c->location->map->annotations->add(c->location->coords, replacementTile);
} // dungeonTouchOrb


/**
 * Handles dungeon traps
 */
bool dungeonHandleTrap(TrapType)
{
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    switch (static_cast<TrapType>(dungeon->currentSubToken())) {
    case TRAP_WINDS:
        screenMessage("\nWinde!\n");
        c->party->quenchTorch();
        break;
    case TRAP_FALLING_ROCK:
        // Treat falling rocks and pits like bomb traps
        // XXX: That's a little harsh.
        screenMessage("\nFelssturz!\n");
        c->party->applyEffect(EFFECT_LAVA);
        break;
    case TRAP_PIT:
        screenMessage("\nFallgrube!\n");
        c->party->applyEffect(EFFECT_LAVA);
        break;
    default:
        break;
    }
    return true;
}


/**
 * Returns true if a ladder-up is found at the given coordinates
 */
bool Dungeon::ladderUpAt(MapCoords coords)
{
    Annotation::List a = annotations->allAt(coords);
    if ((tokenAt(coords) == DUNGEON_LADDER_UP)
        || (tokenAt(coords) == DUNGEON_LADDER_UPDOWN)) {
        return true;
    }
    if (a.size() > 0) {
        Annotation::List::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile() == tileset->getByName("up_ladder")->getId()) {
                return true;
            }
        }
    }
    return false;
}


/**
 * Returns true if a ladder-down is found at the given coordinates
 */
bool Dungeon::ladderDownAt(MapCoords coords)
{
    Annotation::List a = annotations->allAt(coords);
    if ((tokenAt(coords) == DUNGEON_LADDER_DOWN)
        || (tokenAt(coords) == DUNGEON_LADDER_UPDOWN)) {
        return true;
    }
    if (a.size() > 0) {
        Annotation::List::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile() == tileset->getByName("down_ladder")->getId()) {
                return true;
            }
        }
    }
    return false;
}

bool Dungeon::validTeleportLocation(MapCoords coords)
{
    MapTile *tile = tileAt(coords, WITH_OBJECTS);
    return tokenForTile(*tile) == DUNGEON_CORRIDOR;
}
