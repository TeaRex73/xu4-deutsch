/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <limits>
#include <cstdlib>
#include <deque>
#include <vector>

#include "creature.h"

#include "aura.h"
#include "combat.h"
#include "config.h"
#include "context.h"
#include "coords.h"
#include "debug.h"
#include "direction.h"
#include "error.h"
#include "game.h" /* required by specialAction and specialEffect functions */
#include "location.h"
#include "map.h"
#include "movement.h"
#include "object.h"
#include "player.h" /* required by specialAction and specialEffect functions */
#include "savegame.h"
#include "screen.h" /* FIXME: remove dependence on this */
#include "settings.h"
#include "sound.h"
#include "textcolor.h" /* required to change color of screen message text */
#include "tile.h"
#include "tileset.h"
#include "types.h"
#include "utils.h"


CreatureMgr *CreatureMgr::instance = nullptr;

bool isCreature(Object *pUnknown)
{
    if (dynamic_cast<Creature *>(pUnknown) != nullptr) {
        return true;
    }
    return false;
}

/**
 * Creature class implementation
 */
Creature::Creature(MapTile tile)
    :Object(CREATURE),
     id(0),
     leader(0),
     base_hp(0),
     hp(0),
     status(STAT_GOOD),
     xp(0),
     ranged(0),
     leaves_tile(false),
     m_attr(),
     movement_attr(),
     slowed_type(SLOWED_BY_TILE),
     encounter_size(0),
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
        { .name = "undead", .mask = M_ATTR_UNDEAD },
        { .name = "good", .mask = M_ATTR_GOOD },
        { .name = "swims", .mask = M_ATTR_WATER },
        { .name = "sails", .mask = M_ATTR_WATER },
        { .name = "cantattack", .mask = M_ATTR_NON_ATTACKABLE },
        { .name = "camouflage", .mask = M_ATTR_CAMOUFLAGE },
        { .name = "wontattack", .mask = M_ATTR_NO_ATTACK },
        { .name = "ambushes", .mask = M_ATTR_AMBUSHES },
        { .name = "incorporeal", .mask = M_ATTR_INCORPOREAL },
        { .name = "nochest", .mask = M_ATTR_NO_CHEST },
        { .name = "divides", .mask = M_ATTR_DIVIDES },
        { .name = "forceOfNature", .mask = M_ATTR_FORCE_OF_NATURE }
    };
    /* steals="" */
    static constexpr struct {
        const char *name;
        unsigned int mask;
    } steals[] = {
        { .name = "food", .mask = M_ATTR_STEAL_FOOD },
        { .name = "gold", .mask = M_ATTR_STEAL_GOLD }
    };
    /* casts="" */
    static constexpr struct {
        const char *name;
        unsigned int mask;
    } casts[] = {
        { .name = "sleep", .mask = M_ATTR_CASTS_SLEEP },
        { .name = "negate", .mask = M_ATTR_NEGATE }
    };
    /* movement="" */
    static constexpr struct {
        const char *name;
        unsigned int mask;
    } movement[] = {
        { .name = "none", .mask = M_ATTR_STATIONARY },
        { .name = "wanders", .mask = M_ATTR_WANDERS }
    };
    /* boolean attributes that affect movement */
    static constexpr struct {
        const char *name;
        unsigned int mask;
    } movementBoolean[] = {
        { .name = "swims", .mask = M_ATTR_SWIMS },
        { .name = "sails", .mask = M_ATTR_SAILS },
        { .name = "flies", .mask = M_ATTR_FLIES },
        { .name = "teleports", .mask = M_ATTR_TELEPORT },
        { .name = "canMoveOntoCreatures", .mask = M_ATTR_CAN_MOVE_CREATURES },
        { .name = "canMoveOntoAvatar", .mask = M_ATTR_CAN_MOVE_AVATAR }
    };
    static constexpr struct {
        const char *name;
        TileEffect effect;
    } effects[] = {
        { .name = "fire", .effect = EFFECT_FIRE },
        { .name = "poison", .effect = EFFECT_POISON },
        { .name = "sleep", .effect = EFFECT_SLEEP }
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
    m_attr = static_cast<CreatureAttrib>(0);
    movement_attr = static_cast<CreatureMovementAttrib>(0);
    resists = 0;
    /* get the encounter size */
    encounter_size = conf.getInt("encounterSize", 0);
    /* get the base hp */
    base_hp = conf.getInt("basehp", 0);
    /* adjust basehp according to battle difficulty setting */
    if (settings.battleDiff == "Hard") {
        base_hp *= 2;
    }
    if (settings.battleDiff == "Expert") {
        base_hp *= 4;
    }
    /* get the camouflaged tile */
    if (conf.exists("camouflageTile")) {
        camouflage_tile = conf.getString("camouflageTile");
    }
    /* get the ranged tile for world map attacks */
    if (conf.exists("worldrangedtile")) {
        world_ranged_tile = conf.getString("worldrangedtile");
    }
    /* get ranged hit tile */
    if (conf.exists("rangedhittile")) {
        if (conf.getString("rangedhittile") == "random") {
            /* m_attr is still zero here, which cppcheck doesn't like */
            m_attr = M_ATTR_RANDOM_RANGED;
        } else {
            setHitTile(conf.getString("rangedhittile"));
        }
    }
    /* get ranged miss tile */
    if (conf.exists("rangedmisstile")) {
        if (conf.getString("rangedmisstile") == "random") {
            m_attr = static_cast<CreatureAttrib>(m_attr | M_ATTR_RANDOM_RANGED);
        } else {
            setMissTile(conf.getString("rangedmisstile"));
        }
    }
    /* find out if the creature leaves a tile behind on ranged attacks */
    leaves_tile = conf.getBool("leavestile");
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
            m_attr = static_cast<CreatureAttrib>(
                m_attr | booleanAttributes[idx].mask
            );
        }
    }
    /* Load boolean attributes that affect movement */
    for (idx = 0;
         idx < sizeof(movementBoolean) / sizeof(movementBoolean[0]);
         idx++) {
        if (conf.getBool(movementBoolean[idx].name)) {
            movement_attr = static_cast<CreatureMovementAttrib>(
                movement_attr | movementBoolean[idx].mask
            );
        }
    }
    /* steals="" */
    for (idx = 0; idx < sizeof(steals) / sizeof(steals[0]); idx++) {
        if (conf.getString("steals") == steals[idx].name) {
            m_attr = static_cast<CreatureAttrib>(m_attr | steals[idx].mask);
        }
    }
    /* casts="" */
    for (idx = 0; idx < sizeof(casts) / sizeof(casts[0]); idx++) {
        if (conf.getString("casts") == casts[idx].name) {
            m_attr = static_cast<CreatureAttrib>(m_attr | casts[idx].mask);
        }
    }
    /* movement="" */
    for (idx = 0; idx < sizeof(movement) / sizeof(movement[0]); idx++) {
        if (conf.getString("movement") == movement[idx].name) {
            movement_attr = static_cast<CreatureMovementAttrib>(
                movement_attr | movement[idx].mask
            );
        }
    }
    if (conf.exists("spawnsOnDeath")) {
        m_attr = static_cast<CreatureAttrib>(m_attr | M_ATTR_SPAWNS_ON_DEATH);
        spawn = static_cast<unsigned char>(
            conf.getInt("spawnsOnDeath")
        );
    }
    /* Figure out which 'slowed' function to use. */
    slowed_type = SLOWED_BY_TILE;
    if (sails()) {
        /* sailing creatures (pirate ships) */
        slowed_type = SLOWED_BY_WIND;
    } else if (flies() || isIncorporeal()) {
        /* flying creatures (dragons, bats, etc.) and
           incorporeal creatures (ghosts, zorns) */
        slowed_type = SLOWED_BY_NOTHING;
    }
} // Creature::load

bool Creature::isAttackable() const
{
    if (m_attr & M_ATTR_NON_ATTACKABLE) {
        return false;
    }
    /* can't attack horse transport */
    if (tile.getTileType()->isHorse()
        && getMovementBehavior() == MOVEMENT_FIXED) {
        return false;
    }
    return true;
}

int Creature::getDamage() const
{
    return xu4_random(base_hp >> 2);
}

int Creature::setInitialHp(const int points)
{
    if (points < 0) {
        hp = xu4_random(base_hp) | base_hp / 2;
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
        ranged_hit_tile = ranged_miss_tile = "poison_field";
        break;
    case 1:
        ranged_hit_tile = ranged_miss_tile = "energy_field";
        break;
    case 2:
        ranged_hit_tile = ranged_miss_tile = "fire_field";
        break;
    case 3:
        ranged_hit_tile = ranged_miss_tile = "sleep_field";
        break;
    default:
        errorFatal("BUG: xu4_random(4) output should be 0-3");
    }
}

CreatureState Creature::getState() const
{
    const int crit_threshold = base_hp >> 2;
    const int heavy_threshold = base_hp >> 1;
    const int light_threshold = crit_threshold + heavy_threshold;
    if (hp <= 0) {
        return M_STAT_DEAD;
    }
    if (hp < 24) {
        return M_STAT_FLEEING;
    }
    if (hp < crit_threshold) {
        return M_STAT_CRITICAL;
    }
    if (hp < heavy_threshold) {
        return M_STAT_HEAVILY_WOUNDED;
    }
    if (hp < light_threshold) {
        return M_STAT_LIGHTLY_WOUNDED;
    }
    return M_STAT_BARELY_WOUNDED;
} // Creature::getState


/**
 * Performs a special action for the creature
 * Returns true if the action takes up the creatures
 * whole turn (i.e. it can't move afterwords)
 */
bool Creature::specialAction() const
{
    bool retval = false;
    const int dx = std::abs(c->location->coords.x - coords.x);
    const int dy = std::abs(c->location->coords.y - coords.y);
    const int map_dist =
        c->location->coords.distance(coords, c->location->map);
    /* find out which direction the avatar is
       in relation to the creature */
    const MapCoords map_coords(coords);
    const int dir = map_coords.getRelativeDirection(
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
        if (map_dist <= 3
            && xu4_random(2) == 0
            && (c->location->context & CTX_CITY) == 0) {
            soundPlay(SOUND_NPC_ATTACK);
            const std::vector<Coords> path = gameGetDirectionalActionPath(
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
        if (((dx == 0 && dy <= 3) ||
             /* avatar is close enough and on the same row, AND */
             (dy == 0 && dx <= 3)) &&
            /* pirate ship is firing broadsides */
            (broadsidesDirs & dir) > 0) {
            // nothing (not even mountains!) can block cannonballs
            soundPlay(SOUND_NPC_ATTACK);
            const std::vector<Coords> path = gameGetDirectionalActionPath(
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
        if (coords == c->location->coords) {
            soundPlay(SOUND_STORM, false, -1, true);
            for (int j = 0; j < 4; j++) {
                c->party->applyEffect(EFFECT_FIRE);
            }
            return true;
        }
        /* See if the storm is on top of any objects and destroy them! */
        for (auto i = c->location->map->objects.begin();
             i != c->location->map->objects.end();
             /* nothing */ ) {
            obj = *i;
            if (this != obj && obj->getCoords() == coords) {
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
        if (coords == c->location->coords
            && c->transportContext == TRANSPORT_SHIP) {
            soundPlay(SOUND_WHIRLPOOL, false, -1, true);
            /* Deal 10 damage to the ship */
            c->party->applyEffect(EFFECT_FIRE);
            /* Send the party to Loch Lake */
            const MapCoords old_c = c->location->coords;
            c->location->coords = c->location->map->getLabel(
                "lockelake"
            );
            /* Teleport the whirlpool far away */
            int new_x = 128, new_y = 128;
            if (old_c.x >= 64 && old_c.x < 192) {
                new_x = 0;
            }
            if (old_c.y >= 64 && old_c.y < 192) {
                new_y = 0;
            }
            this->setCoords(Coords(new_x, new_y, 0));
            retval = true;
            break;
        }
        /* See if the whirlpool is on top of any objects and destroy them! */
        for (auto i = c->location->map->objects.begin();
             i != c->location->map->objects.end();
             /* nothing */ ) {
            obj = *i;
            if (this != obj && obj->getCoords() == coords) {
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

void Creature::act(const CombatController *controller)
{
    int dist;
    CombatAction action;
    bool harder;
    /* see if creature wakes up if it is asleep */
    if (getStatus() == STAT_SLEEPING && xu4_random(8) == 0) {
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
    if (teleports() && xu4_random(8) == 0) {
        action = CA_TELEPORT;
    }
    // creatures who ranged attack do so 1/4 of the time.  Make sure
    // their ranged attack is not negated!
    else if (ranged != 0
             && xu4_random(4) == 0
             && (ranged_hit_tile != "magic_flash"
                 || *c->aura != Aura::NEGATE)) {
        action = CA_RANGED;
    }
    // creatures who cast sleep do so 1/4 of the time they
    // don't ranged attack
    else if (castsSleep()
             && *c->aura != Aura::NEGATE
             && xu4_random(4) == 0) {
        action = CA_CAST_SLEEP;
    } else if (getState() == M_STAT_FLEEING) {
        action = CA_FLEE;
    }
    // default action: attack (or move towards) closest target
    else {
        action = CA_ATTACK;
    }
    /*
     * now find out who to do it to
     */
    Creature *target = nearestOpponent(&dist, action == CA_RANGED);
    if (target == nullptr) {
        return;
    }
    if (action == CA_ATTACK && dist > 1) {
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
        harder = *c->aura == Aura::PROTECTION;
        if (CombatController::attackHit(this, target, harder)) {
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
            if (stealsGold() && xu4_random(4) == 0) {
                // ITEM_STOLEN, gold
                soundPlay(SOUND_ITEM_STOLEN, false);
                c->party->adjustGold(-xu4_random(0x40));
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
            const PartyMemberVector party =
                controller->getMap()->getPartyMembers();
            for (auto *j: party) {
                if (xu4_random(2) == 0) {
                    j->putToSleep(true);
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
                if (firstTry && tile->getSpeed() != FAST) {
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
        const MapCoords m_coords = getCoords();
        const MapCoords p_coords = target->getCoords();
        // figure out which direction to fire the weapon
        const int dir = m_coords.getRelativeDirection(p_coords);
        // NPC_ATTACK, ranged
        soundPlay(SOUND_NPC_ATTACK, false);
        const std::vector<Coords> path = gameGetDirectionalActionPath(
            dir,
            MASK_DIR_ALL,
            m_coords,
            1,
            11,
            &Tile::canAttackOverTile,
            false
        );
        const bool hit = std::any_of(
            path.cbegin(),
            path.cend(),
            [&](const Coords &v) -> bool {
                return controller->rangedAttack(v, this);
            }
        );
        if (!hit && !path.empty()) {
            controller->rangedMiss(path[path.size() - 1], this);
        }
        break;
    }
    case CA_FLEE:
    case CA_ADVANCE:
    {
        Map *map = getMap();
        if (moveCombatObject(action, map, this, target->getCoords())) {
            const Coords coords = getCoords();
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
void Creature::addStatus(const StatusType s)
{
    const StatusType prev = status;
    if (prev == s) { /* same as before */
        return;
    }
    if ((prev == STAT_DEAD && s != STAT_DEAD) ||
        (prev == STAT_SLEEPING &&
         (s == STAT_POISONED || s == STAT_GOOD)) ||
        (prev == STAT_POISONED && s == STAT_GOOD)) {
        /* new status is "better" - do nothing */
        return;
    }
    status = s;
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

void Creature::applyTileEffect(const TileEffect effect)
{
    if (effect != EFFECT_NONE) {
        gameUpdateScreen();
        switch (effect) {
        case EFFECT_SLEEP:
            /* creature fell asleep! */
            if (resists != EFFECT_SLEEP && xu4_random(256) >= hp) {
                putToSleep();
            }
            break;
        case EFFECT_LAVA:
        case EFFECT_FIRE:
            /* deal 0 - 127 damage to the creature
               if it is not immune to fire damage */
            if (resists != EFFECT_FIRE && resists != EFFECT_LAVA) {
                applyDamage(xu4_random(128), false);
            }
            break;
        case EFFECT_POISON:
            /* deal 0 - 127 damage to the creature
               if it is not immune to poison field damage */
            if (resists != EFFECT_POISON) {
                wakeUp(); /* just to be fair - poison wakes up players too */
                applyDamage(xu4_random(128), false);
            }
            break;
        case EFFECT_SWAMP:
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
    const int dir_mask = map->getValidMoves(getCoords(), getTile());
    const Direction d = dirRandomDir(dir_mask);
    /* this is a game enhancement, make sure it's turned on! */
    if (!settings.enhancementsOptions.slimeDivides) {
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
        const int dividedHp = (this->hp + 1) / 2;
        addedCreature->hp = dividedHp;
        this->hp = dividedHp;
        return true;
    }
    return false;
} // Creature::divide

void Creature::spawnOnDeath() const
{
    Map *map = getMap();
    /* this is a game enhancement, make sure it's turned on! */
    if (!settings.enhancementsOptions.gazerSpawnsInsects) {
        return;
    }
    /* make sure there's a place to put the divided creature! */
    const MapCoords coords(getCoords());
    /* create our new creature! */
    map->addCreature(creatureMgr->getById(spawn), coords);
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
        if (dist < 5 && !isVisible()) {
            setVisible(); /* show yourself */
        } else if (dist >= 5) {
            setVisible(false); /* hide and take no action! */
        }
    }
    return isVisible();
}

Creature *Creature::nearestOpponent(int *dist, const bool ranged_attack)
{
    Creature *opponent = nullptr;
    int d, leastDist = std::numeric_limits<int>::max();
    const bool jinx = *c->aura == Aura::JINX;
    const Map *map = getMap();
    for (auto i = map->objects.cbegin(); i != map->objects.cend(); ++i) {
        if (!isCreature(*i)) {
            continue;
        }
        const bool amPlayer = isPartyMember(this);
        const bool fightingPlayer = isPartyMember(*i);
        /* if a party member, find a creature.
           If a creature, find a party member */
        /* if jinxed is false, find anything that isn't self */
        if (amPlayer != fightingPlayer
            || (jinx && !amPlayer && *i != this)) {
            MapCoords objCoords = (*i)->getCoords();
            /* if ranged, get the distance using diagonals,
               otherwise get movement distance */
            if (ranged_attack) {
                d = objCoords.distance(getCoords(), map);
            } else {
                d = objCoords.movementDistance(getCoords(), map);
            }
            if (d < leastDist) {
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

void Creature::removeStatus(const StatusType s)
{
    const StatusType prev = status;
    if (prev != s) {
        return;
    }
    status = STAT_GOOD;
    setAnimated(); /* animate creature */
}

void Creature::setStatus(const StatusType s)
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
bool Creature::applyDamage(const int damage, const bool by_player)
{
    /* deal the damage - LB is invulnerable */
    if (id != LORD_BRITISH_ID) {
        AdjustValueMin(hp, -damage, 0);
    }
    switch (getState()) {
    case M_STAT_DEAD:
        if (by_player) {
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
    case M_STAT_FLEEING:
        screenMessage(
            "\n%c%s\nAUF DER FLUCHT%c\n",
            FG_YELLOW,
            uppercase(name).c_str(),
            FG_WHITE
        );
        break;
    case M_STAT_CRITICAL:
        screenMessage("\n%s\nKRITISCH\n", uppercase(name).c_str());
        break;
    case M_STAT_HEAVILY_WOUNDED:
        screenMessage("\n%s\nSCHWER VERWUNDET\n", uppercase(name).c_str());
        break;
    case M_STAT_LIGHTLY_WOUNDED:
        screenMessage("\n%s\nLEICHT VERWUNDET\n", uppercase(name).c_str());
        break;
    case M_STAT_BARELY_WOUNDED:
        screenMessage("\n%s\nKAUM VERWUNDET\n", uppercase(name).c_str());
        break;
    } // switch
      /* creature is still alive and has the chance to
         divide - xu4 enhancement */
    if (divides() && xu4_random(2) == 0) {
        divide();
    }
    return true;
} // Creature::applyDamage

bool Creature::dealDamage(Creature *m, const int damage)
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
    const std::vector<ConfigElement> creatureConfs =
        config->getElement("creatures").getChildren();
    for (const auto &creatureConf: creatureConfs) {
        if (creatureConf.getName() != "creature") {
            continue;
        }
        auto *m = new Creature(0);
        m->load(creatureConf);
        /* add the creature to the list */
        creatures[m->getId()] = m;
    }
}


/**
 * Returns a creature using a tile to find which one to create
 * or nullptr if a creature with that tile cannot be found
 */
Creature *CreatureMgr::getByTile(MapTile tile) const
{
    const auto i = std::find_if(
        creatures.cbegin(),
        creatures.cend(),
        [&](const CreatureMap::value_type &v) -> bool {
            return v.second->getTile() == tile;
        }
    );
    if (i != creatures.cend()) {
        return i->second;
    }
    return nullptr;
}


/**
 * Returns the creature that has the corresponding id
 * or returns nullptr if no creature with that id could
 * be found.
 */
Creature *CreatureMgr::getById(const CreatureId id)
{
    const CreatureMap::const_iterator i = creatures.find(id);
    if (i != creatures.cend()) {
        return i->second;
    }
    return nullptr;
}


/**
 * Returns the creature that has the corresponding name
 * or returns nullptr if no creature can be found with
 * that name (case insensitive)
 */
Creature *CreatureMgr::getByName(const std::string &name) const
{
    const auto i = std::find_if(
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
    }
    return i->second;
}


/**
 * Creates a random creature based on the tile given
 */
Creature *CreatureMgr::randomForTile(const Tile *tile) const
{
    if (tile->spawnsSeaMonster()) {
        if (xu4_random(8) != 0) {
            return nullptr;
        }
        TileId randTile = creatures.find(PIRATE_ID)->second->getTile().getId();
        // Pirates are twice as likely as others
        const int tempRand = xu4_random(8);
        randTile += tempRand == 7 ? 0 : tempRand;
        return getByTile(randTile);
    }
    if (tile->spawnsLandMonster()) {
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
    return nullptr;
} // CreatureMgr::randomForTile


/**
 * Creates a random creature based on the dungeon level given
 */
Creature *CreatureMgr::randomForDungeon(const int dngLevel)
{
    const CreatureId monster = RAT_ID + dngLevel + xu4_random(4);
    if (monster == MIMIC_ID) {
        return nullptr;
    }
    return getById(monster);
}


/**
 * Creates a random ambushing creature
 */
Creature *CreatureMgr::randomAmbushing() const
{
    static int numAmbushingCreatures = -1;
    /* first, find out how many creatures exist that might ambush you */
    /* this is done only once */
    if (numAmbushingCreatures == -1) {
        numAmbushingCreatures = static_cast<int>(std::count_if(
            creatures.cbegin(),
            creatures.cend(),
            [&](const CreatureMap::value_type &v) -> bool {
                return v.second->ambushes();
            }
        ));
    }
    if (numAmbushingCreatures > 0) {
        /* now, randomly select one of them */
        const int randCreature = xu4_random(numAmbushingCreatures);
        int countAmbushingCreatures = 0;
        /* now, find the one we selected */
        for (const auto creature: creatures) {
            if (creature.second->ambushes()) {
                /* found the creature - return it! */
                if (countAmbushingCreatures == randCreature) {
                    return creature.second;
                }
                /* move on to the next creature */
                countAmbushingCreatures++;
            }
        }
    }
    U4ASSERT(0, "failed to find an ambushing creature");
    return nullptr;
} // CreatureMgr::randomAmbushing
