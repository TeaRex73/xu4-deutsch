/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <vector>
#include <cstring>

#include "armor.h"

#include "config.h"
#include "error.h"
#include "names.h"
#include "tile.h"
#include "utils.h"



bool Armor::confLoaded = false;
std::vector<Armor *> Armor::armors;


/**
 * Returns armor by ArmorType.
 */
const Armor *Armor::get(ArmorType a)
{
    // Load in XML if it hasn't been already
    loadConf();
    if (static_cast<unsigned int>(a) >= armors.size()) {
        return nullptr;
    }
    return armors[a];
}


/**
 * Returns armor that has the given name
 */
const Armor *Armor::get(const std::string &name)
{
    // Load in XML if it hasn't been already
    loadConf();
    for (unsigned int i = 0; i < armors.size(); i++) {
        if (xu4_strcasecmp(name.c_str(), armors[i]->name.c_str()) == 0) {
            return armors[i];
        }
    }
    return nullptr;
}


Armor::Armor(const ConfigElement &conf)
    :type(static_cast<ArmorType>(armors.size())),
     name(conf.getString("name")),
     neg(conf.getString("neg")),
     canuse(0xFF),
     defense(conf.getInt("defense")),
     mask(0),
     mystic(conf.getBool("mystic"))
{
    std::vector<ConfigElement> contraintConfs = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = contraintConfs.begin();
         i != contraintConfs.end();
         i++) {
        unsigned char mask = 0;
        if (i->getName() != "constraint") {
            continue;
        }
        for (int cl = 0; cl < 8; cl++) {
            if (xu4_strcasecmp(
                    i->getString("class").c_str(),
                    getClassNameEnglish(
                        static_cast<ClassType>(cl))
                ) == 0) {
                mask = (1 << cl);
            }
        }
        if ((mask == 0)
            && (xu4_strcasecmp(i->getString("class").c_str(), "all") == 0)) {
            mask = 0xFF;
        }
        if (mask == 0) {
            errorFatal(
                "malformed armor.xml file: constraint has unknown class %s",
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

Armor::~Armor()
{
    for (std::vector<Armor *>::iterator i = armors.begin();
         i != armors.end();
         ) {
        if (*i == this) {
            i = armors.erase(i);
        } else {
            i++;
        }
    }
}

void Armor::cleanup()
{
    for (std::vector<Armor *>::iterator i = armors.begin();
         i != armors.end();
        ) { // no increment, deleting moves consecutive elements to 1st pos
        delete *i;
    }
    armors.clear();
}    

void Armor::loadConf()
{
    if (!__builtin_expect(confLoaded, true)) {
        confLoaded = true;
    } else {
        return;
    }
    const Config *config = Config::getInstance();
    std::vector<ConfigElement> armorConfs =
        config->getElement("armors").getChildren();
    for (std::vector<ConfigElement>::iterator i = armorConfs.begin();
         i != armorConfs.end();
         i++) {
        if (i->getName() != "armor") {
            continue;
        }
        armors.push_back(new Armor(*i));
    }
}
