/*
 * $Id$
 */

#ifndef ARMOR_H
#define ARMOR_H

#include <string>
#include <vector>

#include "savegame.h"

using std::string;

class ConfigElement;

class Armor {
public:
    static const Armor *get(ArmorType a);
    static const Armor *get(const string &name);

    ArmorType getType() const
    {
        return type; /**< Returns the ArmorType of the armor */
    }
    
    const string &getName() const
    {
        return name;  /**< Returns the name of the armor */
    }

	/** Returns the defense value of the armor */
    int getDefense(bool needsMystic) const
    {
        return needsMystic ? (mystic ? defense : 96) : defense;
	}

    /** Returns true if the class given can wear the armor */
    bool canWear(ClassType klass) const
    {
        return canuse & (1 << klass);
    }
    
private:
    Armor(const ConfigElement &conf);
    static void loadConf();
    static bool confLoaded;
    static std::vector<Armor *> armors;
    ArmorType type;
    string name;
    unsigned char canuse;
    int defense;
    unsigned short mask;
	bool mystic;
};

#endif // ifndef ARMOR_H
