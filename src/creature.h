/**
 * $Id$
 */

#ifndef CREATURE_H
#define CREATURE_H

#include <map>
#include <string>
#include <vector>

#include "movement.h"
#include "object.h"
#include "savegame.h"
#include "types.h"

class CombatController;
class ConfigElement;
class Creature;
class Tile;

typedef unsigned short CreatureId;
typedef std::map<CreatureId, Creature *> CreatureMap;
typedef std::vector<Creature *> CreatureVector;

#define MAX_CREATURES 128
/* Creatures on world map */

#define MAX_CREATURES_ON_MAP 4
#define MAX_CREATURE_DISTANCE 24

/* Creature ids */
typedef enum {
    HORSE1_ID = 0,
    HORSE2_ID = 1,
    MAGE_ID = 2,
    BARD_ID = 3,
    FIGHTER_ID = 4,
    DRUID_ID = 5,
    TINKER_ID = 6,
    PALADIN_ID = 7,
    RANGER_ID = 8,
    SHEPHERD_ID = 9,
    GUARD_ID = 10,
    VILLAGER_ID = 11,
    SINGING_BARD_ID = 12,
    JESTER_ID = 13,
    BEGGAR_ID = 14,
    CHILD_ID = 15,
    BULL_ID = 16,
    LORD_BRITISH_ID = 17,
    PIRATE_ID = 18,
    NIXIE_ID = 19,
    GIANT_SQUID_ID = 20,
    SEA_SERPENT_ID = 21,
    SEAHORSE_ID = 22,
    WHIRLPOOL_ID = 23,
    STORM_ID = 24,
    RAT_ID = 25,
    BAT_ID = 26,
    GIANT_SPIDER_ID = 27,
    GHOST_ID = 28,
    SLIME_ID = 29,
    TROLL_ID = 30,
    GREMLIN_ID = 31,
    MIMIC_ID = 32,
    REAPER_ID = 33,
    INSECT_SWARM_ID = 34,
    GAZER_ID = 35,
    PHANTOM_ID = 36,
    ORC_ID = 37,
    SKELETON_ID = 38,
    ROGUE_ID = 39,
    PYTHON_ID = 40,
    ETTIN_ID = 41,
    HEADLESS_ID = 42,
    CYCLOPS_ID = 43,
    WISP_ID = 44,
    EVIL_MAGE_ID = 45,
    LICH_ID = 46,
    LAVA_LIZARD_ID = 47,
    ZORN_ID = 48,
    DAEMON_ID = 49,
    HYDRA_ID = 50,
    DRAGON_ID = 51,
    BALRON_ID = 52,
    MAX_CREATURE_ID = BALRON_ID
} CreatureType;

typedef enum {
    M_ATTR_STEAL_FOOD = 0x1,
    M_ATTR_STEAL_GOLD = 0x2,
    M_ATTR_CASTS_SLEEP = 0x4,
    M_ATTR_UNDEAD = 0x8,
    M_ATTR_GOOD = 0x10,
    M_ATTR_WATER = 0x20,
    M_ATTR_NON_ATTACKABLE = 0x40,
    M_ATTR_NEGATE = 0x80,
    M_ATTR_CAMOUFLAGE = 0x100,
    M_ATTR_NO_ATTACK = 0x200,
    M_ATTR_AMBUSHES = 0x400,
    M_ATTR_RANDOM_RANGED = 0x800,
    M_ATTR_INCORPOREAL = 0x1000,
    M_ATTR_NO_CHEST = 0x2000,
    M_ATTR_DIVIDES = 0x4000,
    M_ATTR_SPAWNS_ON_DEATH = 0x8000,
    M_ATTR_FORCE_OF_NATURE = 0x10000
} CreatureAttrib;

typedef enum {
    M_ATTR_STATIONARY = 0x1,
    M_ATTR_WANDERS = 0x2,
    M_ATTR_SWIMS = 0x4,
    M_ATTR_SAILS = 0x8,
    M_ATTR_FLIES = 0x10,
    M_ATTR_TELEPORT = 0x20,
    M_ATTR_CAN_MOVE_CREATURES = 0x40,
    M_ATTR_CAN_MOVE_AVATAR = 0x80
} CreatureMovementAttrib;

typedef enum {
    M_STAT_DEAD,
    M_STAT_FLEEING,
    M_STAT_CRITICAL,
    M_STAT_HEAVILY_WOUNDED,
    M_STAT_LIGHTLY_WOUNDED,
    M_STAT_BARELY_WOUNDED
} CreatureState;


/**
 * Creature Class Definition
 * @todo
 * <ul>
 *      <li>split into a CreatureType (all the settings for a
 *      particular creature e.g. orc) and Creature (a specific
 *      creature instance)</li>
 *      <li>creatures can be looked up by name,
 *      ids can probably go away</li>
 * </ul>
 */
class Creature:public Object {
public:
    explicit Creature(MapTile tile = MapTile(0));
    Creature(const Creature &c) = default;
    Creature(Creature &&c) = default;
    Creature &operator=(const Creature &c) = default;
    Creature &operator=(Creature &&c) = default;
    ~Creature() override = default;
    void load(const ConfigElement &conf);

    virtual std::string getName() const
    {
        return name;
    }

    virtual const std::string &getHitTile() const
    {
        return ranged_hit_tile;
    }

    virtual const std::string &getMissTile() const
    {
        return ranged_miss_tile;
    }

    CreatureId getId() const
    {
        return id;
    }

    CreatureId getLeader() const
    {
        return leader;
    }

    virtual int getHp() const
    {
        return hp;
    }

    virtual int getXp() const
    {
        return xp;
    }

    virtual const std::string &getWorldRangedTile() const
    {
        return world_ranged_tile;
    }

    SlowedType getSlowedType() const
    {
        return slowed_type;
    }

    int getEncounterSize() const
    {
        return encounter_size;
    }

    unsigned char getResists() const
    {
        return resists;
    }

    void setName(const std::string &s)
    {
        name = s;
    }

    void setHitTile(const std::string &t)
    {
        ranged_hit_tile = t;
    }

    void setMissTile(const std::string &t)
    {
        ranged_miss_tile = t;
    }

    virtual void setHp(const int points)
    {
        hp = points;
    }

    bool isGood() const
    {
        return m_attr & M_ATTR_GOOD;
    }

    bool isEvil() const
    {
        return !isGood();
    }

    bool isUndead() const
    {
        return m_attr & M_ATTR_UNDEAD;
    }

    bool leavesChest() const
    {
        return !isAquatic() && !(m_attr & M_ATTR_NO_CHEST);
    }

    bool isAquatic() const
    {
        return m_attr & M_ATTR_WATER;
    }

    bool wanders() const
    {
        return movement_attr & M_ATTR_WANDERS;
    }

    bool isStationary() const
    {
        return movement_attr & M_ATTR_STATIONARY;
    }

    bool flies() const
    {
        return movement_attr & M_ATTR_FLIES;
    }

    bool teleports() const
    {
        return movement_attr & M_ATTR_TELEPORT;
    }

    bool swims() const
    {
        return movement_attr & M_ATTR_SWIMS;
    }

    bool sails() const
    {
        return movement_attr & M_ATTR_SAILS;
    }

    bool walks() const
    {
        return !(flies() || swims() || sails());
    }

    bool divides() const
    {
        return m_attr & M_ATTR_DIVIDES;
    }

    bool spawnsOnDeath() const
    {
        return m_attr & M_ATTR_SPAWNS_ON_DEATH;
    }

    bool canMoveOntoCreatures() const
    {
        return movement_attr & M_ATTR_CAN_MOVE_CREATURES;
    }

    bool canMoveOntoPlayer() const
    {
        return movement_attr & M_ATTR_CAN_MOVE_AVATAR;
    }

    bool isAttackable() const;

    bool willAttack() const
    {
        return !(m_attr & M_ATTR_NO_ATTACK);
    }

    bool stealsGold() const
    {
        return m_attr & M_ATTR_STEAL_GOLD;
    }

    bool stealsFood() const
    {
        return m_attr & M_ATTR_STEAL_FOOD;
    }

    bool negates() const
    {
        return m_attr & M_ATTR_NEGATE;
    }

    bool camouflages() const
    {
        return m_attr & M_ATTR_CAMOUFLAGE;
    }

    bool ambushes() const
    {
        return m_attr & M_ATTR_AMBUSHES;
    }

    bool isIncorporeal() const
    {
        return m_attr & M_ATTR_INCORPOREAL;
    }

    bool hasRandomRanged() const
    {
        return m_attr & M_ATTR_RANDOM_RANGED;
    }

    bool leavesTile() const
    {
        return leaves_tile;
    }

    bool castsSleep() const
    {
        return m_attr & M_ATTR_CASTS_SLEEP;
    }

    bool isForceOfNature() const
    {
        return m_attr & M_ATTR_FORCE_OF_NATURE;
    }

    virtual int getDamage() const;

    const std::string &getCamouflageTile() const
    {
        return camouflage_tile;
    }

    void putToSleep()
    {
        putToSleep(true);
    }

    bool applyDamage(const int damage)
    {
        return applyDamage(damage, true);
    }

    void setRandomRanged();
    int setInitialHp(int points = -1);
    bool specialAction() const;
    bool specialEffect();
    /* combat methods */
    void act(const CombatController *controller);
    virtual void addStatus(StatusType s);
    void applyTileEffect(TileEffect effect);
    virtual int getAttackBonus() const;
    virtual int getDefense(bool needsMystic) const;
    bool divide();

    void spawnOnDeath() const;
    virtual CreatureState getState() const;
    StatusType getStatus() const;
    bool isAsleep() const;
    bool hideOrShow();
    Creature *nearestOpponent(int *dist, bool ranged_attack);
    virtual void putToSleep(bool sound);
    virtual void removeStatus(StatusType s);
    virtual void setStatus(StatusType s);
    virtual void wakeUp();
    virtual bool applyDamage(int damage, bool by_player);
    virtual bool dealDamage(Creature *m, int damage);

protected:
    std::string name;
    std::string ranged_hit_tile;
    std::string ranged_miss_tile;
    CreatureId id;
    std::string camouflage_tile;
    CreatureId leader;
    int base_hp;
    int hp;
    StatusType status;
    int xp;
    unsigned char ranged;
    std::string world_ranged_tile;
    bool leaves_tile;
    CreatureAttrib m_attr;
    CreatureMovementAttrib movement_attr;
    SlowedType slowed_type;
    int encounter_size;
    unsigned char resists;
    CreatureId spawn;
};


/**
 * CreatureMgr Class Definition
 */
class CreatureMgr {
public:
    // disallow assignments, copy construction
    CreatureMgr(const CreatureMgr &) = delete;
    CreatureMgr(CreatureMgr &&) = delete;
    CreatureMgr &operator=(const CreatureMgr &) = delete;
    CreatureMgr &operator=(CreatureMgr &&) = delete;

    ~CreatureMgr();
    static CreatureMgr *getInstance();
    void loadAll();
    Creature *getByTile(MapTile tile) const;
    Creature *getById(CreatureId id);
    Creature *getByName(const std::string &name) const;
    Creature *randomForTile(const Tile *tile) const;
    Creature *randomForDungeon(int dngLevel);
    Creature *randomAmbushing() const;

private:
    CreatureMgr()
    {
    }

    static CreatureMgr *instance;
    CreatureMap creatures;
};

bool isCreature(Object *pUnknown);

#define creatureMgr (CreatureMgr::getInstance())

#endif // CREATURE_H
