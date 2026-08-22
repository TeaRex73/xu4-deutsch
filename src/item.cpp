/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>

#include "item.h"

#include "annotation.h"
#include "aura.h"
#include "codex.h"
#include "combat.h"
#include "context.h"
#include "coords.h"
#include "debug.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "tile.h"
#include "tileset.h"
#include "types.h"
#include "utils.h"
#include "weapon.h"


static DestroyAllCreaturesCallback destroyAllCreaturesCallback;

void itemSetDestroyAllCreaturesCallback(
    const DestroyAllCreaturesCallback callback
)
{
    destroyAllCreaturesCallback = callback;
}

static int needStoneNames = 0;
static unsigned char stoneMask = 0;
static bool isRuneInInventory(int virtue);
static void putRuneInInventory(int virtue);
static bool isStoneInInventory(int virtue);
static void putStoneInInventory(int virtue);
static bool isItemInInventory(int item);
static bool isSkullInInventory(int);
static void putItemInInventory(int item);
static void useBBC(int item);
static void useHorn(int);
static void useWheel(int);
static void useSkull(int);
static void useStone(int item);
static void useKey(int);
static bool isMysticInInventory(int mystic);
static void putMysticInInventory(int mystic);
static bool isWeaponInInventory(int weapon);
static void putWeaponInInventory(int weapon);
static void useTelescope(int);
static bool isReagentInInventory(int);
static void putReagentInInventory(int reagent);
static void itemHandleStones(const std::string &color);
static bool itemConditionsMet(unsigned int conditions);

static const ItemLocation items[] = {
    {
        .name = "Alraune",
        .shortname = nullptr,
        .locationLabel = "mandrake1",
        .isItemInInventory = &isReagentInInventory,
        .putItemInInventory = &putReagentInInventory,
        .useItem = nullptr,
        .data = REAG_MANDRAKE,
        .conditions = SC_NEW_MOONS | SC_REAGENT_DELAY
    },
    {
        .name = "Alraune",
        .shortname = nullptr,
        .locationLabel = "mandrake2",
        .isItemInInventory = &isReagentInInventory,
        .putItemInInventory = &putReagentInInventory,
        .useItem = nullptr,
        .data = REAG_MANDRAKE,
        .conditions = SC_NEW_MOONS | SC_REAGENT_DELAY
    },
    {
        .name = "Schatten",
        .shortname = nullptr,
        .locationLabel = "nightshade1",
        .isItemInInventory = &isReagentInInventory,
        .putItemInInventory = &putReagentInInventory,
        .useItem = nullptr,
        .data = REAG_NIGHTSHADE,
        .conditions = SC_NEW_MOONS | SC_REAGENT_DELAY
    },
    {
        .name = "Schatten",
        .shortname = nullptr,
        .locationLabel = "nightshade2",
        .isItemInInventory = &isReagentInInventory,
        .putItemInInventory = &putReagentInInventory,
        .useItem = nullptr,
        .data = REAG_NIGHTSHADE,
        .conditions = SC_NEW_MOONS | SC_REAGENT_DELAY
    },
    {
        .name = "die Glocke des Mutes",
        .shortname = "glocke",
        .locationLabel = "bell",
        .isItemInInventory = &isItemInInventory,
        .putItemInInventory = &putItemInInventory,
        .useItem = &useBBC,
        .data = ITEM_BELL,
        .conditions = 0
    },
    {
        .name = "das Buch der Wahrheit",
        .shortname = "buch",
        .locationLabel = "book",
        .isItemInInventory = &isItemInInventory,
        .putItemInInventory = &putItemInInventory,
        .useItem = &useBBC,
        .data = ITEM_BOOK,
        .conditions = 0
    },
    {
        .name = "die Kerze der Liebe",
        .shortname = "kerze",
        .locationLabel = "candle",
        .isItemInInventory = &isItemInInventory,
        .putItemInInventory = &putItemInInventory,
        .useItem = &useBBC,
        .data = ITEM_CANDLE,
        .conditions = 0
    },
    {
        .name = "ein Silbernes Horn",
        .shortname = "horn",
        .locationLabel = "horn",
        .isItemInInventory = &isItemInInventory,
        .putItemInInventory = &putItemInInventory,
        .useItem = &useHorn,
        .data = ITEM_HORN,
        .conditions = 0
    },
    {
        .name = "das Steuer von Seiner Majest{t Schiff 'Kap'",
        .shortname = "steuer",
        .locationLabel = "wheel",
        .isItemInInventory = &isItemInInventory,
        .putItemInInventory = &putItemInInventory,
        .useItem = &useWheel,
        .data = ITEM_WHEEL,
        .conditions = 0
    },
    {
        .name = "den Sch{del Mondains des Zauberers",
        .shortname = "sch{del",
        .locationLabel = "skull",
        .isItemInInventory = &isSkullInInventory,
        .putItemInInventory = &putItemInInventory,
        .useItem = &useSkull,
        .data = ITEM_SKULL,
        .conditions = SC_NEW_MOONS
    },
    {
        .name = "den Roten Stein",
        .shortname = "rot",
        .locationLabel = "redstone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_RED,
        .conditions = 0
    },
    {
        .name = "den Orangenen Stein",
        .shortname = "orange",
        .locationLabel = "orangestone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_ORANGE,
        .conditions = 0
    },
    {
        .name = "den Gelben Stein",
        .shortname = "gelb",
        .locationLabel = "yellowstone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_YELLOW,
        .conditions = 0
    },
    {
        .name = "den Gr}nen Stein",
        .shortname = "gr}n",
        .locationLabel = "greenstone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_GREEN,
        .conditions = 0
    },
    {
        .name = "den Blauen Stein",
        .shortname = "blau",
        .locationLabel = "bluestone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_BLUE,
        .conditions = 0
    },
    {
        .name = "den Violetten Stein",
        .shortname = "violett",
        .locationLabel = "purplestone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_PURPLE,
        .conditions = 0
    },
    {
        .name = "den Schwarzen Stein",
        .shortname = "schwarz",
        .locationLabel = "blackstone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_BLACK,
        .conditions = SC_NEW_MOONS
    },
    {
        .name = "den Wei~en Stein",
        .shortname = "wei~",
        .locationLabel = "whitestone",
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = &putStoneInInventory,
        .useItem = &useStone,
        .data = STONE_WHITE,
        .conditions = 0
    },
    /* handlers for using generic objects */
    {
        .name = nullptr,
        .shortname = "stein",
        .locationLabel = nullptr,
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = nullptr,
        .useItem = &useStone,
        .data = -1,
        .conditions = 0
    },
    {
        .name = nullptr,
        .shortname = "steine",
        .locationLabel = nullptr,
        .isItemInInventory = &isStoneInInventory,
        .putItemInInventory = nullptr,
        .useItem = &useStone,
        .data = -1,
        .conditions = 0
    },
    {
        .name = nullptr,
        .shortname = "schl}ssel",
        .locationLabel = nullptr,
        .isItemInInventory = &isItemInInventory,
        .putItemInInventory = nullptr,
        .useItem = &useKey,
        .data = ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T,
        .conditions = 0
    },
    {
        .name = nullptr,
        .shortname = "schl}ssel",
        .locationLabel = nullptr,
        .isItemInInventory = &isItemInInventory,
        .putItemInInventory = nullptr,
        .useItem = &useKey,
        .data = ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T,
        .conditions = 0
    },
    /* Lyceum telescope */
    {
        .name = nullptr,
        .shortname = nullptr,
        .locationLabel = "telescope",
        .isItemInInventory = nullptr,
        .putItemInInventory = &useTelescope,
        .useItem = nullptr,
        .data = 0,
        .conditions = 0
    },
    {
        .name = "Mystische R}stung",
        .shortname = nullptr,
        .locationLabel = "mysticarmor",
        .isItemInInventory = &isMysticInInventory,
        .putItemInInventory = &putMysticInInventory,
        .useItem = nullptr,
        .data = ARMR_MYSTICROBES,
        .conditions = SC_FULL_AVATAR
    },
    {
        .name = "Mystische Schwerter",
        .shortname = nullptr,
        .locationLabel = "mysticswords",
        .isItemInInventory = &isMysticInInventory,
        .putItemInInventory = &putMysticInInventory,
        .useItem = nullptr,
        .data = WEAP_MYSTICSWORD,
        .conditions = SC_FULL_AVATAR
    },
    {
        .name = "die verschwelten ]berreste einer uralten sosarischen "
        "Laserpistole. Sie zerf{llt in deinen Fingern zu Asche",
        .shortname = nullptr,
        .locationLabel = "lasergun",
        .isItemInInventory = &isWeaponInInventory,
        .putItemInInventory = &putWeaponInInventory,
        .useItem = nullptr,
        .data = 16,
        .conditions = 0
    },
    {
        .name = "die Rune der Ehrlichkeit",
        .shortname = nullptr,
        .locationLabel = "honestyrune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_HONESTY,
        .conditions = 0
    },
    {
        .name = "die Rune des Mitgef}hls",
        .shortname = nullptr,
        .locationLabel = "compassionrune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_COMPASSION,
        .conditions = 0
    },
    {
        .name = "die Rune der Tapferkeit",
        .shortname = nullptr,
        .locationLabel = "valorrune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_VALOR,
        .conditions = 0
    },
    {
        .name = "die Rune der Gerechtigkeit",
        .shortname = nullptr,
        .locationLabel = "justicerune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_JUSTICE,
        .conditions = 0
    },
    {
        .name = "die Rune des Verzichts",
        .shortname = nullptr,
        .locationLabel = "sacrificerune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_SACRIFICE,
        .conditions = 0
    },
    {
        .name = "die Rune der Ehre",
        .shortname = nullptr,
        .locationLabel = "honorrune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_HONOR,
        .conditions = 0
    },
    {
        .name = "die Rune der Spiritualit{t",
        .shortname = nullptr,
        .locationLabel = "spiritualityrune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_SPIRITUALITY,
        .conditions = 0
    },
    {
        .name = "die Rune der Demut",
        .shortname = nullptr,
        .locationLabel = "humilityrune",
        .isItemInInventory = &isRuneInInventory,
        .putItemInInventory = &putRuneInInventory,
        .useItem = nullptr,
        .data = RUNE_HUMILITY,
        .conditions = 0
    }
};

#define N_ITEMS (static_cast<int>(sizeof(items) / sizeof(items[0])))

static bool isRuneInInventory(const int virtue)
{
    return c->saveGame->runes & virtue;
}

static void putRuneInInventory(const int virtue)
{
    c->party->member(0)->awardXp(100);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->runes |= virtue;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

bool isStoneInInventory(const int virtue)
{
    /* generic test: does the party have any stones yet? */
    if (virtue == -1) {
        return c->saveGame->stones > 0;
    }
    /* specific test: does the party have a specific stone? */
    return c->saveGame->stones & virtue;
}

static void putStoneInInventory(const int virtue)
{
    c->party->member(0)->awardXp(200);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->stones |= virtue;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

static bool isItemInInventory(const int item)
{
    return c->saveGame->items & item;
}

static bool isSkullInInventory(int)
{
    return c->saveGame->items & (ITEM_SKULL | ITEM_SKULL_DESTROYED);
}

static void putItemInInventory(const int item)
{
    /* in u4apple2, findig an item on the world map
       (such as the bell) does not award xp, but I
       consider that a bug, so it's not reproduced here */
    c->party->member(0)->awardXp(400);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->items |= item;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}


/**
 * Use bell, book, or candle on the entrance to the Abyss
 */
static void useBBC(const int item)
{
    const Coords abyssEntrance(0xe9, 0xe9);
    /* on top of the Abyss entrance */
    if (c->location->coords == abyssEntrance) {
        /* must use bell first */
        if (item == ITEM_BELL) {
            screenMessage("\n\nDIE GLOCKE L[UTET FORT UND FORT!\n");
            c->saveGame->items |= ITEM_BELL_USED;
        }
        /* then the book */
        else if (item == ITEM_BOOK
                 && c->saveGame->items & ITEM_BELL_USED) {
            screenMessage(
                "\n\nDIE WORTE HALLEN MIT DEM L[UTEN MIT!\n"
            );
            c->saveGame->items |= ITEM_BOOK_USED;
        }
        /* then the candle */
        else if (item == ITEM_CANDLE
                 && c->saveGame->items & ITEM_BOOK_USED) {
            screenMessage(
                "\n\nALS DU DIE KERZE ENTZ]NDEST, ERBEBT DIE ERDE!\n"
            );
            // play abyss opening sound effect
            // taken from C64 version
            screenDisableCursor();
            soundPlay(SOUND_NPC_STRUCK, false);
            screenShake(1);
            EventHandler::sleep(100);
            soundPlay(SOUND_PC_STRUCK, false);
            screenShake(1);
            EventHandler::sleep(100);
            soundPlay(SOUND_NPC_STRUCK, false);
            screenShake(1);
            EventHandler::sleep(200);
            screenEnableCursor();
            c->saveGame->items |= ITEM_CANDLE_USED;
        } else {
            screenMessage("\n\nHMM... KEINE WIRKUNG!\n");
        }
    }
    /* somewhere else */
    else {
        screenMessage("\n\nHMM... KEINE WIRKUNG!\n");
    }
} // useBBC


/**
 * Uses the silver horn
 */
static void useHorn(int)
{
    screenMessage(
        "\n\nDAS HORN L[SST EINEN SCHAUERLICHEN KLANG ERSCHALLEN!\n"
    );
    if (settings.enhancements) soundPlay(SOUND_STORM, false);
    c->aura->set(Aura::HORN, 10);
}


/**
 * Uses the wheel (if on board a ship)
 */
static void useWheel(int)
{
    if (c->transportContext == TRANSPORT_SHIP
        && c->saveGame->shiphull == 50) {
        screenMessage(
            "\n\nNACH DEM EINBAU ERGL]HT DAS STEUER IN BLAUEM LICHTE!\n"
        );
        c->party->setShipHull(99);
    } else {
        screenMessage("\n\nHMM... KEINE WIRKUNG!\n");
    }
}


/**
 * Uses or destroys the skull of Mondain
 */
static void useSkull(int)
{
    /* FIXME: check to see if the abyss must be opened first
       for the skull to be *able* to be destroyed */
    /* We do the check here instead of in the table,
       because we need to distinguish between a
       never-found skull and a destroyed skull. */
    if (c->saveGame->items & ITEM_SKULL_DESTROYED) {
        screenMessage("\n\nBESITZT DU NICHT!\n");
        return;
    }
    /* destroy the skull! pat yourself on the back */
    const Coords abyssEntrance(0xe9, 0xe9);
    /* on top of the Abyss entrance */
    if (c->location->coords == abyssEntrance) {
        screenMessage(
            "\n\nDU WIRFST DEN SCH[DEL MONDAINS IN DEN ABGRUND!\n"
        );
        c->saveGame->items =
            (c->saveGame->items & ~ITEM_SKULL)
            | ITEM_SKULL_DESTROYED;
        c->party->adjustKarma(KA_DESTROYED_SKULL);
    }
    /* use the skull... bad, very bad */
    else {
        screenMessage(
            "\n\nDU ERHEBST DEN B\\SEN SCH[DEL DES ZAUBERERS MONDAIN...\n"
        );
        /* destroy all creatures */
        (*destroyAllCreaturesCallback)();
        /* we don't lose the skull until we toss it into the abyss */
        c->party->adjustKarma(KA_USED_SKULL);
    }
} // useSkull


/**
 * Handles using the virtue stones in dungeon altar rooms and on dungeon altars
 */
static void useStone(const int item)
{
    const auto stone = static_cast<unsigned char>(item);
    const MapCoords coords = c->location->getCurrentPosition();
    /**
     * Named a specific stone (after using "stone" or "stones")
     */
    if (item != -1) {
        const CombatMap *cm = getCombatMap();
        if (needStoneNames) {
            /* named a stone while in a dungeon altar room */
            if (c->location->context & CTX_ALTAR_ROOM) {
                constexpr unsigned char truth =
                    STONE_WHITE | STONE_PURPLE | STONE_GREEN | STONE_BLUE;
                constexpr unsigned char love =
                    STONE_WHITE | STONE_YELLOW | STONE_GREEN | STONE_ORANGE;
                constexpr unsigned char courage =
                    STONE_WHITE | STONE_RED | STONE_PURPLE | STONE_ORANGE;
                static const unsigned char *attr = nullptr;
                needStoneNames--;
                switch (cm->getAltarRoom()) {
                case VIRT_TRUTH:
                    attr = &truth;
                    break;
                case VIRT_LOVE:
                    attr = &love;
                    break;
                case VIRT_COURAGE:
                    attr = &courage;
                    break;
                default:
                    break;
                }
                /* make sure we're in an altar room */
                if (attr) {
                    /* we need to use the stone, and we haven't used it yet */
                    if (*attr & stone && stone & ~stoneMask) {
                        stoneMask |= stone;
                    }
                    /* we already used that stone! */
                    else if (stone & stoneMask) {
                        screenMessage("\nSCHON BENUTZT!\n");
                        needStoneNames = 0;
                        /* reset the mask so you can try again */
                        stoneMask = 0;
                        return;
                    }
                } else {
                    U4ASSERT(0, "Not in an altar room!");
                }
                /* see if we have all the stones, if not, get more names! */
                if (attr && needStoneNames) {
                    screenMessage("\n%c:", 'E' - needStoneNames);
                    itemHandleStones(gameGetInput());
                }
                /* all the stones have been entered,
                   verify them! */
                else {
                    unsigned short key;
                    switch (cm->getAltarRoom()) {
                    case VIRT_TRUTH:
                        key = ITEM_KEY_T;
                        break;
                    case VIRT_LOVE:
                        key = ITEM_KEY_L;
                        break;
                    case VIRT_COURAGE:
                        key = ITEM_KEY_C;
                        break;
                    default:
                        key = static_cast<unsigned short>(-1);
                        break;
                    }
                    /* in an altar room, named all of the stones, and don't
                       have the key yet... */
                    if (attr
                        && stoneMask == *attr
                        && !(c->saveGame->items & key)) {
                        screenMessage(
                            "\n\nDU FINDEST EIN DRITTEL DES DREITEILIGEN "
                            "SCHL]SSELS!\n"
                        );
                        c->saveGame->items |= key;
                    } else {
                        screenMessage("\n\nHMM... KEINE WIRKUNG!\n");
                    }
                    /* reset the mask so you can try again */
                    stoneMask = 0;
                }
            }
            /* Otherwise, we're asking for a stone
               while in the abyss on top of an altar */
            else {
                /* see if they entered the correct stone */
                if (stone == 1 << c->location->coords.z) {
                    if (c->location->coords.z < 7) {
                        /* replace the altar with a down-ladder */
                        screenMessage(
                            "\n\nDER ALTAR VERWANDELT SICH VOR DEINEN AUGEN!\n"
                        );
                        const MapCoords ladderCoords =
                            c->location->getCurrentPosition();
                        c->location->map->annotations->add(
                                ladderCoords,
                                c->location->map->tileset->getByName(
                                    "down_ladder"
                                )->getId()
                            );
                    }
                    /* start chamber of the codex
                       sequence... */
                    else {
                        EventHandler::simulateDiskLoad(2000, false);
                        codexStart();
                    }
                } else {
                    screenMessage("\n\nHMM... KEINE WIRKUNG!\n");
                }
            }
        } else {
            screenMessage("\n\nKEIN NUTZBARER GEGENSTAND!\n");
            /* reset the mask so you can try again */
            stoneMask = 0;
        }
    }
    /**
     * in the abyss, on an altar to place the stones
     */
    else if (c->location->map->id == MAP_ABYSS
             && c->location->context & CTX_DUNGEON
             && dynamic_cast<Dungeon *>(c->location->map)->currentToken()
             == DUNGEON_ALTAR) {
        const int virtueMask = getBaseVirtues(
            static_cast<Virtue>(c->location->coords.z)
        );
        if (virtueMask > 0) {
            screenMessage(
                "\n\nAls du dich n{herst, erschallt eine Stimme: Welche "
                "Tugend erstehet aus %s?\n\n",
                getBaseVirtueName(virtueMask)
            );
        } else {
            screenMessage(
                "\n\nEine Stimme erschallt: Welche Tugend hanget nicht an "
                "der Wahrheit, der Liebe und dem Mute?\n\n"
            );
        }
        const std::string virtue = gameGetInput();
        if (xu4_strncasecmp(
                virtue.c_str(),
                getVirtueName(static_cast<Virtue>(c->location->coords.z)),
                6
            ) == 0) {
            /* now ask for stone */
            screenMessage(
                "\n\nDie Stimme spricht: Benutze deinen Stein.\n\nFARBE:\n"
            );
            needStoneNames = 1;
            itemHandleStones(gameGetInput());
        } else {
            screenMessage("\n\nHMM... KEINE WIRKUNG!\n");
        }
    }
    /**
     * in a dungeon altar room, on the altar
     */
    else if (c->location->context & CTX_ALTAR_ROOM
             && coords.x == 5
             && coords.y == 5) {
        needStoneNames = 4;
        screenMessage(
            "\n\nES GIBT \\FFNUNGEN F]R 4 STEINE.\nWELCHE FARBEN:\nA:"
        );
        itemHandleStones(gameGetInput());
    } else {
        screenMessage("\nSIE PASSEN HIER NICHT!\n");
    }
} // useStone

static void useKey(int)
{
    screenMessage("\nSIE PASSEN HIER NICHT!\n");
}

static bool isMysticInInventory(const int mystic)
{
    /* FIXME: you could feasibly get more mystic weapons and armor if you
       have 8 party members and equip them all with everything,
       then search for Mystic Weapons/Armor again.
       Or, you could just sell them all and search again.  What an easy
       way to make some cash!
       This would be a good candidate for an xu4 "extended" savegame
       format.
    */
    if (mystic == WEAP_MYSTICSWORD) {
        return c->saveGame->weapons[WEAP_MYSTICSWORD] > 0;
    }
    if (mystic == ARMR_MYSTICROBES) {
        return c->saveGame->armor[ARMR_MYSTICROBES] > 0;
    }
    U4ASSERT(0, "Invalid mystic item was tested in isMysticInInventory()");
    return false;
}

static void putMysticInInventory(const int mystic)
{
    c->party->member(0)->awardXp(400);
    c->party->adjustKarma(KA_FOUND_ITEM);
    if (mystic == WEAP_MYSTICSWORD) {
        c->saveGame->weapons[WEAP_MYSTICSWORD] += 8;
    } else if (mystic == ARMR_MYSTICROBES) {
        c->saveGame->armor[ARMR_MYSTICROBES] += 8;
    } else {
        U4ASSERT(0, "Invalid mystic item was added in putMysticInInventory()");
    }
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

static bool isWeaponInInventory(const int weapon)
{
    if (c->saveGame->weapons[weapon]) {
        return true;
    }
    for (int i = 0; i < c->party->size(); i++) {
        if (c->party->member(i)->getWeapon()->getType() == weapon) {
            return true;
        }
    }
    return false;
}

static void putWeaponInInventory(const int weapon)
{
    c->saveGame->weapons[weapon]++;
}

static void useTelescope(int)
{
    screenMessage(
        "\nDU SIEHST EIN EINSTELLRAD AM TELESKOPE, MIT MARKIERUNGEN VON "
        "A BIS P.\nDU W[HLST:"
    );
    const int choice = AlphaActionController::get('p', "DU W[HLST:");
    if (choice == -1) {
        return;
    }
    screenMessage("%c", 'A' + choice);
    gamePeerCity(choice, nullptr);
    screenMessage("\n");
}

static bool isReagentInInventory(int)
{
    /* Finding reagents is not hindered by already owning some */
    return false;
}

static void putReagentInInventory(const int reagent)
{
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->reagents[reagent] += xu4_random(8) + 2;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
    if (c->saveGame->reagents[reagent] > 99) {
        c->saveGame->reagents[reagent] = 99;
        screenMessage("ZUM TEIL VERLOREN!\n");
    }
}


/**
 * Returns true if the specified conditions are met to be able to get the item
 */
static bool itemConditionsMet(const unsigned int conditions)
{
    if (conditions & SC_NEW_MOONS
        && !(c->saveGame->trammelphase == 0
             && c->saveGame->feluccaphase == 0)) {
        return false;
    }
    if (conditions & SC_FULL_AVATAR) {
        for (const unsigned short karmum: c->saveGame->karma) {
            if (karmum != 0) {
                return false;
            }
        }
    }
    if (conditions & SC_REAGENT_DELAY
        && (c->saveGame->moves & 0xF0) == c->saveGame->lastreagent) {
        return false;
    }
    return true;
}


/**
 * Returns an item location record if a searchable object exists at
 * the given location. nullptr is returned if nothing is there.
 */
const ItemLocation *itemAtLocation(const Map *map, const Coords &coords)
{
    for (const auto &item: items) {
        if (!item.locationLabel) {
            continue;
        }
        if (map->getLabel(item.locationLabel) == coords
            && itemConditionsMet(item.conditions)) {
            return &item;
        }
    }
    return nullptr;
}


/**
 * Uses the item indicated by 'shortname'
 */
void itemUse(const std::string &shortname)
{
    bool foundItem = false;
    for (const auto &item: items) {
        if (item.shortname
            && xu4_strcasecmp(
                deumlaut(item.shortname).c_str(),
                deumlaut(shortname).c_str()
            ) == 0) {
            foundItem = true;
            /* item name found, see if we have that item in
               our inventory */
            if (!item.isItemInInventory
                || (*item.isItemInInventory)(item.data)) {
                /* use the item, if we can! */
                if (!item.useItem) {
                    soundPlay(SOUND_ERROR);
                    screenMessage("\n\nKEIN NUTZBARER GEGENSTAND!\n");
                } else {
                    (*item.useItem)(item.data);
                }
            } else {
                soundPlay(SOUND_ERROR);
                screenMessage("\n\nBESITZT DU NICHT!\n");
            }
            /* we found the item, no need to keep searching */
            break;
        }
    }
    /* item was not found */
    if (!foundItem) {
        screenMessage("\n\nKEIN NUTZBARER GEGENSTAND!\n");
    }
} // itemUse


/**
 * Checks to see if the abyss was opened
 */
bool isAbyssOpened(const Portal *)
{
    /* make sure the bell, book and candle have all been used */
    const int saveGameItems = c->saveGame->items;
    const bool isOpen = saveGameItems & ITEM_BELL_USED
        && saveGameItems & ITEM_BOOK_USED
        && saveGameItems & ITEM_CANDLE_USED;
    if (!isOpen) {
        soundPlay(SOUND_ERROR);
        screenMessage("Betreten\nKANN NICHT!\n");
    }
    return isOpen;
}


/**
 * Handles naming of stones when used
 */
static void itemHandleStones(const std::string &color)
{
    bool found = false;

    for (int i = 0; i < 8; i++) {
        if (xu4_strcasecmp(
                deumlaut(color).c_str(),
                deumlaut(getStoneName(static_cast<Virtue>(i))).c_str()
            ) == 0
            && isStoneInInventory(1 << i)) {
            found = true;
            itemUse(color);
        }
    }
    if (!found) {
        soundPlay(SOUND_ERROR);
        screenMessage("\n\nBESITZT DU NICHT!\n");
        stoneMask = 0; /* make sure stone mask is reset */
    }
}
