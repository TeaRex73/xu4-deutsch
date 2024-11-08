/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
#include <cstring>

#include "weapon.h"

#include "config.h"
#include "error.h"
#include "names.h"

using std::string;
using std::vector;


bool Weapon::confLoaded = false;
vector<Weapon *> Weapon::weapons;

/**
 * Returns weapon by WeaponType.
 */
const Weapon *Weapon::get(WeaponType w) {
    // Load in XML if it hasn't been already
    loadConf();

    if (static_cast<unsigned>(w) >= weapons.size())
        return NULL;
    return weapons[w];
}

/**
 * Returns weapon that has the given name
 */
const Weapon *Weapon::get(const string &name) {
    // Load in XML if it hasn't been already
    loadConf();

    for (unsigned i = 0; i < weapons.size(); i++) {
        if (strcasecmp(name.c_str(), weapons[i]->name.c_str()) == 0)
            return weapons[i];
    }
    return NULL;
}

Weapon::Weapon(const ConfigElement &conf)
	: type (static_cast<WeaponType>(weapons.size()))
	, name (conf.getString("name"))
	, abbr (conf.getString("abbr"))
	, canuse (0xFF)
	, range (0)
	, damage (conf.getInt("damage"))
	, hittile ("hit_flash")
	, misstile ("miss_flash")
	, leavetile ("")
	, flags (0) {
    static const struct {
        const char *name;
        unsigned int flag;
    } booleanAttributes[] = {
        { "lose", WEAP_LOSE },
        { "losewhenranged", WEAP_LOSEWHENRANGED },
        { "choosedistance", WEAP_CHOOSEDISTANCE },
        { "alwayshits", WEAP_ALWAYSHITS },
        { "magic", WEAP_MAGIC },
        { "attackthroughobjects", WEAP_ATTACKTHROUGHOBJECTS },
        { "returns", WEAP_RETURNS },
        { "dontshowtravel", WEAP_DONTSHOWTRAVEL }
    };

    /* Get the range of the weapon, whether it is absolute or normal range */
    string _range = conf.getString("range");
    if (_range.empty()) {
        _range = conf.getString("absolute_range");
        if (!_range.empty())
            flags |= WEAP_ABSOLUTERANGE;
    }
    if (_range.empty())
        errorFatal("malformed weapons.xml file: range or absolute_range not found for weapon %s", name.c_str());

    range = atoi(_range.c_str());

    /* Load weapon attributes */
    for (unsigned at = 0; at < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); at++) {
        if (conf.getBool(booleanAttributes[at].name)) {
            flags |= booleanAttributes[at].flag;
        }
    }

    /* Load hit tiles */
    if (conf.exists("hittile"))
        hittile = conf.getString("hittile");

    /* Load miss tiles */
    if (conf.exists("misstile"))
        misstile = conf.getString("misstile");

    /* Load leave tiles */
    if (conf.exists("leavetile")) {
        leavetile = conf.getString("leavetile");
    }

    vector<ConfigElement> contraintConfs = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = contraintConfs.begin(); i != contraintConfs.end(); i++) {
        unsigned char mask = 0;

        if (i->getName() != "constraint")
            continue;

        for (int cl = 0; cl < 8; cl++) {
            if (strcasecmp(i->getString("class").c_str(), getClassNameEnglish(static_cast<ClassType>(cl))) == 0)
                mask = (1 << cl);
        }
        if (mask == 0 && strcasecmp(i->getString("class").c_str(), "all") == 0)
            mask = 0xFF;
        if (mask == 0) {
            errorFatal("malformed weapons.xml file: constraint has unknown class %s",
                       i->getString("class").c_str());
        }
        if (i->getBool("canuse"))
            canuse |= mask;
        else
            canuse &= ~mask;
    }
}

void Weapon::loadConf() {
    if (confLoaded)
        return;

    confLoaded = true;
    const Config *config = Config::getInstance();

    vector<ConfigElement> weaponConfs = config->getElement("weapons").getChildren();
    for (std::vector<ConfigElement>::iterator i = weaponConfs.begin(); i != weaponConfs.end(); i++) {
        if (i->getName() != "weapon")
            continue;

        weapons.push_back(new Weapon(*i));
    }
}

