/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "item.h"

#include "annotation.h"
#include "codex.h"
#include "combat.h"
#include "context.h"
#include "debug.h"
#include "dungeon.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "names.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "screen.h"
#include "tileset.h"
#include "u4.h"
#include "utils.h"
#include "weapon.h"

using std::string;

DestroyAllCreaturesCallback destroyAllCreaturesCallback;

void itemSetDestroyAllCreaturesCallback(DestroyAllCreaturesCallback callback) {
    destroyAllCreaturesCallback = callback;
}

int needStoneNames = 0;
unsigned char stoneMask = 0;

bool isRuneInInventory(int virt);
void putRuneInInventory(int virt);
bool isStoneInInventory(int virt);
void putStoneInInventory(int virt);
bool isItemInInventory(int item);
bool isSkullInInventory(int item);
void putItemInInventory(int item);
void useBBC(int item);
void useHorn(int item);
void useWheel(int item);
void useSkull(int item);
void useStone(int item);
void useKey(int item);
bool isMysticInInventory(int mystic);
void putMysticInInventory(int mystic);
bool isWeaponInInventory(int weapon);
void putWeaponInInventory(int weapon);
void useTelescope(int notused);
bool isReagentInInventory(int reag);
void putReagentInInventory(int reag);
bool isAbyssOpened(const Portal *p);
void itemHandleStones(const string &color);

static const ItemLocation items[] = {
    { "Alraune", NULL, "mandrake1",
      &isReagentInInventory, &putReagentInInventory, NULL, REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Alraune", NULL, "mandrake2",
      &isReagentInInventory, &putReagentInInventory, NULL, REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Schatten", NULL, "nightshade1",
      &isReagentInInventory, &putReagentInInventory, NULL, REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY},
    { "Schatten", NULL, "nightshade2",
      &isReagentInInventory, &putReagentInInventory, NULL, REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "die Glocke des Mutes", "glocke", "bell",
      &isItemInInventory, &putItemInInventory, &useBBC, ITEM_BELL, 0 },
    { "das Buch der Wahrheit", "buch", "book",
      &isItemInInventory, &putItemInInventory, &useBBC, ITEM_BOOK, 0 },
    { "die Kerze der Liebe", "kerze", "candle",
      &isItemInInventory, &putItemInInventory, &useBBC, ITEM_CANDLE, 0 },
    { "ein Silbernes Horn", "horn", "horn",
      &isItemInInventory, &putItemInInventory, &useHorn, ITEM_HORN, 0 },
    { "das Steuer von Seiner Majest{t Schiff 'Kap'", "steuer", "wheel",
      &isItemInInventory, &putItemInInventory, &useWheel, ITEM_WHEEL, 0 },
    { "den Sch{del Mondains des Zauberers", "sch{del", "skull",
      &isSkullInInventory, &putItemInInventory, &useSkull, ITEM_SKULL, SC_NEWMOONS },
    { "den Roten Stein", "rot", "redstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_RED, 0 },
    { "den Orangenen Stein", "orange", "orangestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_ORANGE, 0 },
    { "den Gelben Stein", "gelb", "yellowstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_YELLOW, 0 },
    { "den Gr}nen Stein", "gr}n", "greenstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_GREEN, 0 },
    { "den Blauen Stein", "blau", "bluestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_BLUE, 0 },
    { "den Violetten Stein", "violett", "purplestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_PURPLE, 0 },
    { "den Schwarzen Stein", "schwarz", "blackstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_BLACK, SC_NEWMOONS },
    { "den Wei~en Stein", "wei~", "whitestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, STONE_WHITE, 0 },

    /* handlers for using generic objects */
    { NULL, "stein",  NULL, &isStoneInInventory, NULL, &useStone, -1, 0 },
    { NULL, "steine", NULL, &isStoneInInventory, NULL, &useStone, -1, 0 },
    { NULL, "schl}ssel",   NULL, &isItemInInventory, NULL, &useKey, (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T), 0 },
    { NULL, "schl}ssel",   NULL, &isItemInInventory, NULL, &useKey, (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T), 0 },

    /* Lycaeum telescope */
    { NULL, NULL, "telescope", NULL, &useTelescope, NULL, 0, 0 },

    { "Mystische R}stung", NULL, "mysticarmor",
      &isMysticInInventory, &putMysticInInventory, NULL, ARMR_MYSTICROBES, SC_FULLAVATAR },
    { "Mystische Schwerter", NULL, "mysticswords",
      &isMysticInInventory, &putMysticInInventory, NULL, WEAP_MYSTICSWORD, SC_FULLAVATAR },
    { "die schwefligen ]berreste einer uralten sosarischen Laserpistole. Sie zerf{llt in deinen Fingern zu Asche", NULL, "lasergun", // lol, where'd that come from?
    		//Looks like someone was experimenting with "maps.xml". It effectively increments sulfur ash by one due to '16' being an invalid weapon index.
      &isWeaponInInventory, &putWeaponInInventory, 0, 16 },
    { "die Rune der Ehrlichkeit", NULL, "honestyrune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_HONESTY, 0 },
	{ "die Rune des Mitgef}hls", NULL, "compassionrune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_COMPASSION, 0 },
    { "die Rune der Tapferkeit", NULL, "valorrune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_VALOR, 0 },
    { "die Rune der Gerechtigkeit", NULL, "justicerune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_JUSTICE, 0 },
    { "die Rune des Verzichts", NULL, "sacrificerune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_SACRIFICE, 0 },
    { "die Rune der Ehre", NULL, "honorrune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_HONOR, 0 },
    { "die Rune der Spiritualit{t", NULL, "spiritualityrune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_SPIRITUALITY, 0 },
    { "die Rune der Demut", NULL, "humilityrune",
      &isRuneInInventory, &putRuneInInventory, NULL, RUNE_HUMILITY, 0 }
};

#define N_ITEMS (sizeof(items) / sizeof(items[0]))

bool isRuneInInventory(int virt) {
    return c->saveGame->runes & virt;
}

void putRuneInInventory(int virt) {
    c->party->member(0)->awardXp(100);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->runes |= virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

bool isStoneInInventory(int virt) {
    /* generic test: does the party have any stones yet? */
    if (virt == -1)
        return (c->saveGame->stones > 0);
    /* specific test: does the party have a specific stone? */
    else return c->saveGame->stones & virt;
}

void putStoneInInventory(int virt) {
    c->party->member(0)->awardXp(200);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->stones |= virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

bool isItemInInventory(int item) {
    return c->saveGame->items & item;
}

bool isSkullInInventory(int unused) {
    return (c->saveGame->items & (ITEM_SKULL | ITEM_SKULL_DESTROYED));
}

void putItemInInventory(int item) {
    c->party->member(0)->awardXp(400);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->items |= item;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

/**
 * Use bell, book, or candle on the entrance to the Abyss
 */
void useBBC(int item) {
    Coords abyssEntrance(0xe9, 0xe9);
    /* on top of the Abyss entrance */
    if (c->location->coords == abyssEntrance) {
        /* must use bell first */
        if (item == ITEM_BELL) {
            screenMessage("\nDIE GLOCKE L[UTET FORT UND FORT!\n");
            c->saveGame->items |= ITEM_BELL_USED;
        }
        /* then the book */
        else if ((item == ITEM_BOOK) && (c->saveGame->items & ITEM_BELL_USED)) {
            screenMessage("\nDIE WORTE HALLEN MIT DEM L[UTEN MIT!\n");
            c->saveGame->items |= ITEM_BOOK_USED;
        }
        /* then the candle */
        else if ((item == ITEM_CANDLE) && (c->saveGame->items & ITEM_BOOK_USED)) {
            screenMessage("\nALS DU DIE KERZE ENTZ]NDEST, ERBEBT DIE ERDE!\n");
            c->saveGame->items |= ITEM_CANDLE_USED;
        }
        else screenMessage("\nHMM... KEINE WIRKUNG!\n");
    }
    /* somewhere else */
    else screenMessage("\nHMM... KEINE WIRKUNG!\n");
}

/**
 * Uses the silver horn
 */
void useHorn(int item) {
    screenMessage("\nDAS HORN L[SST EINEN SCHAUERLICHEN KLANG ERSCHALLEN!\n");
    c->aura->set(Aura::HORN, 10);
}

/**
 * Uses the wheel (if on board a ship)
 */
void useWheel(int item) {
    if ((c->transportContext == TRANSPORT_SHIP) && (c->saveGame->shiphull == 50)) {
        screenMessage("\nNACH DEM EINBAU ERGL]HT DAS STEUER IN BLAUEM LICHTE!\n");
        c->party->setShipHull(99);
    }
    else screenMessage("\nHMM... KEINE WIRKUNG!\n");
}

/**
 * Uses or destroys the skull of Mondain
 */
void useSkull(int item) {
    /* FIXME: check to see if the abyss must be opened first
       for the skull to be *able* to be destroyed */

    /* We do the check here instead of in the table, because we need to distinguish between a
       never-found skull and a destroyed skull. */
    if (c->saveGame->items & ITEM_SKULL_DESTROYED) {
        screenMessage("\nBESITZT DU NICHT!\n");
        return;
    }

    /* destroy the skull! pat yourself on the back */
    if (c->location->coords.x == 0xe9 && c->location->coords.y == 0xe9) {
        screenMessage("\n\nDU WIRFST DEN SCH[DEL MONDAINS IN DEN ABGRUND!\n");
        c->saveGame->items = (c->saveGame->items & ~ITEM_SKULL) | ITEM_SKULL_DESTROYED;
        c->party->adjustKarma(KA_DESTROYED_SKULL);
    }

    /* use the skull... bad, very bad */
    else {
        screenMessage("\n\nDU ERHEBST DEN B\\SEN SCH[DEL DES ZAUBERERS MONDAIN...\n");

        /* destroy all creatures */
        (*destroyAllCreaturesCallback)();

        /* we don't lose the skull until we toss it into the abyss */
        //c->saveGame->items = (c->saveGame->items & ~ITEM_SKULL);
        c->party->adjustKarma(KA_USED_SKULL);
    }
}

/**
 * Handles using the virtue stones in dungeon altar rooms and on dungeon altars
 */
void useStone(int item) {
    MapCoords coords;
    unsigned char stone = static_cast<unsigned char>(item);

    static unsigned char truth   = STONE_WHITE | STONE_PURPLE | STONE_GREEN  | STONE_BLUE;
    static unsigned char love    = STONE_WHITE | STONE_YELLOW | STONE_GREEN  | STONE_ORANGE;
    static unsigned char courage = STONE_WHITE | STONE_RED    | STONE_PURPLE | STONE_ORANGE;
    static unsigned char *attr   = NULL;

    c->location->getCurrentPosition(&coords);

    /**
     * Named a specific stone (after using "stone" or "stones")
     */
    if (item != -1) {
        CombatMap *cm = getCombatMap();

        if (needStoneNames) {
            /* named a stone while in a dungeon altar room */
            if (c->location->context & CTX_ALTAR_ROOM) {
                needStoneNames--;

                switch(cm->getAltarRoom()) {
                case VIRT_TRUTH: attr = &truth; break;
                case VIRT_LOVE: attr = &love; break;
                case VIRT_COURAGE: attr = &courage; break;
                default: break;
                }

                /* make sure we're in an altar room */
                if (attr) {
                    /* we need to use the stone, and we haven't used it yet */
                    if ((*attr & stone) && (stone & ~stoneMask))
                        stoneMask |= stone;
                    /* we already used that stone! */
                    else if (stone & stoneMask) {
                        screenMessage("\nSCHON BENUTZT!\n");
                        needStoneNames = 0;
                        stoneMask = 0; /* reset the mask so you can try again */
                        return;
                    }
                }
                else ASSERT(0, "Not in an altar room!");

                /* see if we have all the stones, if not, get more names! */
                if (attr && needStoneNames) {
                    screenMessage("\n%c:", 'E'-needStoneNames);
                    itemHandleStones(gameGetInput());
                }
                /* all the stones have been entered, verify them! */
                else {
                    unsigned short key = 0xFFFF;
                    switch(cm->getAltarRoom()) {
                        case VIRT_TRUTH:    key = ITEM_KEY_T; break;
                        case VIRT_LOVE:     key = ITEM_KEY_L; break;
                        case VIRT_COURAGE:  key = ITEM_KEY_C; break;
                        default: break;
                    }

                    /* in an altar room, named all of the stones, and don't have the key yet... */
                    if (attr && (stoneMask == *attr) && !(c->saveGame->items & key)) {
                        screenMessage("\nDU FINDEST EIN DRITTEL DES DREITEILIGEN SCHL]SSELS!\n");
                        c->saveGame->items |= key;
                    }
                    else screenMessage("\nHMM... KEINE WIRKUNG!\n");

                    stoneMask = 0; /* reset the mask so you can try again */
                }
            }

            /* Otherwise, we're asking for a stone while in the abyss on top of an altar */
            else {
                /* see if they entered the correct stone */
                if (stone == (1 << c->location->coords.z)) {
                    if (c->location->coords.z < 7) {
                        /* replace the altar with a down-ladder */
                        MapCoords coords;
                        screenMessage("\n\nDER ALTAR VERWANDELT SICH VOR DEINEN AUGEN!\n");
                        c->location->getCurrentPosition(&coords);
                        c->location->map->annotations->add(coords, c->location->map->tileset->getByName("down_ladder")->getId());
                    }
                    /* start chamber of the codex sequence... */
                    else {
                        codexStart();
                    }
                }
                else screenMessage("\nHMM... KEINE WIRKUNG!\n");
            }
        }
        else {
            screenMessage("\nKEIN NUTZBARER GEGENSTAND!\n");
            stoneMask = 0; /* reset the mask so you can try again */
        }
    }

    /**
     * in the abyss, on an altar to place the stones
     */
    else if ((c->location->map->id == MAP_ABYSS) &&
             (c->location->context & CTX_DUNGEON) &&
             (dynamic_cast<Dungeon *>(c->location->map)->currentToken() == DUNGEON_ALTAR)) {

        int virtueMask = getBaseVirtues((Virtue)c->location->coords.z);
        if (virtueMask > 0)
            screenMessage("\n\nAls du dich n{herst, erschallt eine Stimme: Welche Tugend erstehet aus %s?\n\n", getBaseVirtueName(virtueMask));
        else screenMessage("\n\nEine Stimme erschallt: Welche Tugend hanget nicht an der Wahrheit, der Liebe und dem Mute?\n\n");
        string virtue = gameGetInput();

        if (strncasecmp(virtue.c_str(), getVirtueName((Virtue)c->location->coords.z), 6) == 0) {
            /* now ask for stone */
            screenMessage("\n\nDie Stimme spricht: Benutze deinen Stein.\n\nFARBE:\n");
            needStoneNames = 1;
            itemHandleStones(gameGetInput());
        }
        else {
            screenMessage("\nHMM... KEINE WIRKUNG!\n");
        }
    }

    /**
     * in a dungeon altar room, on the altar
     */
    else if ((c->location->context & CTX_ALTAR_ROOM) &&
             coords.x == 5 && coords.y == 5) {
        needStoneNames = 4;
        screenMessage("\n\nES GIBT \\FFNUNGEN F]R 4 STEINE.\nWELCHE FARBEN:\nA:");
        itemHandleStones(gameGetInput());
    }
    else screenMessage("\nSIE PASSEN HIER NICHT!\n");
    // This used to say "\nNo place to Use them!\nHmm...No effect!\n"
    // That doesn't match U4DOS; does it match another?
}

void useKey(int item) {
    screenMessage("\nSIE PASSEN HIER NICHT!\n");
}

bool isMysticInInventory(int mystic) {
    /* FIXME: you could feasibly get more mystic weapons and armor if you
       have 8 party members and equip them all with everything,
       then search for Mystic Weapons/Armor again

       or, you could just sell them all and search again.  What an easy
       way to make some cash!

       This would be a good candidate for an xu4 "extended" savegame
       format.
    */
    if (mystic == WEAP_MYSTICSWORD)
        return c->saveGame->weapons[WEAP_MYSTICSWORD] > 0;
    else if (mystic == ARMR_MYSTICROBES)
        return c->saveGame->armor[ARMR_MYSTICROBES] > 0;
    else
        ASSERT(0, "Invalid mystic item was tested in isMysticInInventory()");
    return false;
}

void putMysticInInventory(int mystic) {
    c->party->member(0)->awardXp(400);
    c->party->adjustKarma(KA_FOUND_ITEM);
    if (mystic == WEAP_MYSTICSWORD)
        c->saveGame->weapons[WEAP_MYSTICSWORD] += 8;
    else if (mystic == ARMR_MYSTICROBES)
        c->saveGame->armor[ARMR_MYSTICROBES] += 8;
    else
        ASSERT(0, "Invalid mystic item was added in putMysticInInventory()");
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

bool isWeaponInInventory(int weapon) {
    if (c->saveGame->weapons[weapon])
        return true;
    else {
        for (int i = 0; i < c->party->size(); i++) {
            if (c->party->member(i)->getWeapon()->getType() == weapon)
                return true;
        }
    }
    return false;
}

void putWeaponInInventory(int weapon) {
    c->saveGame->weapons[weapon]++;
}

void useTelescope(int notused) {
    screenMessage("DU SIEHST EIN DREHRAD AM TELESKOPE, MIT MARKIERUNGEN VON A BIS P.\nDU W[HLST:");
    int choice = AlphaActionController::get('p', "DU W[HLST:");

    if (choice == -1)
        return;

    gamePeerCity(choice, NULL);
}

bool isReagentInInventory(int reag) {
    return false;
}

void putReagentInInventory(int reag) {
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->reagents[reag] += xu4_random(8) + 2;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;

    if (c->saveGame->reagents[reag] > 99) {
        c->saveGame->reagents[reag] = 99;
        screenMessage("ZUM TEIL VERLOREN!\n");
    }
}

/**
 * Returns true if the specified conditions are met to be able to get the item
 */
bool itemConditionsMet(unsigned char conditions) {
    int i;

    if ((conditions & SC_NEWMOONS) &&
        !(c->saveGame->trammelphase == 0 && c->saveGame->feluccaphase == 0))
        return false;

    if (conditions & SC_FULLAVATAR) {
        for (i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] != 0)
                return false;
        }
    }

    if ((conditions & SC_REAGENTDELAY) &&
        (c->saveGame->moves & 0xF0) == c->saveGame->lastreagent)
        return false;

    return true;
}

/**
 * Returns an item location record if a searchable object exists at
 * the given location. NULL is returned if nothing is there.
 */
const ItemLocation *itemAtLocation(const Map *map, const Coords &coords) {
    unsigned int i;
    for (i = 0; i < N_ITEMS; i++) {
        if (!items[i].locationLabel)
            continue;
        if (map->getLabel(items[i].locationLabel) == coords &&
            itemConditionsMet(items[i].conditions))
            return &(items[i]);
    }
    return NULL;
}

/**
 * Uses the item indicated by 'shortname'
 */
void itemUse(const string &shortname) {
    unsigned int i;
    const ItemLocation *item = NULL;

    for (i = 0; i < N_ITEMS; i++) {
        if (items[i].shortname &&
            strcasecmp(deumlaut(items[i].shortname).c_str(), deumlaut(shortname).c_str()) == 0) {

            item = &items[i];

            /* item name found, see if we have that item in our inventory */
            if (!items[i].isItemInInventory || (*items[i].isItemInInventory)(items[i].data)) {

                /* use the item, if we can! */
                if (!item || !item->useItem)
                    screenMessage("\nKEIN NUTZBARER GEGENSTAND!\n");
                else
                    (*item->useItem)(items[i].data);
            }
            else
                screenMessage("\nBESITZT DU NICHT!\n");

            /* we found the item, no need to keep searching */
            break;
        }
    }

    /* item was not found */
    if (!item)
        screenMessage("\nKEIN NUTZBARER GEGENSTAND!\n");
}

/**
 * Checks to see if the abyss was opened
 */
bool isAbyssOpened(const Portal *p) {
    /* make sure the bell, book and candle have all been used */
    int items = c->saveGame->items;
    int isopened = (items & ITEM_BELL_USED) && (items & ITEM_BOOK_USED) && (items & ITEM_CANDLE_USED);

    if (!isopened)
        screenMessage("Betreten\nKANN NICHT!\n");
    return isopened;
}

/**
 * Handles naming of stones when used
 */
void itemHandleStones(const string &color) {
    bool found = false;

    for (int i = 0; i < 8; i++) {
      if (strcasecmp(deumlaut(color).c_str(), deumlaut(getStoneName((Virtue)i)).c_str()) == 0 &&
	  isStoneInInventory(1<<i)) {
          found = true;
          itemUse(color.c_str());
      }
    }

    if (!found) {
      screenMessage("\nBESITZT DU NICHT!\n");
      stoneMask = 0; /* make sure stone mask is reset */
    }
}
