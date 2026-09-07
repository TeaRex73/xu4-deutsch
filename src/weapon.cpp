/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdlib>
#include <string>
#include <vector>

#include "weapon.h"

#include "config.h"
#include "error.h"
#include "names.h"
#include "utils.h"


bool Weapon::confLoaded = false;
std::vector<Weapon *> Weapon::weapons;


/**
 * Returns weapon by WeaponType.
 */
const Weapon *Weapon::get(const WeaponType w)
{
    // Load in XML if it hasn't been already
    loadConf();
    if (static_cast<unsigned int>(w) >= weapons.size()) {
        return nullptr;
    }
    return weapons[w];
}


/**
 * Returns weapon that has the given name
 */
const Weapon *Weapon::get(const std::string &name)
{
    // Load in XML if it hasn't been already
    loadConf();
    for (const auto *weapon: weapons) {
        if (xu4_strcasecmp(name.c_str(), weapon->name.c_str()) == 0) {
            return weapon;
        }
    }
    return nullptr;
}


Weapon::Weapon(const ConfigElement &conf)
    :type(static_cast<WeaponType>(weapons.size())),
     name(conf.getString("name")),
     abbr(conf.getString("abbr")),
     neg(conf.getString("neg")),
     canUse(0xFF),
     range(0),
     damage(conf.getInt("damage")),
     hitTile("hit_flash"),
     missTile("miss_flash"),
     flags(0)
{
    static const struct {
        const char *name;
        unsigned int flag;
    } booleanAttributes[] = {
        { .name = "lose", .flag = WEAPON_LOSE },
        { .name = "losewhenranged", .flag = WEAPON_LOSE_WHEN_RANGED },
        { .name = "choosedistance", .flag = WEAPON_CHOOSE_DISTANCE },
        { .name = "alwayshits", .flag = WEAPON_ALWAYS_HITS },
        { .name = "magic", .flag = WEAPON_MAGIC },
        { .name = "attackthroughobjects", .flag = WEAPON_ATTACK_THROUGH_OBJS },
        { .name = "returns", .flag = WEAPON_RETURNS },
        { .name = "dontshowtravel", .flag = WEAPON_DONT_SHOW_TRAVEL },
        { .name = "rangedonly", .flag = WEAPON_RANGED_ONLY },
        { .name = "mystic", .flag = WEAPON_MYSTIC },
    };
    /* Get the range of the weapon, whether it is absolute or
       normal range */
    std::string wrange = conf.getString("range");
    if (wrange.empty()) {
        wrange = conf.getString("absolute_range");
        if (!wrange.empty()) {
            flags |= WEAPON_ABSOLUTE_RANGE;
        }
    }
    if (wrange.empty()) {
        errorFatal(
            "malformed weapons.xml file: range or absolute_range not found "
            "for weapon %s",
            name.c_str()
        );
    }
    range = static_cast<int>(std::strtol(wrange.c_str(), nullptr, 10));
    /* Load weapon attributes */
    for (const auto &booleanAttribute : booleanAttributes) {
        if (conf.getBool(booleanAttribute.name)) {
            flags |= booleanAttribute.flag;
        }
    }
    /* Load hit tiles */
    if (conf.exists("hittile")) {
        hitTile = conf.getString("hittile");
    }
    /* Load miss tiles */
    if (conf.exists("misstile")) {
        missTile = conf.getString("misstile");
    }
    /* Load leave tiles */
    if (conf.exists("leavetile")) {
        leaveTile = conf.getString("leavetile");
    }
    const std::vector<ConfigElement> constraintConfs = conf.getChildren();
    for (const auto &constraintConf: constraintConfs) {
        unsigned char mask = 0;
        if (constraintConf.getName() != "constraint") {
            continue;
        }
        for (int cl = 0; cl < 8; cl++) {
            if (xu4_strcasecmp(
                    constraintConf.getString("class").c_str(),
                    getClassNameEnglish(static_cast<ClassType>(cl))
                ) == 0) {
                mask = 1 << cl;
            }
        }
        if (mask == 0 &&
            xu4_strcasecmp(
                constraintConf.getString("class").c_str(), "all"
            ) == 0) {
            mask = 0xFF;
        }
        if (mask == 0) {
            errorFatal(
                "malformed weapons.xml file: constraint has unknown class %s",
                constraintConf.getString("class").c_str()
            );
        }
        if (constraintConf.getBool("canuse")) {
            canUse |= mask;
        } else {
            canUse &= ~mask;
        }
    }
}

Weapon::~Weapon()
{
    for (auto weapon = weapons.begin(); weapon != weapons.end();) {
        if (*weapon == this) {
            weapon = weapons.erase(weapon);
        } else {
            ++weapon;
        }
    }
}

void Weapon::cleanup()
{
    for (const auto weapon = weapons.begin(); weapon != weapons.end();) {
        // no increment, deleting moves consecutive elements to 1st pos
        delete *weapon;
    }
    weapons.clear();
}

void Weapon::loadConf()
{
    if (__builtin_expect(confLoaded, true)) {
        return;
    }
    confLoaded = true;
    const Config *config = Config::getInstance();
    const std::vector<ConfigElement> weaponConfs =
        config->getElement("weapons").getChildren();
    for (const auto &weaponConf : weaponConfs) {
        if (weaponConf.getName() != "weapon") {
            continue;
        }
        weapons.push_back(new Weapon(weaponConf));
    }
}
