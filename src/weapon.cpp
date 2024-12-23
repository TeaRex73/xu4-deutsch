/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include <cstdlib>
#include <cstring>

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
const Weapon *Weapon::get(WeaponType w)
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
    for (unsigned int i = 0; i < weapons.size(); i++) {
        if (xu4_strcasecmp(name.c_str(), weapons[i]->name.c_str()) == 0) {
            return weapons[i];
        }
    }
    return nullptr;
}


Weapon::Weapon(const ConfigElement &conf)
    :type(static_cast<WeaponType>(weapons.size())),
     name(conf.getString("name")),
     abbr(conf.getString("abbr")),
     neg(conf.getString("neg")),
     canuse(0xFF),
     range(0),
     damage(conf.getInt("damage")),
     hittile("hit_flash"),
     misstile("miss_flash"),
     leavetile(),
     flags(0)
{
    static const struct {
        const char *name;
        unsigned int flag;
    } booleanAttributes[] = {
        { "lose", WEAP_LOSE },
        { "losewhenranged", WEAP_LOSEWHENRANGED },
        { "choosedistance", WEAP_CHOOSEDISTANCE },
        { "alwayshits", WEAP_ALWAYSHITS },
        { "mystic", WEAP_MYSTIC },
        { "attackthroughobjects", WEAP_ATTACKTHROUGHOBJECTS },
        { "returns", WEAP_RETURNS },
        { "dontshowtravel", WEAP_DONTSHOWTRAVEL },
        { "rangedonly", WEAP_RANGEDONLY }
    };
    /* Get the range of the weapon, whether it is absolute or
       normal range */
    std::string wrange = conf.getString("range");
    if (wrange.empty()) {
        wrange = conf.getString("absolute_range");
        if (!wrange.empty()) {
            flags |= WEAP_ABSOLUTERANGE;
        }
    }
    if (wrange.empty()) {
        errorFatal(
            "malformed weapons.xml file: range or absolute_range not found "
            "for weapon %s",
            name.c_str()
        );
    }
    range = std::atoi(wrange.c_str());
    /* Load weapon attributes */
    for (unsigned int at = 0;
         at < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]);
         at++) {
        if (conf.getBool(booleanAttributes[at].name)) {
            flags |= booleanAttributes[at].flag;
        }
    }
    /* Load hit tiles */
    if (conf.exists("hittile")) {
        hittile = conf.getString("hittile");
    }
    /* Load miss tiles */
    if (conf.exists("misstile")) {
        misstile = conf.getString("misstile");
    }
    /* Load leave tiles */
    if (conf.exists("leavetile")) {
        leavetile = conf.getString("leavetile");
    }
    std::vector<ConfigElement> contraintConfs = conf.getChildren();
    for (std::vector<ConfigElement>::const_iterator i =
             contraintConfs.cbegin();
         i != contraintConfs.cend();
         ++i) {
        unsigned char mask = 0;
        if (i->getName() != "constraint") {
            continue;
        }
        for (int cl = 0; cl < 8; cl++) {
            if (xu4_strcasecmp(
                    i->getString("class").c_str(),
                    getClassNameEnglish(static_cast<ClassType>(cl))
                ) == 0) {
                mask = (1 << cl);
            }
        }
        if ((mask == 0) &&
            (xu4_strcasecmp(i->getString("class").c_str(), "all") == 0)) {
            mask = 0xFF;
        }
        if (mask == 0) {
            errorFatal(
                "malformed weapons.xml file: constraint has unknown class %s",
                i->getString("class").c_str()
            );
        }
        if (i->getBool("canuse")) {
            canuse |= mask;
        } else {
            canuse &= ~mask;
        }
    }
}

Weapon::~Weapon()
{
    for (std::vector<Weapon *>::iterator i = weapons.begin();
         i != weapons.end();
         ) {
        if (*i == this) {
            i = weapons.erase(i);
        } else {
            ++i;
        }
    }
}

void Weapon::cleanup()
{
    for (std::vector<Weapon *>::iterator i = weapons.begin();
         i != weapons.end();
        ) { // no increment, deleting moves consecutive elements to 1st pos
        delete *i;
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
    std::vector<ConfigElement> weaponConfs =
        config->getElement("weapons").getChildren();
    for (std::vector<ConfigElement>::const_iterator i = weaponConfs.cbegin();
         i != weaponConfs.cend();
         ++i) {
        if (i->getName() != "weapon") {
            continue;
        }
        weapons.push_back(new Weapon(*i));
    }
}
