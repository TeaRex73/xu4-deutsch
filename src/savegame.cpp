/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "savegame.h"

#include <cstring>
#include "io.h"
#include "object.h"
#include "types.h"



bool SaveGame::write(std::FILE *f) const
{
    int i;
    if (!writeInt(unknown1, f) || !writeInt(moves, f)) {
        return false;
    }
    for (i = 0; i < 8; i++) {
        if (!players[i].write(f)) {
            return false;
        }
    }
    if (!writeInt(food, f) || !writeShort(gold, f)) {
        return false;
    }
    for (i = 0; i < 8; i++) {
        if (!writeShort(karma[i], f)) {
            return false;
        }
    }
    if (!writeShort(torches, f)
        || !writeShort(gems, f)
        || !writeShort(keys, f)
        || !writeShort(sextants, f)) {
        return false;
    }
    for (i = 0; i < ARMR_MAX; i++) {
        if (!writeShort(armor[i], f)) {
            return false;
        }
    }
    for (i = 0; i < WEAP_MAX; i++) {
        if (!writeShort(weapons[i], f)) {
            return false;
        }
    }
    for (i = 0; i < REAG_MAX; i++) {
        if (!writeShort(reagents[i], f)) {
            return false;
        }
    }
    for (i = 0; i < SPELL_MAX; i++) {
        if (!writeShort(mixtures[i], f)) {
            return false;
        }
    }
    if (!writeShort(items, f)
        || !writeChar(x, f)
        || !writeChar(y, f)
        || !writeChar(stones, f)
        || !writeChar(runes, f)
        || !writeShort(members, f)
        || !writeShort(transport, f)
        || !writeShort(balloonstate, f)
        || !writeShort(trammelphase, f)
        || !writeShort(feluccaphase, f)
        || !writeShort(shiphull, f)
        || !writeShort(lbintro, f)
        || !writeShort(lastcamp, f)
        || !writeShort(lastreagent, f)
        || !writeShort(lastmeditation, f)
        || !writeShort(lastvirtue, f)
        || !writeChar(dngx, f)
        || !writeChar(dngy, f)
        || !writeShort(orientation, f)
        || !writeShort(dnglevel, f)
        || !writeShort(location, f)) {
        return false;
    }
    return true;
} // SaveGame::write

bool SaveGame::read(std::FILE *f)
{
    int i;
    if (!readInt(&unknown1, f) || !readInt(&moves, f)) {
        return false;
    }
    for (i = 0; i < 8; i++) {
        if (!players[i].read(f)) {
            return false;
        }
    }
    if (!readInt(&food, f)
        || !readShort(&gold, f)) {
        return false;
    }
    for (i = 0; i < 8; i++) {
        if (!readShort(&(karma[i]), f)) {
            return false;
        }
    }
    if (!readShort(&torches, f)
        || !readShort(&gems, f)
        || !readShort(&keys, f)
        || !readShort(&sextants, f)) {
        return false;
    }
    for (i = 0; i < ARMR_MAX; i++) {
        if (!readShort(&(armor[i]), f)) {
            return false;
        }
    }
    for (i = 0; i < WEAP_MAX; i++) {
        if (!readShort(&(weapons[i]), f)) {
            return false;
        }
    }
    for (i = 0; i < REAG_MAX; i++) {
        if (!readShort(&(reagents[i]), f)) {
            return false;
        }
    }
    for (i = 0; i < SPELL_MAX; i++) {
        if (!readShort(&(mixtures[i]), f)) {
            return false;
        }
    }
    if (!readShort(&items, f)
        || !readChar(&x, f)
        || !readChar(&y, f)
        || !readChar(&stones, f)
        || !readChar(&runes, f)
        || !readShort(&members, f)
        || !readShort(&transport, f)
        || !readShort(&balloonstate, f)
        || !readShort(&trammelphase, f)
        || !readShort(&feluccaphase, f)
        || !readShort(&shiphull, f)
        || !readShort(&lbintro, f)
        || !readShort(&lastcamp, f)
        || !readShort(&lastreagent, f)
        || !readShort(&lastmeditation, f)
        || !readShort(&lastvirtue, f)
        || !readChar(&dngx, f)
        || !readChar(&dngy, f)
        || !readShort(&orientation, f)
        || !readShort(&dnglevel, f)
        || !readShort(&location, f)) {
        return false;
    }
    /* workaround of U4DOS bug to retain savegame compatibility */
    if ((location == 0) && (dnglevel == 0)) {
        dnglevel = 0xFFFF;
    }
    return true;
} // SaveGame::read

void SaveGame::init(const SaveGamePlayerRecord *avatarInfo)
{
    int i;
    unknown1 = 0;
    moves = 0;
    players[0] = *avatarInfo;
    for (i = 1; i < 8; i++) {
        players[i].init();
    }
    food = 0;
    gold = 0;
    for (i = 0; i < 8; i++) {
        karma[i] = 20;
    }
    torches = 0;
    gems = 0;
    keys = 0;
    sextants = 0;
    for (i = 0; i < ARMR_MAX; i++) {
        armor[i] = 0;
    }
    for (i = 0; i < WEAP_MAX; i++) {
        weapons[i] = 0;
    }
    for (i = 0; i < REAG_MAX; i++) {
        reagents[i] = 0;
    }
    for (i = 0; i < SPELL_MAX; i++) {
        mixtures[i] = 0;
    }
    items = 0;
    x = 0;
    y = 0;
    stones = 0;
    runes = 0;
    members = 1;
    transport = 0x1f;
    balloonstate = 0;
    trammelphase = 0;
    feluccaphase = 0;
    shiphull = 50;
    lbintro = 0;
    lastcamp = 0;
    lastreagent = 0;
    lastmeditation = 0;
    lastvirtue = 0;
    dngx = 0;
    dngy = 0;
    orientation = 0;
    dnglevel = 0xFFFF;
    location = 0;
} // SaveGame::init

bool SaveGamePlayerRecord::write(std::FILE *f) const
{
    int i;
    if (!writeShort(hp, f)
        || !writeShort(hpMax, f)
        || !writeShort(xp, f)
        || !writeShort(str, f)
        || !writeShort(dex, f)
        || !writeShort(intel, f)
        || !writeShort(mp, f)
        || !writeShort(unknown, f)
        || !writeShort(weapon, f)
        || !writeShort(armor, f)) {
        return false;
    }
    for (i = 0; i < 16; i++) {
        if (!writeChar(name[i], f)) {
            return false;
        }
    }
    if (!writeChar(static_cast<unsigned char>(sex), f)
        || !writeChar(static_cast<unsigned char>(klass), f)
        || !writeChar(static_cast<unsigned char>(status), f)) {
        return false;
    }
    return true;
}

bool SaveGamePlayerRecord::read(std::FILE *f)
{
    int i;
    unsigned char ch;
    unsigned short s;
    if (!readShort(&hp, f)
        || !readShort(&hpMax, f)
        || !readShort(&xp, f)
        || !readShort(&str, f)
        || !readShort(&dex, f)
        || !readShort(&intel, f)
        || !readShort(&mp, f)
        || !readShort(&unknown, f)) {
        return false;
    }
    if (!readShort(&s, f)) {
        return false;
    }
    weapon = static_cast<WeaponType>(s);
    if (!readShort(&s, f)) {
        return false;
    }
    armor = static_cast<ArmorType>(s);
    for (i = 0; i < 16; i++) {
        if (!readChar(&ch, f)) {
            return false;
        }
        name[i] = static_cast<char>(ch);
    }
    if (!readChar(&ch, f)) {
        return false;
    }
    sex = static_cast<SexType>(ch);
    if (!readChar(&ch, f)) {
        return false;
    }
    klass = static_cast<ClassType>(ch);
    if (!readChar(&ch, f)) {
        return false;
    }
    status = static_cast<StatusType>(ch);
    return true;
} // SaveGamePlayerRecord::read

void SaveGamePlayerRecord::init()
{
    int i;
    hp = 0;
    hpMax = 0;
    xp = 0;
    str = 0;
    dex = 0;
    intel = 0;
    mp = 0;
    unknown = 0;
    weapon = WEAP_HANDS;
    armor = ARMR_NONE;
    for (i = 0; i < 16; i++) {
        name[i] = '\0';
    }
    sex = SEX_MALE;
    klass = CLASS_MAGE;
    status = STAT_GOOD;
}

bool saveGameMonstersWrite(SaveGameMonsterRecord *monsterTable, std::FILE *f)
{
    int i, max;
    if (monsterTable) {
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].tile, f)) {
                return false;
            }
        }
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].x, f)) {
                return false;
            }
        }
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].y, f)) {
                return false;
            }
        }
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].prevTile, f)) {
                return false;
            }
        }
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].prevx, f)) {
                return false;
            }
        }
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].prevy, f)) {
                return false;
            }
        }
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].z, f)) {
                return false;
            }
        }
        for (i = 0; i < MONSTERTABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].unused, f)) {
                return false;
            }
        }
    } else {
        max = MONSTERTABLE_SIZE * 8;
        for (i = 0; i < max; i++) {
            if (!writeChar(static_cast<unsigned char>(0), f)) {
                return false;
            }
        }
    }
    return true;
} // saveGameMonstersWrite

bool saveGameMonstersRead(SaveGameMonsterRecord *monsterTable, std::FILE *f)
{
    int i;
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].tile, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].x, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].y, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].prevTile, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].prevx, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].prevy, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].z, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].unused, f)) {
            return false;
        }
    }
    return true;
} // saveGameMonstersRead
