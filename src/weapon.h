/*
 * $Id$
 */

#ifndef WEAPON_H
#define WEAPON_H

#include <string>
#include <vector>

class ConfigElement;
enum ClassType: unsigned char;
enum WeaponType: unsigned short;


class Weapon {
public:
    enum Flags: unsigned short {
        WEAPON_LOSE = 0x0001,
        WEAPON_LOSE_WHEN_RANGED = 0x0002,
        WEAPON_CHOOSE_DISTANCE = 0x0004,
        WEAPON_ALWAYS_HITS = 0x0008,
        WEAPON_MAGIC = 0x0010,
        WEAPON_ATTACK_THROUGH_OBJS = 0x0040,
        WEAPON_ABSOLUTE_RANGE = 0x0080,
        WEAPON_RETURNS = 0x0100,
        WEAPON_DONT_SHOW_TRAVEL = 0x0200,
        WEAPON_RANGED_ONLY = 0x0400,
        WEAPON_MYSTIC = 0x0800
    };

    static void cleanup();

    static const Weapon *get(WeaponType w);
    static const Weapon *get(const std::string &name);

    WeaponType getType() const
    {
        return type;
    }

    const std::string &getName() const
    {
        return name;
    }

    const std::string &getAbbrev() const
    {
        return abbr;
    }

    const std::string &getNeg() const
    {
        return neg;
    }

    bool canReady(const ClassType klass) const
    {
        return (canUse & (1 << klass)) != 0;
    }

    int getRange() const
    {
        return range;
    }

    int getDamage() const
    {
        return damage;
    }

    const std::string &getHitTile() const
    {
        return hitTile;
    }

    const std::string &getMissTile() const
    {
        return missTile;
    }

    const std::string &leavesTile() const
    {
        return leaveTile;
    }

    unsigned short getFlags() const
    {
        return flags;
    }

    bool loseWhenUsed() const
    {
        return flags & WEAPON_LOSE;
    }

    bool loseWhenRanged() const
    {
        return flags & WEAPON_LOSE_WHEN_RANGED;
    }

    bool canChooseDistance() const
    {
        return flags & WEAPON_CHOOSE_DISTANCE;
    }

    bool alwaysHits() const
    {
        return flags & WEAPON_ALWAYS_HITS;
    }

    bool isMagic() const
    {
        return flags & WEAPON_MAGIC;
    }

    bool rangedOnly() const
    {
        return flags & WEAPON_RANGED_ONLY;
    }

    bool isMystic() const
    {
        return flags & WEAPON_MYSTIC;
    }

    bool canAttackThroughObjects() const
    {
        return flags & WEAPON_ATTACK_THROUGH_OBJS;
    }

    bool rangeAbsolute() const
    {
        return flags & WEAPON_ABSOLUTE_RANGE;
    }

    bool returns() const
    {
        return flags & WEAPON_RETURNS;
    }

    bool showTravel() const
    {
        return !(flags & WEAPON_DONT_SHOW_TRAVEL);
    }

private:
    explicit Weapon(const ConfigElement &conf);
    ~Weapon();
    static void loadConf();
    static bool confLoaded;
    static std::vector<Weapon *> weapons;
    WeaponType type;
    std::string name;
    std::string abbr; /**< abbreviation for the weapon */
    std::string neg; /**< negative of the name (needed in German) */
    unsigned char canUse; /**< bitmask of classes that can use */
    int range; /**< range of weapon */
    int damage; /**< damage of weapon */
    std::string hitTile; /**< tile to display a hit */
    std::string missTile; /**< tile to display a miss */
    std::string leaveTile; /**< the tile #, zero if nothing left */
    unsigned short flags;
};

#endif // WEAPON_H
