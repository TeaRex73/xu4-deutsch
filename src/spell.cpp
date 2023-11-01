/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "u4.h"

#include "spell.h"

#include <cstring>
#include "annotation.h"
#include "combat.h"
#include "context.h"
#include "debug.h"
#include "direction.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "creature.h"
#include "moongate.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "tileset.h"
#include "utils.h"

SpellEffectCallback spellEffectCallback = nullptr;
CombatController *spellCombatController();
void spellMagicAttack(
    const std::string &tilename, Direction dir, int minDamage, int maxDamage
);
bool spellMagicAttackAt(
    const Coords &coords, MapTile attackTile, int attackDamage
);
static bool spellAwaken(int player);
static bool spellBlink(int dir);
static bool spellCure(int player);
static bool spellDispel(int dir);
static bool spellEField(int param);
static bool spellFireball(int dir);
static bool spellGate(int phase);
static bool spellHeal(int player);
static bool spellIceball(int dir);
static bool spellJinx(int);
static bool spellKill(int dir);
static bool spellLight(int);
static bool spellMMissle(int dir);
static bool spellNegate(int);
static bool spellOpen(int);
static bool spellProtect(int);
static bool spellRez(int player);
static bool spellQuick(int);
static bool spellSleep(int);
static bool spellTremor(int);
static bool spellUndead(int);
static bool spellView(int);
static bool spellWinds(int fromdir);
static bool spellXit(int);
static bool spellYup(int);
static bool spellZdown(int);

/* masks for reagents */
#define ASH (1 << REAG_ASH)
#define GINSENG (1 << REAG_GINSENG)
#define GARLIC (1 << REAG_GARLIC)
#define SILK (1 << REAG_SILK)
#define MOSS (1 << REAG_MOSS)
#define PEARL (1 << REAG_PEARL)
#define NIGHTSHADE (1 << REAG_NIGHTSHADE)
#define MANDRAKE (1 << REAG_MANDRAKE)

/* spell error messages */
static const struct {
    SpellCastError err;
    const char *msg;
} spellErrorMsgs[] = {
    { CASTERR_NOMIX, "KEINE GEMISCHT!\n" },
    { CASTERR_MPTOOLOW, "ZU WENIG M.P.!\nFEHLSCHLAG!\n" },
    { CASTERR_FAILED, "FEHLSCHLAG!\n" },
    { CASTERR_WRONGCONTEXT, "HIER NICHT!\n" },
    { CASTERR_COMBATONLY, "NUR IM KAMPFE!\nFEHLSCHLAG!\n" },
    { CASTERR_DUNGEONONLY, "NUR IN H\\HLEN!\nFEHLSCHLAG!\n" },
    { CASTERR_WORLDMAPONLY, "NUR IM FREIEN!\nFEHLSCHLAG!\n" }
};
static const Spell spells[] = {
    {
        "Avacus",
        GINSENG | GARLIC,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellAwaken,
        Spell::PARAM_PLAYER,
        5
    },
    {
        "Blincatus",
        SILK | MOSS,
        CTX_WORLDMAP,
        TRANSPORT_FOOT_OR_HORSE,
        &spellBlink,
        Spell::PARAM_DIR,
        15
    },
    {
        "Curitus",
        GINSENG | GARLIC,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellCure,
        Spell::PARAM_PLAYER,
        5
    },
    {
        "Dispelitus",
        ASH | GARLIC | PEARL,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellDispel,
        Spell::PARAM_DIR,
        20
    },
    {
        "Energitus",
        ASH | SILK | PEARL,
        static_cast<LocationContext>(CTX_COMBAT | CTX_DUNGEON | CTX_CITY),
        TRANSPORT_ANY,
        &spellEField,
        Spell::PARAM_TYPEDIR,
        10
    },
    {
        "Firbalatus",
        ASH | PEARL,
        CTX_COMBAT,
        TRANSPORT_ANY,
        &spellFireball,
        Spell::PARAM_DIR,
        15
    },
    {
        "Gatetus",
        ASH | PEARL | MANDRAKE,
        CTX_WORLDMAP,
        TRANSPORT_FOOT_OR_HORSE,
        &spellGate,
        Spell::PARAM_PHASE,
        40
    },
    {
        "Hilus",
        GINSENG | SILK,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellHeal,
        Spell::PARAM_PLAYER,
        10
    },
    {
        "Isbalatus",
        PEARL | MANDRAKE,
        CTX_COMBAT,
        TRANSPORT_ANY,
        &spellIceball,
        Spell::PARAM_DIR,
        20
    },
    {
        "Jinxus",
        PEARL | NIGHTSHADE | MANDRAKE,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellJinx,
        Spell::PARAM_NONE,
        30
    },
    {
        "Kilutus",
        PEARL | NIGHTSHADE,
        CTX_COMBAT,
        TRANSPORT_ANY,
        &spellKill,
        Spell::PARAM_DIR,
        25
    },
    {
        "Laitus",
        ASH,
        CTX_DUNGEON,
        TRANSPORT_ANY,
        &spellLight,
        Spell::PARAM_NONE,
        5
    },
    {
        "Missailus",
        ASH | PEARL,
        CTX_COMBAT,
        TRANSPORT_ANY,
        &spellMMissle,
        Spell::PARAM_DIR,
        5
    },
    {
        "Negativus",
        ASH | GARLIC | MANDRAKE,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellNegate,
        Spell::PARAM_NONE,
        20
    },
    {
        "Operatus",
        ASH | MOSS,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellOpen,
        Spell::PARAM_NONE,
        5
    },
    {
        "Protectus",
        ASH | GINSENG | GARLIC,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellProtect,
        Spell::PARAM_NONE,
        15
    },
    {
        "Quicatus",
        ASH | GINSENG | MOSS,
        CTX_ANY,
        TRANSPORT_ANY,
        &spellQuick,
        Spell::PARAM_NONE,
        20
    },
    {
        "Resurrectus",
        ASH | GINSENG | GARLIC | SILK | MOSS | MANDRAKE,
        CTX_NON_COMBAT,
        TRANSPORT_ANY,
        &spellRez,
        Spell::PARAM_PLAYER,
        45
    },
    {
        "Slipitus",
        SILK | GINSENG,
        CTX_COMBAT,
        TRANSPORT_ANY,
        &spellSleep,
        Spell::PARAM_NONE,
        15
    },
    {
        "Tremendus",
        ASH | MOSS | MANDRAKE,
        CTX_COMBAT,
        TRANSPORT_ANY,
        &spellTremor,
        Spell::PARAM_NONE,
        30
    },
    {
        "Undedus",
        ASH | GARLIC,
        CTX_COMBAT,
        TRANSPORT_ANY,
        &spellUndead,
        Spell::PARAM_NONE,
        15
    },
    {
        "Vietus",
        NIGHTSHADE | MANDRAKE,
        CTX_NON_COMBAT,
        TRANSPORT_ANY,
        &spellView,
        Spell::PARAM_NONE,
        15
    },
    {
        "Windatus",
        ASH | MOSS,
        CTX_WORLDMAP,
        TRANSPORT_ANY,
        &spellWinds,
        Spell::PARAM_FROMDIR,
        10
    },
    {
        "Xitatus",
        ASH | SILK | MOSS,
        CTX_DUNGEON,
        TRANSPORT_ANY,
        &spellXit,
        Spell::PARAM_NONE,
        15
    },
    {
        "Ypsapus",
        SILK | MOSS,
        CTX_DUNGEON,
        TRANSPORT_ANY,
        &spellYup,
        Spell::PARAM_NONE,
        10
    },
    {
        "Zidaunus",
        SILK | MOSS,
        CTX_DUNGEON,
        TRANSPORT_ANY,
        &spellZdown,
        Spell::PARAM_NONE,
        5
    }
};

#define N_SPELLS (sizeof(spells) / sizeof(spells[0]))

void spellSetEffectCallback(SpellEffectCallback callback)
{
    spellEffectCallback = callback;
}

Ingredients::Ingredients()
{
    for (int i = 0; i < REAG_MAX; i++) {
        reagents[i] = 0;
    }
}

bool Ingredients::addReagent(Reagent reagent)
{
    ASSERT(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    if (c->party->getReagent(reagent) < 1) {
        return false;
    }
    c->party->adjustReagent(reagent, -1);
    reagents[reagent]++;
    return true;
}

bool Ingredients::removeReagent(Reagent reagent)
{
    ASSERT(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    if (reagents[reagent] == 0) {
        return false;
    }
    c->party->adjustReagent(reagent, 1);
    reagents[reagent]--;
    return true;
}

int Ingredients::getReagent(Reagent reagent) const
{
    ASSERT(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    return reagents[reagent];
}

void Ingredients::revert()
{
    int reg;
    for (reg = 0; reg < REAG_MAX; reg++) {
        c->saveGame->reagents[reg] += reagents[reg];
        reagents[reg] = 0;
    }
}

bool Ingredients::checkMultiple(int batches) const
{
    for (int i = 0; i < REAG_MAX; i++) {
        /* see if there's enough reagents to mix
           (-1 because one is already counted) */
        if ((reagents[i] > 0) && (c->saveGame->reagents[i] < batches - 1)) {
            return false;
        }
    }
    return true;
}

void Ingredients::multiply(int batches)
{
    ASSERT(
        checkMultiple(batches),
        "not enough reagents to multiply ingredients by %d\n",
        batches
    );
    for (int i = 0; i < REAG_MAX; i++) {
        if (reagents[i] > 0) {
            c->saveGame->reagents[i] -= batches - 1;
            reagents[i] += batches - 1;
        }
    }
}

const char *spellGetName(unsigned int spell)
{
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    return spells[spell].name;
}

int spellGetRequiredMP(unsigned int spell)
{
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    return spells[spell].mp;
}

LocationContext spellGetContext(unsigned int spell)
{
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    return spells[spell].context;
}

TransportContext spellGetTransportContext(unsigned int spell)
{
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    return spells[spell].transportContext;
}

std::string spellGetErrorMessage(unsigned int spell, SpellCastError error)
{
    unsigned int i;
    SpellCastError err = error;
    /* try to find a more specific error message */
    if (err == CASTERR_WRONGCONTEXT) {
        switch (spells[spell].context) {
        case CTX_COMBAT:
            err = CASTERR_COMBATONLY;
            break;
        case CTX_DUNGEON:
            err = CASTERR_DUNGEONONLY;
            break;
        case CTX_WORLDMAP:
            err = CASTERR_WORLDMAPONLY;
            break;
        default:
            break;
        }
    }
    /* find the message that we're looking for and return it! */
    for (i = 0; i < sizeof(spellErrorMsgs) / sizeof(spellErrorMsgs[0]); i++) {
        if (err == spellErrorMsgs[i].err) {
            return std::string(spellErrorMsgs[i].msg);
        }
    }
    return std::string();
} // spellGetErrorMessage


/**
 * Mix reagents for a spell.  Fails and returns false if the reagents
 * selected were not correct.
 */
bool spellMix(unsigned int spell, const Ingredients *ingredients)
{
    int regmask, reg;
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    regmask = 0;
    for (reg = 0; reg < REAG_MAX; reg++) {
        if (ingredients->getReagent(static_cast<Reagent>(reg)) > 0) {
            regmask |= (1 << reg);
        }
    }
    if (regmask != spells[spell].components) {
        return false;
    }
    c->saveGame->mixtures[spell]++;
    return true;
}

Spell::Param spellGetParamType(unsigned int spell)
{
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    return spells[spell].paramType;
}


/**
 * Checks some basic prerequistes for casting a spell.  Returns an
 * error if no mixture is available, the context is invalid, or the
 * character doesn't have enough magic points.
 */
SpellCastError spellCheckPrerequisites(unsigned int spell, int character)
{
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    ASSERT(
        character >= 0 && character < c->saveGame->members,
        "character out of range: %d",
        character
    );
    if (c->saveGame->mixtures[spell] == 0) {
        return CASTERR_NOMIX;
    }
    if ((c->location->context & spells[spell].context) == 0) {
        return CASTERR_WRONGCONTEXT;
    }
    if ((c->transportContext & spells[spell].transportContext) == 0) {
        return CASTERR_FAILED;
    }
    if (c->party->member(character)->getMp() < spells[spell].mp) {
        return CASTERR_MPTOOLOW;
    }
    return CASTERR_NOERROR;
}


/**
 * Casts spell.  Fails and returns false if the spell cannot be cast.
 * The error code is updated with the reason for failure.
 */
bool spellCast(
    unsigned int spell,
    int character,
    int param,
    SpellCastError *error,
    bool spellEffect
)
{
    int subject =
        (spells[spell].paramType == Spell::PARAM_PLAYER) ? param : -1;
    PartyMember *p = c->party->member(character);
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    ASSERT(
        character >= 0 && character < c->saveGame->members,
        "character out of range: %d",
        character
    );
    *error = spellCheckPrerequisites(spell, character);
    // subtract the mixture for even trying to cast the spell
    AdjustValueMin(c->saveGame->mixtures[spell], -1, 0);
    if (*error != CASTERR_NOERROR) {
        return false;
    }
    // If there's a negate magic aura, spells fail!
    if (*c->aura == Aura::NEGATE) {
        *error = CASTERR_FAILED;
        return false;
    }
    // subtract the mp needed for the spell
    p->adjustMp(-spells[spell].mp);
    if (spellEffect) {
        int time;
        /* recalculate spell speed - based on 5/sec */
        double MP_OF_LARGEST_SPELL = 45;
        int spellMp = spells[spell].mp;
        time = static_cast<int>(
            17790.4 / settings.spellEffectSpeed * spellMp / MP_OF_LARGEST_SPELL
        );
        soundPlay(SOUND_PREMAGIC_MANA_JUMBLE, false, time);
        EventHandler::wait_msecs(time);
        (*spellEffectCallback)(spell + 'a', subject, SOUND_MAGIC);
    }
    if (!(*spells[spell].spellFunc)(param)) {
        *error = CASTERR_FAILED;
        return false;
    }
    return true;
} // spellCast

CombatController *spellCombatController()
{
    CombatController *cc = dynamic_cast<CombatController *>(
        eventHandler->getController()
    );

    return cc;
}


/**
 * Makes a special magic ranged attack in the given direction
 */
void spellMagicAttack(
    const std::string &tilename, Direction dir, int minDamage, int maxDamage
)
{
    CombatController *controller = spellCombatController();
    PartyMemberVector *party = controller->getParty();
    MapTile tile = c->location->map->tileset->getByName(tilename)->getId();
    int attackDamage = (minDamage >= 0) ?
        (xu4_random(maxDamage) | minDamage) :
        maxDamage;

    std::vector<Coords> path = gameGetDirectionalActionPath(
        MASK_DIR(dir),
        MASK_DIR_ALL,
        (*party)[controller->getFocus()]->getCoords(),
        1,
        11,
        Tile::canAttackOverTile,
        false
    );
    for (std::vector<Coords>::iterator i = path.begin();
         i != path.end();
         i++) {
        if (spellMagicAttackAt(*i, tile, attackDamage)) {
            return;
        }
    }
}

bool spellMagicAttackAt(
    const Coords &coords, MapTile attackTile, int attackDamage
)
{
    bool objectHit = false;
    // int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;
    CombatMap *cm = getCombatMap();
    Creature *creature = cm->creatureAt(coords);
    if (!creature) {
        GameController::flashTile(coords, attackTile, 4);
    } else {
        objectHit = true;
        /* show the 'hit' tile */
        soundPlay(SOUND_NPC_STRUCK);
        GameController::flashTile(coords, attackTile, 4);
        /* apply the damage to the creature */
        CombatController *controller = spellCombatController();
        controller->getCurrentPlayer()->dealDamage(
            creature, attackDamage
        );
    }
    return objectHit;
}

static bool spellAwaken(int player)
{
    ASSERT(player < 8, "player out of range: %d", player);
    PartyMember *p = c->party->member(player);
    if ((player < c->party->size())
        && (p->getStatus() == STAT_SLEEPING)) {
        p->wakeUp();
        return true;
    }
    return false;
}

static bool spellBlink(int dir)
{
    int i, distance, diff, *var;
    bool success;
    Direction reverseDir = dirReverse(static_cast<Direction>(dir));
    MapCoords coords = c->location->coords;
    /* Blink doesn't work near the mouth of the abyss */
    /* Note: This means you can teleport to Hythloth from the top of the
       map, and that you can teleport to the abyss from the left edge of
       the map. At least in the original. This is now fixed. */
    if ((coords.x >= 192) && (coords.y >= 192)) {
        return false;
    }
    /* figure out what numbers we're working with */
    var = (dir & (DIR_WEST | DIR_EAST)) ? &coords.x : &coords.y;
    /* find the distance we are going to move */
    distance = (*var) % 0x10;
    if ((dir == DIR_EAST) || (dir == DIR_SOUTH)) {
        distance = 0x10 - distance;
    }
    /* see if we move another 16 spaces over */
    diff = 0x10 - distance;
    if ((diff > 0) && (xu4_random(diff * diff) > distance)) {
        distance += 0x10;
    }
    /* test our distance, and see if it works */
    for (i = 0; i < distance; i++) {
        coords.move(static_cast<Direction>(dir), c->location->map);
    }
    i = distance;
    /* begin walking backward until you find a valid spot */
    while ((i-- > 0)
           && !c->location->map->tileTypeAt(coords, WITH_OBJECTS)
           ->isWalkable()) {
        coords.move(reverseDir, c->location->map);
    }
    if (c->location->map->tileTypeAt(coords, WITH_OBJECTS)->isWalkable()) {
        /* we didn't move! */
        if (c->location->coords == coords) {
            success = false;
        } else if ((coords.x >= 192) && (coords.y >= 192)) {
            /* CHANGE: No teleporting onto isle of abyss and surroundings */
            success = false;
        } else {
            EventHandler::simulateDiskLoad(2000);
            c->location->coords = coords;
            success = true;
        }
    } else {
        success = false;
    }
    return success;
} // spellBlink

static bool spellCure(int player)
{
    ASSERT(player < 8, "player out of range: %d", player);
    GameController::flashTile(
        c->party->member(player)->getCoords(), "wisp", 4
    );
    return c->party->member(player)->heal(HT_CURE);
}

static bool spellDispel(int dir)
{
    MapTile *tile;
    MapCoords field;
    /*
     * get the location of the avatar (or current party member,
     * if in battle)
     */
    c->location->getCurrentPosition(&field);
    /*
     * find where we want to dispel the field
     */
    field.move(static_cast<Direction>(dir), c->location->map);
    GameController::flashTile(field, "wisp", 4);
    /*
     * if there is a field annotation, remove it and replace it with a
     * valid replacement annotation.  We do this because sometimes
     * dungeon triggers create annotations, that, if just removed,
     * leave a wall tile behind (or other unwalkable surface).  So, we
     * need to provide a valid replacement annotation to fill in the gap :)
     */
    Annotation::List a = c->location->map->annotations->allAt(field);
    if (a.size() > 0) {
        Annotation::List::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile().getTileType()->canDispel()) {
                /*
                 * get a replacement tile for the field
                 */
                MapTile newTile(c->location->getReplacementTile(
                                    field, i->getTile().getTileType()
                                ));
                c->location->map->annotations->remove(*i);
                c->location->map->annotations->add(
                    field, newTile, false, true
                );
                return true;
            }
        }
    }
    /*
     * if the map tile itself is a field, overlay it with replacement tile
     */
    tile = c->location->map->tileAt(field, WITHOUT_OBJECTS);
    if (!tile->getTileType()->canDispel()) {
        return false;
    }
    /*
     * get a replacement tile for the field
     */
    MapTile newTile(
        c->location->getReplacementTile(field, tile->getTileType())
    );
    c->location->map->annotations->add(field, newTile, false, true);
    return true;
} // spellDispel

static bool spellEField(int param)
{
    MapTile fieldTile(0);
    int fieldType;
    int dir;
    MapCoords coords;
    /* Unpack fieldType and direction */
    fieldType = param >> 4;
    dir = param & 0xF;
    /* Make sure params valid */
    if (c->location->map->type == Map::DUNGEON) {
        switch (fieldType) {
        case ENERGYFIELD_FIRE:
            fieldTile =
                c->location->map->tileset
                ->getByName("dungeon_fire_field")->getId();
            break;
        case ENERGYFIELD_LIGHTNING:
            fieldTile =
                c->location->map->tileset
                ->getByName("dungeon_energy_field")->getId();
            break;
        case ENERGYFIELD_POISON:
            fieldTile =
                c->location->map->tileset
                ->getByName("dungeon_poison_field")->getId();
            break;
        case ENERGYFIELD_SLEEP:
            fieldTile =
                c->location->map->tileset
                ->getByName("dungeon_sleep_field")->getId();
            break;
        default:
            return false;
        }
    } else {
        switch (fieldType) {
        case ENERGYFIELD_FIRE:
            fieldTile =
                c->location->map->tileset->getByName("fire_field")->getId();
            break;
        case ENERGYFIELD_LIGHTNING:
            fieldTile =
                c->location->map->tileset->getByName("energy_field")->getId();
            break;
        case ENERGYFIELD_POISON:
            fieldTile =
                c->location->map->tileset->getByName("poison_field")->getId();
            break;
        case ENERGYFIELD_SLEEP:
            fieldTile =
                c->location->map->tileset->getByName("sleep_field")->getId();
            break;
        default:
            return false;
        }
    }
    c->location->getCurrentPosition(&coords);
    coords.move(static_cast<Direction>(dir), c->location->map);
    if (MAP_IS_OOB(c->location->map, coords)) {
        return false;
    } else {
        /*
         * Observed behaviour on Amiga version of Ultima IV:
         * Field cast on other field: Works, unless original field
         * is lightning in which case it doesn't.
         * Field cast on creature: Works, creature remains the
         * visible tile
         * Field cast on top of field and then dispel = no fields left
         * The code below seems to produce this behaviour.
         */
        const Tile *tile =
            c->location->map->tileTypeAt(coords, WITH_GROUND_OBJECTS);
        if (!tile->isWalkable()) {
            return false;
        }
        /* Get rid of old field, if any */
        Annotation::List a =
            c->location->map->annotations->allAt(coords);
        if (a.size() > 0) {
            Annotation::List::iterator i;
            for (i = a.begin(); i != a.end(); i++) {
                if (i->getTile().getTileType()->canDispel()) {
                    c->location->map->annotations->remove(*i);
                }
            }
        }
        c->location->map->annotations->add(coords, fieldTile);
    }
    return true;
} // spellEField

static bool spellFireball(int dir)
{
    spellMagicAttack("hit_flash", static_cast<Direction>(dir), 24, 128);
    return true;
}

static bool spellGate(int phase)
{
    const Coords *moongate;
    GameController::flashTile(c->location->coords, "moongate", 4);
    moongate = moongateGetGateCoordsForPhase(phase);
    if (moongate) {
        EventHandler::simulateDiskLoad(2000);
        c->location->coords = *moongate;
    }
    return true;
}

static bool spellHeal(int player)
{
    ASSERT(player < 8, "player out of range: %d", player);
    GameController::flashTile(
        c->party->member(player)->getCoords(), "wisp", 4
    );
    c->party->member(player)->heal(HT_HEAL);
    return true;
}

static bool spellIceball(int dir)
{
    spellMagicAttack("magic_flash", static_cast<Direction>(dir), 32, 224);
    return true;
}

static bool spellJinx(int)
{
    c->aura->set(Aura::JINX, 10);
    return true;
}

static bool spellKill(int dir)
{
    spellMagicAttack("whirlpool", static_cast<Direction>(dir), -1, 232);
    return true;
}

static bool spellLight(int)
{
    c->party->lightTorch(100, false);
    return true;
}

static bool spellMMissle(int dir)
{
    spellMagicAttack("miss_flash", static_cast<Direction>(dir), 16, 64);
    return true;
}

static bool spellNegate(int)
{
    c->aura->set(Aura::NEGATE, 10);
    return true;
}

static bool spellOpen(int)
{
    getChest(-2);
    // HACK: -2 will not prompt for opener
    return true;
}

static bool spellProtect(int)
{
    c->aura->set(Aura::PROTECTION, 10);
    return true;
}

static bool spellRez(int player)
{
    ASSERT(player < 8, "player out of range: %d", player);
    return c->party->member(player)->heal(HT_RESURRECT);
}

static bool spellQuick(int)
{
    c->aura->set(Aura::QUICKNESS, 10);
    return true;
}

static bool spellSleep(int)
{
    CombatMap *cm = getCombatMap();
    CreatureVector creatures = cm->getCreatures();
    CreatureVector::iterator i;
    /* try to put each creature to sleep */
    for (i = creatures.begin(); i != creatures.end(); i++) {
        Creature *m = *i;
        Coords coords = m->getCoords();
        GameController::flashTile(coords, "wisp", 4);
        // BUGFIX from u4apple2: Balron resists sleep AND fire, which our
        // creatures.xml file cannot currenty model
        if ((m->getResists() != EFFECT_SLEEP)
            && (m->getId() != BALRON_ID)
            && (xu4_random(0xFF) >= m->getHp())) {
            soundPlay(SOUND_POISON_EFFECT);
            m->putToSleep();
            GameController::flashTile(coords, "sleep_field", 4);
        } else {
            soundPlay(SOUND_EVADE);
        }
    }
    return true;
}

static bool spellTremor(int)
{
    CombatController *ct = spellCombatController();
    CreatureVector creatures = ct->getMap()->getCreatures();
    CreatureVector::iterator i;
    for (i = creatures.begin(); i != creatures.end(); i++) {
        Creature *m = *i;
        Coords coords = m->getCoords();
        // GameController::flashTile(coords, "rocks", 4);
        /* creatures with over 192 hp are unaffected */
        if (m->getHp() > 192) {
            soundPlay(SOUND_EVADE);
            continue;
        } else {
            /* Deal maximum damage to creature */
            if (xu4_random(2) == 0) {
                soundPlay(SOUND_NPC_STRUCK);
                GameController::flashTile(coords, "hit_flash", 4);
                ct->getCurrentPlayer()->dealDamage(m, 0xFF);
            }
            /* Deal enough damage to creature to make it flee */
            else if (xu4_random(2) == 0) {
                soundPlay(SOUND_NPC_STRUCK);
                GameController::flashTile(coords, "hit_flash", 4);
                if (m->getHp() > 23) {
                    ct->getCurrentPlayer()->dealDamage(m, m->getHp() - 23);
                }
            } else {
                soundPlay(SOUND_EVADE);
            }
        }
    }
    return true;
} // spellTremor

static bool spellUndead(int)
{
    CombatController *ct = spellCombatController();
    CreatureVector creatures = ct->getMap()->getCreatures();
    CreatureVector::iterator i;
    for (i = creatures.begin(); i != creatures.end(); i++) {
        Creature *m = *i;
        if (m && m->isUndead()) {
            if (m->getHp() > 23) {
                m->setHp(23);
            }
        }
    }
    return true;
}

static bool spellView(int)
{
    peer(false);
    return true;
}

static bool spellWinds(int fromdir)
{
    c->windDirection = fromdir;
    return true;
}

static bool spellXit(int)
{
    if (!c->location->map->isWorldMap()) {
        /* CHANGE: can't cast in Hythloth - too easy */
        if (c->location->map->id == MAP_HYTHLOTH) {
            return false;
        }
        /*Otherwise it always works */
        screenMessage("Verlasse...\n");
        EventHandler::simulateDiskLoad(2000, false);
        game->exitToParentMap();
        musicMgr->play();
        return true;
    }
    return false;
}

static bool spellYup(int)
{
    MapCoords coords = c->location->coords;
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    /* can't cast in the Abyss CHANGE: or in Hythloth - too easy */
    if (c->location->map->id == MAP_ABYSS ||
        c->location->map->id == MAP_HYTHLOTH) {
        return false;
    }
    /* staying in the dungeon */
    else if (coords.z > 0) {
        for (int i = 0; i < 0x100; i++) {
            coords = MapCoords(
                xu4_random(8), xu4_random(8), c->location->coords.z - 1
            );
            if (dungeon->validTeleportLocation(coords)) {
                c->location->coords = coords;
                return true;
            }
        }
    }
    /* exiting the dungeon */
    else {
        screenMessage("Verlasse...\n");
        EventHandler::simulateDiskLoad(2000, false);
        game->exitToParentMap();
        musicMgr->play();
        return true;
    }
    /* didn't find a place to go, failed! */
    return false;
} // spellYup

static bool spellZdown(int)
{
    MapCoords coords = c->location->coords;
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    /* can't cast in the Abyss CHANGE: or in Hythloth - too easy */
    if (c->location->map->id == MAP_ABYSS ||
        c->location->map->id == MAP_HYTHLOTH) {
        return false;
    }
    /* can't go lower than level 8 */
    else if (coords.z >= 7) {
        return false;
    } else {
        for (int i = 0; i < 0x100; i++) {
            coords = MapCoords(
                xu4_random(8), xu4_random(8), c->location->coords.z + 1
            );
            if (dungeon->validTeleportLocation(coords)) {
                c->location->coords = coords;
                return true;
            }
        }
    }
    /* didn't find a place to go, failed! */
    return false;
} // spellZdown

const Spell *getSpell(int i)
{
    return &spells[i];
}
