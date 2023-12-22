/*
 * $Id$
 */

#ifndef ARMOR_H
#define ARMOR_H

#include <string>
#include <vector>

#include "savegame.h"



class ConfigElement;

class Armor {
public:
    static void cleanup();

    static const Armor *get(ArmorType a);
    static const Armor *get(const std::string &name);

    ArmorType getType() const
    {
        return type; /**< Returns the ArmorType of the armor */
    }

    const std::string &getName() const
    {
        return name;  /**< Returns the name of the armor */
    }

    const std::string &getNeg() const
    {
        return neg;  /**< Returns the negative name of the armor */
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
    explicit Armor(const ConfigElement &conf);
    ~Armor();
    static void loadConf();
    static bool confLoaded;
    static std::vector<Armor *> armors;
    ArmorType type;
    std::string name;
    std::string neg;
    unsigned char canuse;
    int defense;
    bool mystic;
};

#endif // ifndef ARMOR_H
