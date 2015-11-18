/*
 * $Id$
 */

#ifndef WEAPON_H
#define WEAPON_H

#include <string>
#include <vector>
#include "savegame.h"

using std::string;

class ConfigElement;

class Weapon {
public:
    enum Flags {
        WEAP_LOSE = 0x0001,
        WEAP_LOSEWHENRANGED = 0x0002,
        WEAP_CHOOSEDISTANCE = 0x0004,
        WEAP_ALWAYSHITS = 0x0008,
        WEAP_MAGIC = 0x0010,
        WEAP_ATTACKTHROUGHOBJECTS = 0x0040,
        WEAP_ABSOLUTERANGE = 0x0080,
        WEAP_RETURNS = 0x0100,
        WEAP_DONTSHOWTRAVEL = 0x0200
    };
    
    static const Weapon *get(WeaponType w);
    static const Weapon *get(const string &name);

    WeaponType getType() const
    {
        return type;
    }
    
    const string &getName() const
    {
        return name;
    }
    
    const string &getAbbrev() const
    {
        return abbr;
    }
    
    bool canReady(ClassType klass) const
    {
        return (canuse & (1 << klass)) != 0;
    }
    
    int getRange() const
    {
        return range;
    }
    
    int getDamage() const
    {
        return damage;
    }
    
    const string &getHitTile() const
    {
        return hittile;
    }
    
    const string &getMissTile() const
    {
        return misstile;
    }
    
    const string &leavesTile() const
    {
        return leavetile;
    }
    
    unsigned short getFlags() const
    {
        return flags;
    }
    
    bool loseWhenUsed() const
    {
        return flags & WEAP_LOSE;
    }
    
    bool loseWhenRanged() const
    {
        return flags & WEAP_LOSEWHENRANGED;
    }
    
    bool canChooseDistance() const
    {
        return flags & WEAP_CHOOSEDISTANCE;
    }
    
    bool alwaysHits() const
    {
        return flags & WEAP_ALWAYSHITS;
    }
    
    bool isMagic() const
    {
        return flags & WEAP_MAGIC;
    }
    
    bool canAttackThroughObjects() const
    {
        return flags & WEAP_ATTACKTHROUGHOBJECTS;
    }
    
    bool rangeAbsolute() const
    {
        return flags & WEAP_ABSOLUTERANGE;
    }
    
    bool returns() const
    {
        return flags & WEAP_RETURNS;
    }
    
    bool showTravel() const
    {
        return !(flags & WEAP_DONTSHOWTRAVEL);
    }
    
private:
    Weapon(const ConfigElement &conf);
    static void loadConf();
    static bool confLoaded;
    static std::vector<Weapon *> weapons;
    WeaponType type;
    string name;
    string abbr; /**< abbreviation for the weapon */
    unsigned char canuse; /**< bitmask of classes that can use */
    int range; /**< range of weapon */
    int damage; /**< damage of weapon */
    string hittile; /**< tile to display a hit */
    string misstile; /**< tile to display a miss */
    string leavetile; /**< the tile #, zero if nothing left */
    unsigned short flags;
};

#endif // ifndef WEAPON_H
