/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>

#include "savegame.h"

#include "u4io.h"


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
    for (i = 0; i < ARMOR_MAX; i++) {
        if (!writeShort(armor[i], f)) {
            return false;
        }
    }
    for (i = 0; i < WEAPON_MAX; i++) {
        if (!writeShort(weapons[i], f)) {
            return false;
        }
    }
    for (i = 0; i < REAGENT_MAX; i++) {
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
        || !writeShort(balloon_state, f)
        || !writeShort(trammel_phase, f)
        || !writeShort(felucca_phase, f)
        || !writeShort(ship_hull, f)
        || !writeShort(lord_british_intro, f)
        || !writeShort(last_camp, f)
        || !writeShort(last_reagent, f)
        || !writeShort(last_meditation, f)
        || !writeShort(last_virtue, f)
        || !writeChar(dungeon_x, f)
        || !writeChar(dungeon_y, f)
        || !writeShort(orientation, f)
        || !writeShort(dungeon_level, f)
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
        if (!readShort(&karma[i], f)) {
            return false;
        }
    }
    if (!readShort(&torches, f)
        || !readShort(&gems, f)
        || !readShort(&keys, f)
        || !readShort(&sextants, f)) {
        return false;
    }
    for (i = 0; i < ARMOR_MAX; i++) {
        if (!readShort(&armor[i], f)) {
            return false;
        }
    }
    for (i = 0; i < WEAPON_MAX; i++) {
        if (!readShort(&weapons[i], f)) {
            return false;
        }
    }
    for (i = 0; i < REAGENT_MAX; i++) {
        if (!readShort(&reagents[i], f)) {
            return false;
        }
    }
    for (i = 0; i < SPELL_MAX; i++) {
        if (!readShort(&mixtures[i], f)) {
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
        || !readShort(&balloon_state, f)
        || !readShort(&trammel_phase, f)
        || !readShort(&felucca_phase, f)
        || !readShort(&ship_hull, f)
        || !readShort(&lord_british_intro, f)
        || !readShort(&last_camp, f)
        || !readShort(&last_reagent, f)
        || !readShort(&last_meditation, f)
        || !readShort(&last_virtue, f)
        || !readChar(&dungeon_x, f)
        || !readChar(&dungeon_y, f)
        || !readShort(&orientation, f)
        || !readShort(&dungeon_level, f)
        || !readShort(&location, f)) {
        return false;
    }
    /* workaround of U4DOS bug to retain savegame compatibility */
    if (location == 0 && dungeon_level == 0) {
        dungeon_level = 0xFFFF;
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
    for (i = 0; i < ARMOR_MAX; i++) {
        armor[i] = 0;
    }
    for (i = 0; i < WEAPON_MAX; i++) {
        weapons[i] = 0;
    }
    for (i = 0; i < REAGENT_MAX; i++) {
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
    balloon_state = 0;
    trammel_phase = 0;
    felucca_phase = 0;
    ship_hull = 50;
    lord_british_intro = 0;
    last_camp = 0;
    last_reagent = 0;
    last_meditation = 0;
    last_virtue = 0;
    dungeon_x = 0;
    dungeon_y = 0;
    orientation = 0;
    dungeon_level = 0xFFFF;
    location = 0;
} // SaveGame::init

bool SaveGamePlayerRecord::write(std::FILE *f) const
{
    if (!writeShort(hp, f)
        || !writeShort(hp_max, f)
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
    for (const char name_char: name) {
        if (!writeChar(name_char, f)) {
            return false;
        }
    }
    if (!writeChar(sex, f)
        || !writeChar(klass, f)
        || !writeChar(status, f)) {
        return false;
    }
    return true;
}

bool SaveGamePlayerRecord::read(std::FILE *f)
{
    unsigned char ch;
    unsigned short s;
    if (!readShort(&hp, f)
        || !readShort(&hp_max, f)
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
    for (char &name_char: name) {
        if (!readChar(&ch, f)) {
            return false;
        }
        name_char = static_cast<char>(ch);
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
    hp = 0;
    hp_max = 0;
    xp = 0;
    str = 0;
    dex = 0;
    intel = 0;
    mp = 0;
    unknown = 0;
    weapon = WEAPON_HANDS;
    armor = ARMOR_NONE;
    for (char &name_char: name) {
        name_char = '\0';
    }
    sex = SEX_MALE;
    klass = CLASS_MAGE;
    status = STAT_GOOD;
}

bool saveGameMonstersWrite(
    const SaveGameMonsterRecord *monsterTable, std::FILE *f
)
{
    if (monsterTable) {
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].tile, f)) {
                return false;
            }
        }
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].x, f)) {
                return false;
            }
        }
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].y, f)) {
                return false;
            }
        }
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].previous_tile, f)) {
                return false;
            }
        }
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].previous_x, f)) {
                return false;
            }
        }
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].previous_y, f)) {
                return false;
            }
        }
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].z, f)) {
                return false;
            }
        }
        for (int i = 0; i < MONSTER_TABLE_SIZE; i++) {
            if (!writeChar(monsterTable[i].unused, f)) {
                return false;
            }
        }
    } else {
        constexpr int max = MONSTER_TABLE_SIZE * 8;
        for (int i = 0; i < max; i++) {
            if (!writeChar(0, f)) {
                return false;
            }
        }
    }
    return true;
} // saveGameMonstersWrite

bool saveGameMonstersRead(SaveGameMonsterRecord *monsterTable, std::FILE *f)
{
    int i;
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].tile, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].x, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].y, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].previous_tile, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].previous_x, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].previous_y, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].z, f)) {
            return false;
        }
    }
    for (i = 0; i < MONSTER_TABLE_SIZE; i++) {
        if (!readChar(&monsterTable[i].unused, f)) {
            return false;
        }
    }
    return true;
} // saveGameMonstersRead
