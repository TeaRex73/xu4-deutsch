/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "creature.h"
#include "combat.h"
#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "game.h" /* required by specialAction and specialEffect functions */
#include "location.h"
#include "map.h"
#include "player.h" /* required by specialAction and specialEffect functions */
#include "savegame.h"
#include "screen.h" /* FIXME: remove dependence on this */
#include "settings.h"
#include "textcolor.h" /* required to change color of screen message text */
#include "tileset.h"
#include "utils.h"

CreatureMgr *CreatureMgr::instance = nullptr;

bool isCreature(Object *punknown)
{
    if (dynamic_cast<Creature *>(punknown) != nullptr) {
        return true;
    } else {
        return false;
    }
}

/**
 * Creature class implementation
 */
Creature::Creature(MapTile tile)
    :Object(Object::CREATURE),
     name(),
     rangedhittile(),
     rangedmisstile(),
     id(0),
     camouflageTile(),
     leader(0),
     basehp(0),
     hp(0),
     status(STAT_GOOD),
     xp(0),
     ranged(0),
     worldrangedtile(),
     leavestile(false),
     mattr(),
     movementAttr(),
     slowedType(SLOWED_BY_TILE),
     encounterSize(0),
     resists(0),
     spawn(0)
{
    const Creature *m = creatureMgr->getByTile(tile);
    if (m) {
        *this = *m;
    }
}

void Creature::load(const ConfigElement &conf)
{
    unsigned int idx;
    static const struct {
        const char *name;
        unsigned int mask;
    } booleanAttributes[] = {
        { "undead", MATTR_UNDEAD },
        { "good", MATTR_GOOD },
        { "swims", MATTR_WATER },
        { "sails", MATTR_WATER },
        { "cantattack", MATTR_NONATTACKABLE },
        { "camouflage", MATTR_CAMOUFLAGE },
        { "wontattack", MATTR_NOATTACK },
        { "ambushes", MATTR_AMBUSHES },
        { "incorporeal", MATTR_INCORPOREAL },
        { "nochest", MATTR_NOCHEST },
        { "divides", MATTR_DIVIDES },
        { "forceOfNature", MATTR_FORCE_OF_NATURE }
    };
    /* steals="" */
    static const struct {
        const char *name;
        unsigned int mask;
    } steals[] = {
        { "food", MATTR_STEALFOOD },
        { "gold", MATTR_STEALGOLD }
    };
    /* casts="" */
    static const struct {
        const char *name;
        unsigned int mask;
    } casts[] = {
        { "sleep", MATTR_CASTS_SLEEP },
        { "negate", MATTR_NEGATE }
    };
    /* movement="" */
    static const struct {
        const char *name;
        unsigned int mask;
    } movement[] = {
        { "none", MATTR_STATIONARY },
        { "wanders", MATTR_WANDERS }
    };
    /* boolean attributes that affect movement */
    static const struct {
        const char *name;
        unsigned int mask;
    } movementBoolean[] = {
        { "swims", MATTR_SWIMS },
        { "sails", MATTR_SAILS },
        { "flies", MATTR_FLIES },
        { "teleports", MATTR_TELEPORT },
        { "canMoveOntoCreatures", MATTR_CANMOVECREATURES },
        { "canMoveOntoAvatar", MATTR_CANMOVEAVATAR }
    };
    static const struct {
        const char *name;
        TileEffect effect;
    } effects[] = {
        { "fire", EFFECT_FIRE },
        { "poison", EFFECT_POISONFIELD },
        { "sleep", EFFECT_SLEEP }
    };
    name = conf.getString("name");
    id = static_cast<unsigned short>(conf.getInt("id"));
    /* Get the leader if it's been included,
       otherwise the leader is itself */
    leader = static_cast<unsigned char>(conf.getInt("leader", id));
    xp = static_cast<unsigned short>(conf.getInt("exp"));
    ranged = conf.getBool("ranged");
    setTile(Tileset::findTileByName(conf.getString("tile")));
    setHitTile("hit_flash");
    setMissTile("miss_flash");
    mattr = static_cast<CreatureAttrib>(0);
    movementAttr = static_cast<CreatureMovementAttrib>(0);
    resists = 0;
    /* get the encounter size */
    encounterSize = conf.getInt("encounterSize", 0);
    /* get the base hp */
    basehp = conf.getInt("basehp", 0);
    /* adjust basehp according to battle difficulty setting */
    if (settings.battleDiff == "Hard") {
        basehp *= 2;
    }
    if (settings.battleDiff == "Expert") {
        basehp *= 4;
    }
    /* get the camouflaged tile */
    if (conf.exists("camouflageTile")) {
        camouflageTile = conf.getString("camouflageTile");
    }
    /* get the ranged tile for world map attacks */
    if (conf.exists("worldrangedtile")) {
        worldrangedtile = conf.getString("worldrangedtile");
    }
    /* get ranged hit tile */
    if (conf.exists("rangedhittile")) {
        if (conf.getString("rangedhittile") == "random") {
            /* mattr is still zero here, which cppcheck doesn't like */
            mattr =
                static_cast<CreatureAttrib>(/* mattr | */ MATTR_RANDOMRANGED);
        } else {
            setHitTile(conf.getString("rangedhittile"));
        }
    }
    /* get ranged miss tile */
    if (conf.exists("rangedmisstile")) {
        if (conf.getString("rangedmisstile") == "random") {
            mattr = static_cast<CreatureAttrib>(mattr | MATTR_RANDOMRANGED);
        } else {
            setMissTile(conf.getString("rangedmisstile"));
        }
    }
    /* find out if the creature leaves a tile behind on ranged attacks */
    leavestile = conf.getBool("leavestile");
    /* get effects that this creature is immune to */
    for (idx = 0; idx < sizeof(effects) / sizeof(effects[0]); idx++) {
        if (conf.getString("resists") == effects[idx].name) {
            resists = effects[idx].effect;
        }
    }
    /* Load creature attributes */
    for (idx = 0;
         idx < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]);
         idx++) {
        if (conf.getBool(booleanAttributes[idx].name)) {
            mattr = static_cast<CreatureAttrib>(
                mattr | booleanAttributes[idx].mask
            );
        }
    }
    /* Load boolean attributes that affect movement */
    for (idx = 0;
         idx < sizeof(movementBoolean) / sizeof(movementBoolean[0]);
         idx++) {
        if (conf.getBool(movementBoolean[idx].name)) {
            movementAttr = static_cast<CreatureMovementAttrib>(
                movementAttr | movementBoolean[idx].mask
            );
        }
    }
    /* steals="" */
    for (idx = 0; idx < sizeof(steals) / sizeof(steals[0]); idx++) {
        if (conf.getString("steals") == steals[idx].name) {
            mattr = static_cast<CreatureAttrib>(mattr | steals[idx].mask);
        }
    }
    /* casts="" */
    for (idx = 0; idx < sizeof(casts) / sizeof(casts[0]); idx++) {
        if (conf.getString("casts") == casts[idx].name) {
            mattr = static_cast<CreatureAttrib>(mattr | casts[idx].mask);
        }
    }
    /* movement="" */
    for (idx = 0; idx < sizeof(movement) / sizeof(movement[0]); idx++) {
        if (conf.getString("movement") == movement[idx].name) {
            movementAttr = static_cast<CreatureMovementAttrib>(
                movementAttr | movement[idx].mask
            );
        }
    }
    if (conf.exists("spawnsOnDeath")) {
        mattr = static_cast<CreatureAttrib>(mattr | MATTR_SPAWNSONDEATH);
        spawn = static_cast<unsigned char>(
            conf.getInt("spawnsOnDeath")
        );
    }
    /* Figure out which 'slowed' function to use. */
    slowedType = SLOWED_BY_TILE;
    if (sails()) {
        /* sailing creatures (pirate ships) */
        slowedType = SLOWED_BY_WIND;
    } else if (flies() || isIncorporeal()) {
        /* flying creatures (dragons, bats, etc.) and
           incorporeal creatures (ghosts, zorns) */
        slowedType = SLOWED_BY_NOTHING;
    }
} // Creature::load

bool Creature::isAttackable() const
{
    if (mattr & MATTR_NONATTACKABLE) {
        return false;
    }
    /* can't attack horse transport */
    if (tile.getTileType()->isHorse()
        && (getMovementBehavior() == MOVEMENT_FIXED)) {
        return false;
    }
    return true;
}

int Creature::getDamage() const
{
    int damage, val, x;
    val = basehp;
    x = xu4_random(val >> 2);
    damage = (x >> 4) * 10 + (x % 10); //Formula from U4DOS
    return damage;
}

int Creature::setInitialHp(int points)
{
    if (points < 0) {
        hp = xu4_random(basehp) | (basehp / 2);
    } else {
        hp = points;
    }
    /* make sure the creature doesn't flee initially */
    if (hp < 24) {
        hp = 24;
    }
    return hp;
}

void Creature::setRandomRanged()
{
    switch (xu4_random(4)) {
    case 0:
        rangedhittile = rangedmisstile = "poison_field";
        break;
    case 1:
        rangedhittile = rangedmisstile = "energy_field";
        break;
    case 2:
        rangedhittile = rangedmisstile = "fire_field";
        break;
    case 3:
        rangedhittile = rangedmisstile = "sleep_field";
        break;
    }
}

CreatureState Creature::getState() const
{
    int heavy_threshold, light_threshold, crit_threshold;
    crit_threshold = basehp >> 2;
    heavy_threshold = basehp >> 1;
    light_threshold = crit_threshold + heavy_threshold;
    if (hp <= 0) {
        return MSTAT_DEAD;
    } else if (hp < 24) {
        return MSTAT_FLEEING;
    } else if (hp < crit_threshold) {
        return MSTAT_CRITICAL;
    } else if (hp < heavy_threshold) {
        return MSTAT_HEAVILYWOUNDED;
    } else if (hp < light_threshold) {
        return MSTAT_LIGHTLYWOUNDED;
    } else {
        return MSTAT_BARELYWOUNDED;
    }
} // Creature::getState


/**
 * Performs a special action for the creature
 * Returns true if the action takes up the creatures
 * whole turn (i.e. it can't move afterwords)
 */
bool Creature::specialAction()
{
    bool retval = false;
    int dx = std::abs(c->location->coords.x - coords.x);
    int dy = std::abs(c->location->coords.y - coords.y);
    int mapdist = c->location->coords.distance(coords, c->location->map);
    /* find out which direction the avatar is
       in relation to the creature */
    MapCoords mapcoords(coords);
    int dir = mapcoords.getRelativeDirection(
        c->location->coords, c->location->map
    );
    // Init outside of switch
    int broadsidesDirs = 0;
    switch (id) {
    case LAVA_LIZARD_ID:
    case SEA_SERPENT_ID:
    case HYDRA_ID:
    case DRAGON_ID:
        /* A 50/50 chance they try to range attack when you're
           close enough and not in a city
           Note: Monsters in settlements in U3 do fire on party
        */
        if ((mapdist <= 3)
            && (xu4_random(2) == 0)
            && ((c->location->context & CTX_CITY) == 0)) {
            soundPlay(SOUND_NPC_ATTACK);
            std::vector<Coords> path = gameGetDirectionalActionPath(
                dir, MASK_DIR_ALL, coords, 1, 3, nullptr, false
            );
            static_cast<void>(
                std::any_of(
                    path.cbegin(),
                    path.cend(),
                    [&](const Coords &v) -> bool {
                        return creatureRangeAttack(v, this);
                    }
                )
            );
        }
        break;
    case PIRATE_ID:
        /* Fire cannon: Pirates only fire broadsides and
           only when they can hit you :) */
        retval = true;
        broadsidesDirs = dirGetBroadsidesDirs(tile.getDirection());
        /* avatar is close enough and on the same column, OR */
        if ((((dx == 0) && (dy <= 3)) ||
             /* avatar is close enough and on the same row, AND */
             ((dy == 0) && (dx <= 3))) &&
            /* pirate ship is firing broadsides */
            ((broadsidesDirs & dir) > 0)) {
            // nothing (not even mountains!) can block cannonballs
            soundPlay(SOUND_NPC_ATTACK);
            std::vector<Coords> path = gameGetDirectionalActionPath(
                dir, broadsidesDirs, coords, 1, 3, nullptr, false
            );
            static_cast<void>(
                std::any_of(
                    path.cbegin(),
                    path.cend(),
                    [&](const Coords &v) -> bool {
                        return fireAt(v, false);
                    }
                )
            );
        } else {
            retval = false;
        }
        break;
    default:
        break;
    } // switch
    return retval;
} // Creature::specialAction


/**
 * Performs a special effect for the creature
 * Returns true if something special happened,
 * or false if nothing happened
 */
bool Creature::specialEffect()
{
    Object *obj;
    bool retval = false;
    switch (id) {
    case STORM_ID:
    {
        ObjectDeque::iterator i;
        if (coords == c->location->coords) {
            soundPlay(SOUND_STORM, false, -1, true);
            for (int j = 0; j < 4; j++) {
                c->party->applyEffect(EFFECT_FIRE);
            }
            return true;
        }
        /* See if the storm is on top of any objects and destroy them! */
        for (i = c->location->map->objects.begin();
             i != c->location->map->objects.end();
             /* nothing */ ) {
            obj = *i;
            if ((this != obj) && (obj->getCoords() == coords)) {
                /* Converged with an object, destroy the object! */
                soundPlay(SOUND_NPC_STRUCK, false);
                i = c->location->map->removeObject(i);
                retval = true;
            } else {
                ++i;
            }
        }
        break;
    }
    case WHIRLPOOL_ID:
    {
        ObjectDeque::iterator i;
        if ((coords == c->location->coords)
            && (c->transportContext == TRANSPORT_SHIP)) {
            soundPlay(SOUND_WHIRLPOOL, false, -1, true);
            /* Deal 10 damage to the ship */
            c->party->applyEffect(EFFECT_FIRE);
            /* Send the party to Loch Lake */
            MapCoords old_c = c->location->coords;
            c->location->coords = c->location->map->getLabel(
                "lockelake"
            );
            /* Teleport the whirlpool far away */
            int newx = 128, newy = 128;
            if (old_c.x >= 64 && old_c.x < 192) {
                newx = 0;
            }
            if (old_c.y >= 64 && old_c.y < 192) {
                newy = 0;
            }
            this->setCoords(Coords(newx, newy, 0));
            retval = true;
            break;
        }
        /* See if the whirlpool is on top of any objects and destroy them! */
        for (i = c->location->map->objects.begin();
             i != c->location->map->objects.end();
             /* nothing */ ) {
            obj = *i;
            if ((this != obj) && (obj->getCoords() == coords)) {
                const Creature *m = dynamic_cast<Creature *>(obj);
                /* Make sure the object isn't a flying creature or object */
                if (!m || ((m->swims() || m->sails()) && !m->flies())) {
                    /* Destroy the object it met with */
                    soundPlay(SOUND_NPC_STRUCK, false);
                    i = c->location->map->removeObject(i);
                    retval = true;
                } else {
                    ++i;
                }
            } else {
                ++i;
            }
        }
                break;
    }
    default:
        break;
    } // switch
    return retval;
} // Creature::specialEffect

void Creature::act(CombatController *controller)
{
    int dist;
    CombatAction action;
    Creature *target;
    bool harder;
    /* see if creature wakes up if it is asleep */
    if ((getStatus() == STAT_SLEEPING) && (xu4_random(8) == 0)) {
        wakeUp();
    }
    /* if the creature is still asleep, then do nothing */
    if (getStatus() == STAT_SLEEPING) {
        return;
    }
    if (negates()) {
        c->aura->set(Aura::NEGATE, 2);
    }
    /*
     * figure out what to do
     */
    // creatures who teleport do so 1/8 of the time
    if (teleports() && (xu4_random(8) == 0)) {
        action = CA_TELEPORT;
    }
    // creatures who ranged attack do so 1/4 of the time.  Make sure
    // their ranged attack is not negated!
    else if ((ranged != 0)
             && (xu4_random(4) == 0)
             && ((rangedhittile != "magic_flash")
                 || (*c->aura != Aura::NEGATE))) {
        action = CA_RANGED;
    }
    // creatures who cast sleep do so 1/4 of the time they
    // don't ranged attack
    else if (castsSleep()
             && (*c->aura != Aura::NEGATE)
             && (xu4_random(4) == 0)) {
        action = CA_CAST_SLEEP;
    } else if (getState() == MSTAT_FLEEING) {
        action = CA_FLEE;
    }
    // default action: attack (or move towards) closest target
    else {
        action = CA_ATTACK;
    }
    /*
     * now find out who to do it to
     */
    target = nearestOpponent(&dist, action == CA_RANGED);
    if (target == nullptr) {
        return;
    }
    if ((action == CA_ATTACK) && (dist > 1)) {
        action = CA_ADVANCE;
    }
    /* let's see if the creature blends into the background, or if he
       appears... */
    if (camouflages() && !hideOrShow()) {
        return; /* creature is hidden -- no action! */
    }
    switch (action) {
    case CA_ATTACK:
        soundPlay(SOUND_NPC_ATTACK, false); // NPC_ATTACK, melee
        harder = (*c->aura == Aura::PROTECTION);
        if (controller->attackHit(this, target, harder)) {
            // PC_STRUCK, melee and ranged
            soundPlay(SOUND_PC_STRUCK, false);
            GameController::flashTile(target->getCoords(), "hit_flash", 4);
            dealDamage(target, getDamage());
        } else {
            GameController::flashTile(
                target->getCoords(), "miss_flash", 1
            );
        }
        // u4apple2: stealing happens even if the creature misses
        if (target && isPartyMember(target)) {
            /* steal gold if the creature steals gold */
            if (stealsGold() && (xu4_random(4) == 0)) {
                // ITEM_STOLEN, gold
                soundPlay(SOUND_ITEM_STOLEN, false);
                c->party->adjustGold(-(xu4_random(0x40)));
            }
            /* steal food if the creature steals food */
            if (stealsFood()) {
                // ITEM_STOLEN, food
                soundPlay(SOUND_ITEM_STOLEN, false);
                c->party->adjustFood(-2500);
            }
        }
        break;
    case CA_CAST_SLEEP:
        screenMessage("\nSLIPITUS!\n");
        /* show the sleep spell effect */
        gameSpellEffect('s', -1, SOUND_MAGIC);
        /* Apply the sleep spell to party members still in combat */
        if (!isPartyMember(this)) {
            PartyMemberVector party =
                controller->getMap()->getPartyMembers();
            PartyMemberVector::const_iterator j;
            for (j = party.cbegin(); j != party.cend(); ++j) {
                if (xu4_random(2) == 0) {
                    (*j)->putToSleep();
                }
            }
        }
        break;
    case CA_TELEPORT:
    {
        Coords new_c;
        bool valid = false;
        bool firstTry = true;
        while (!valid) {
            const Map *map = getMap();
            new_c = Coords(
                xu4_random(map->width),
                xu4_random(map->height),
                c->location->coords.z
            );
            const Tile *tile = map->tileTypeAt(new_c, WITH_OBJECTS);
            if (tile->isCreatureWalkable()) {
                /* If the tile would slow down, try again! */
                if (firstTry && (tile->getSpeed() != FAST)) {
                    firstTry = false;
                }
                /* OK, good enough! */
                else {
                    valid = true;
                }
            }
        }
        /* Teleport! */
        setCoords(new_c);
        break;
    }
    case CA_RANGED:
    {
        // if the creature has a random tile for a ranged weapon,
        // let's switch it now!
        if (hasRandomRanged()) {
            setRandomRanged();
        }
        MapCoords m_coords = getCoords(),
            p_coords = target->getCoords();
        // figure out which direction to fire the weapon
        int dir = m_coords.getRelativeDirection(p_coords);
        // NPC_ATTACK, ranged
        soundPlay(SOUND_NPC_ATTACK, false);
        std::vector<Coords> path = gameGetDirectionalActionPath(
            dir,
            MASK_DIR_ALL,
            m_coords,
            1,
            11,
            &Tile::canAttackOverTile,
            false
        );
        bool hit = std::any_of(
            path.cbegin(),
            path.cend(),
            [&](const Coords &v) -> bool {
                return controller->rangedAttack(v, this);
            }
        );
        if (!hit && (path.size() > 0)) {
            controller->rangedMiss(path[path.size() - 1], this);
        }
        break;
    }
    case CA_FLEE:
    case CA_ADVANCE:
    {
        Map *map = getMap();
        if (moveCombatObject(action, map, this, target->getCoords())) {
            Coords coords = getCoords();
            if (MAP_IS_OOB(map, coords)) {
                screenMessage(
                    "\n%c%s\nFLIEHT%c\n",
                    FG_YELLOW,
                    uppercase(name).c_str(),
                    FG_WHITE
                );
                /* Congrats, you have a heart! */
                if (isGood()) {
                    c->party->adjustKarma(KA_SPARED_GOOD);
                }
                soundPlay(SOUND_FLEE, false);
                map->removeObject(this);
                return;
            }
        }
        break;
    }
    } // switch
    this->animateMovement();
} // Creature::act


/**
 * Add status effects to the creature, in order of importance
 */
void Creature::addStatus(StatusType s)
{
    StatusType prev = status;
    if (prev == s) { /* same as before */
        return;
    }
    if ((prev == STAT_DEAD && s != STAT_DEAD) ||
        (prev == STAT_SLEEPING &&
         (s == STAT_POISONED || s == STAT_GOOD)) ||
        (prev == STAT_POISONED && s == STAT_GOOD)) {
        /* new status is "better" - do nothing */
        return;
    } else {
        status = s;
    }
    switch (status) {
    case STAT_GOOD:
    case STAT_POISONED:
        setAnimated(); /* animate creature */
        break;
    case STAT_SLEEPING:
    case STAT_DEAD:
        setAnimated(false); /* freeze creature */
        break;
    default:
        U4ASSERT(
            0,
            "Invalid status %d in Creature::addStatus",
            static_cast<int>(status)
        );
    }
}

void Creature::applyTileEffect(TileEffect effect)
{
    if (effect != EFFECT_NONE) {
        gameUpdateScreen();
        switch (effect) {
        case EFFECT_SLEEP:
            /* creature fell asleep! */
            if ((resists != EFFECT_SLEEP) && (xu4_random(256) >= hp)) {
                putToSleep();
            }
            break;
        case EFFECT_LAVA:
        case EFFECT_FIRE:
            /* deal 0 - 127 damage to the creature
               if it is not immune to fire damage */
            if ((resists != EFFECT_FIRE) && (resists != EFFECT_LAVA)) {
                applyDamage(xu4_random(128), false);
            }
            break;
        case EFFECT_POISONFIELD:
            /* deal 0 - 127 damage to the creature
               if it is not immune to poison field damage */
            if (resists != EFFECT_POISONFIELD) {
                wakeUp(); /* just to be fair - poison wakes up players too */
                applyDamage(xu4_random(128), false);
            }
            break;
        case EFFECT_POISON:
            /* "Normal" poison from swamps doesn't affect creatures */
        default:
            break;
        } // switch
    }
} // Creature::applyTileEffect

int Creature::getAttackBonus() const
{
    return 1;
}

int Creature::getDefense(bool) const
{
    return 0;
}

bool Creature::divide()
{
    Map *map = getMap();
    int dirmask = map->getValidMoves(getCoords(), getTile());
    Direction d = dirRandomDir(dirmask);
    /* this is a game enhancement, make sure it's turned on! */
    if (!settings.enhancements
        || !settings.enhancementsOptions.slimeDivides) {
        return false;
    }
    /* make sure there's a place to put the divided creature! */
    if (d != DIR_NONE) {
        MapCoords coords(getCoords());
        screenMessage("\n%s\nTEILT SICH\n", uppercase(name).c_str());
        /* find a spot to put our new creature */
        coords.move(d, map);
        /* create our new creature! */
        Creature *addedCreature = map->addCreature(this, coords);
        int dividedHp = (this->hp + 1) / 2;
        addedCreature->hp = dividedHp;
        this->hp = dividedHp;
        return true;
    }
    return false;
} // Creature::divide

bool Creature::spawnOnDeath()
{
    Map *map = getMap();
    /* this is a game enhancement, make sure it's turned on! */
    if (!settings.enhancements
        || !settings.enhancementsOptions.gazerSpawnsInsects) {
        return false;
    }
    /* make sure there's a place to put the divided creature! */
    MapCoords coords(getCoords());
    /* create our new creature! */
    map->addCreature(creatureMgr->getById(spawn), coords);
    return true;
}

StatusType Creature::getStatus() const
{
    return status;
}

bool Creature::isAsleep() const
{
    return status == STAT_SLEEPING;
}

/**
 * Hides or shows a camouflaged creature, depending on its distance from
 * the nearest opponent
 */
bool Creature::hideOrShow()
{
    /* find the nearest opponent */
    int dist;
    /* ok, now we've got the nearest party member.
       Now, see if they're close enough */
    if (nearestOpponent(&dist, false) != nullptr) {
        if ((dist < 5) && !isVisible()) {
            setVisible(); /* show yourself */
        } else if (dist >= 5) {
            setVisible(false); /* hide and take no action! */
        }
    }
    return isVisible();
}

Creature *Creature::nearestOpponent(int *dist, bool ranged)
{
    Creature *opponent = nullptr;
    int d, leastDist = 0xFFFF;
    ObjectDeque::const_iterator i;
    bool jinx = (*c->aura == Aura::JINX);
    Map *map = getMap();
    for (i = map->objects.cbegin(); i != map->objects.cend(); ++i) {
        if (!isCreature(*i)) {
            continue;
        }
        bool amPlayer = isPartyMember(this);
        bool fightingPlayer = isPartyMember(*i);
        /* if a party member, find a creature.
           If a creature, find a party member */
        /* if jinxed is false, find anything that isn't self */
        if ((amPlayer != fightingPlayer)
            || (jinx && !amPlayer && (*i != this))) {
            MapCoords objCoords = (*i)->getCoords();
            /* if ranged, get the distance using diagonals,
               otherwise get movement distance */
            if (ranged) {
                d = objCoords.distance(getCoords());
            } else {
                d = objCoords.movementDistance(getCoords());
            }
            /* skip target 50% of time if same distance */
            if ((d < leastDist)
               /* || ((d == leastDist) && (xu4_random(2) == 0)) */ ) {
                opponent = dynamic_cast<Creature *>(*i);
                leastDist = d;
            }
        }
    }
    if (opponent) {
        *dist = leastDist;
    }
    return opponent;
} // Creature::nearestOpponent

void Creature::putToSleep(bool)
{
    addStatus(STAT_SLEEPING);
}

void Creature::removeStatus(StatusType s)
{
    StatusType prev = status;
    if (prev != s) {
        return;
    } else {
        status = STAT_GOOD;
    }
    setAnimated(); /* animate creature */
}

void Creature::setStatus(StatusType s)
{
    status = s;
}

void Creature::wakeUp()
{
    removeStatus(STAT_SLEEPING);
}

/**
 * Applies damage to the creature.
 * Returns true if the creature still exists after the damage has been applied
 * or false, if the creature was destroyed
 *
 * If byplayer is false (when a monster is killed by walking through
 * fire or poison, or as a result of jinx) we don't report experience
 * on death
 */
bool Creature::applyDamage(int damage, bool byplayer)
{
    /* deal the damage - LB is invulnerable */
    if (id != LORDBRITISH_ID) {
        AdjustValueMin(hp, -damage, 0);
    }
    switch (getState()) {
    case MSTAT_DEAD:
        if (byplayer) {
            screenMessage(
                "\n%c%s\nGET\\TET%c\nERF.+%d\n",
                FG_RED,
                uppercase(name).c_str(),
                FG_WHITE,
                xp
            );
        } else {
            screenMessage(
                "\n%c%s\nGET\\TET%c\n",
                FG_RED,
                uppercase(name).c_str(),
                FG_WHITE
            );
        }
        /*
         * the creature is dead; let it spawns something else on death
         * (e.g. a gazer that spawns insects like in u5) then remove it
         */
        if (spawnsOnDeath()) {
            spawnOnDeath();
        }
        // Remove yourself from the map
        remove();
        return false;
    case MSTAT_FLEEING:
        screenMessage(
            "\n%c%s\nAUF DER FLUCHT%c\n",
            FG_YELLOW,
            uppercase(name).c_str(),
            FG_WHITE
        );
        break;
    case MSTAT_CRITICAL:
        screenMessage("\n%s\nKRITISCH\n", uppercase(name).c_str());
        break;
    case MSTAT_HEAVILYWOUNDED:
        screenMessage("\n%s\nSCHWER VERWUNDET\n", uppercase(name).c_str());
        break;
    case MSTAT_LIGHTLYWOUNDED:
        screenMessage("\n%s\nLEICHT VERWUNDET\n", uppercase(name).c_str());
        break;
    case MSTAT_BARELYWOUNDED:
        screenMessage("\n%s\nKAUM VERWUNDET\n", uppercase(name).c_str());
        break;
    } // switch
      /* creature is still alive and has the chance to
         divide - xu4 enhancement */
    if (divides() && (xu4_random(2) == 0)) {
        divide();
    }
    return true;
} // Creature::applyDamage

bool Creature::dealDamage(Creature *m, int damage)
{
    return m->applyDamage(damage, isPartyMember(this));
}


/**
 * CreatureMgr class implementation
 */
CreatureMgr *CreatureMgr::getInstance()
{
    if (instance == nullptr) {
        instance = new CreatureMgr();
        instance->loadAll();
    }
    return instance;
}

CreatureMgr::~CreatureMgr()
{
    creatures.clear();
}

void CreatureMgr::loadAll()
{
    const Config *config = Config::getInstance();
    std::vector<ConfigElement> creatureConfs =
        config->getElement("creatures").getChildren();
    for (std::vector<ConfigElement>::const_iterator i = creatureConfs.cbegin();
         i != creatureConfs.cend();
         ++i) {
        if (i->getName() != "creature") {
            continue;
        }
        Creature *m = new Creature(0);
        m->load(*i);
        /* add the creature to the list */
        creatures[m->getId()] = m;
    }
}


/**
 * Returns a creature using a tile to find which one to create
 * or nullptr if a creature with that tile cannot be found
 */
Creature *CreatureMgr::getByTile(MapTile tile)
{
    CreatureMap::const_iterator i = std::find_if(
        creatures.cbegin(),
        creatures.cend(),
        [&](const CreatureMap::value_type &v) -> bool {
            return v.second->getTile() == tile;
        }
    );
    if (i != creatures.cend()) {
        return i->second;
    } else {
        return nullptr;
    }
}


/**
 * Returns the creature that has the corresponding id
 * or returns nullptr if no creature with that id could
 * be found.
 */
Creature *CreatureMgr::getById(CreatureId id)
{
    CreatureMap::const_iterator i = creatures.find(id);
    if (i != creatures.cend()) {
        return i->second;
    } else {
        return nullptr;
    }
}


/**
 * Returns the creature that has the corresponding name
 * or returns nullptr if no creature can be found with
 * that name (case insensitive)
 */
Creature *CreatureMgr::getByName(const std::string &name)
{
    CreatureMap::const_iterator i = std::find_if(
        creatures.cbegin(),
        creatures.cend(),
        [&](const CreatureMap::value_type &v) -> bool {
            return !xu4_strcasecmp(
                deumlaut(v.second->getName()).c_str(),
                deumlaut(name).c_str()
            );
        }
    );
    if (i == creatures.cend()) {
        return nullptr;
    } else {
        return i->second;
    }
}


/**
 * Creates a random creature based on the tile given
 */
Creature *CreatureMgr::randomForTile(const Tile *tile)
{
    if (tile->spawnsSeaMonster()) {
        if (xu4_random(8) != 0) {
            return nullptr;
        }
        TileId randTile = creatures.find(PIRATE_ID)->second->getTile().getId();
        // Pirates are twice as likely as others
        int tempRand = xu4_random(8);
        randTile += (tempRand == 7 ? 0 : tempRand);
        return getByTile(randTile);
    }
    else if (tile->spawnsLandMonster()) {
        int era;
        if (c->saveGame->moves >= 30000) {
            era = 0x0f;
        } else if (c->saveGame->moves >= 10000) {
            era = 0x07;
        } else {
            era = 0x03;
        }
        TileId randTile = creatures.find(ORC_ID)->second->getTile().getId();
        randTile += era & xu4_random(0x10) & xu4_random(0x10);
        return getByTile(randTile);
    }
    else return nullptr;
} // CreatureMgr::randomForTile


/**
 * Creates a random creature based on the dungeon level given
 */
Creature *CreatureMgr::randomForDungeon(int dngLevel)
{
    CreatureId monster = RAT_ID + dngLevel + xu4_random(4);
    if (monster == MIMIC_ID) {
        return nullptr;
    }
    return getById(monster);
}


/**
 * Creates a random ambushing creature
 */
Creature *CreatureMgr::randomAmbushing()
{
    CreatureMap::const_iterator i;
    static int numAmbushingCreatures = -1;
    /* first, find out how many creatures exist that might ambush you */
    /* this is done only once */
    if (numAmbushingCreatures == -1) {
        numAmbushingCreatures = std::count_if(
            creatures.cbegin(),
            creatures.cend(),
            [&](const CreatureMap::value_type &v) -> bool {
                return v.second->ambushes();
            }
        );
    }
    if (numAmbushingCreatures > 0) {
        /* now, randomly select one of them */
        int randCreature = xu4_random(numAmbushingCreatures);
        int countAmbushingCreatures = 0;
        /* now, find the one we selected */
        for (i = creatures.cbegin(); i != creatures.cend(); ++i) {
            if (i->second->ambushes()) {
                /* found the creature - return it! */
                if (countAmbushingCreatures == randCreature) {
                    return i->second;
                }
                /* move on to the next creature */
                else {
                    countAmbushingCreatures++;
                }
            }
        }
    }
    U4ASSERT(0, "failed to find an ambushing creature");
    return nullptr;
} // CreatureMgr::randomAmbushing
