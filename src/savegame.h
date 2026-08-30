/*
 * $Id$
 */

#ifndef SAVEGAME_H
#define SAVEGAME_H

#include <cstdio>


#define PARTY_SAV_BASE_FILENAME "party.sav"
#define MONSTERS_SAV_BASE_FILENAME "monsters.sav"
#define OUTMONST_SAV_BASE_FILENAME "outmonst.sav"
#define DNGMAP_SAV_BASE_FILENAME "dngmap.sav"

#define MONSTER_TABLE_SIZE 32
#define MONSTER_TABLE_FORCES_OF_NATURE_SIZE 4
#define MONSTER_TABLE_CREATURES_SIZE 8
#define MONSTER_TABLE_OBJECTS_SIZE \
    (MONSTER_TABLE_SIZE - MONSTER_TABLE_CREATURES_SIZE)

/**
 * The list of all weapons.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
enum WeaponType: unsigned short {
    WEAPON_HANDS,
    WEAPON_STAFF,
    WEAPON_DAGGER,
    WEAPON_SLING,
    WEAPON_MACE,
    WEAPON_AXE,
    WEAPON_SWORD,
    WEAPON_BOW,
    WEAPON_CROSSBOW,
    WEAPON_OIL,
    WEAPON_HALBERD,
    WEAPON_MAGIC_AXE,
    WEAPON_MAGIC_SWORD,
    WEAPON_MAGIC_BOW,
    WEAPON_MAGIC_WAND,
    WEAPON_MYSTIC_SWORD,
    WEAPON_MAX
};


/**
 * The list of all armor types.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
enum ArmorType: unsigned short {
    ARMOR_NONE,
    ARMOR_CLOTH,
    ARMOR_LEATHER,
    ARMOR_CHAIN,
    ARMOR_PLATE,
    ARMOR_MAGIC_CHAIN,
    ARMOR_MAGIC_PLATE,
    ARMOR_MYSTIC_ROBES,
    ARMOR_MAX
};


/**
 * The list of sex values for the savegame character records.  The
 * values match the male and female symbols in the character set.
 */
enum SexType: unsigned char {
    SEX_MALE = 0xb,
    SEX_FEMALE = 0xc
};


/**
 * The list of class types for the savegame character records.
 */
enum ClassType: unsigned char {
    CLASS_MAGE,
    CLASS_BARD,
    CLASS_FIGHTER,
    CLASS_DRUID,
    CLASS_TINKER,
    CLASS_PALADIN,
    CLASS_RANGER,
    CLASS_SHEPHERD
};


/**
 * The list of status values for the savegame character records.  The
 * values match the letters that appear in the Ztats area.
 */
enum StatusType: unsigned char {
    STAT_GOOD = 'G',
    STAT_POISONED = 'V',
    STAT_SLEEPING = 'S',
    STAT_DEAD = 'T'
};

enum Virtue: unsigned char {
    VIRTUE_HONESTY,
    VIRTUE_COMPASSION,
    VIRTUE_VALOR,
    VIRTUE_JUSTICE,
    VIRTUE_SACRIFICE,
    VIRTUE_HONOR,
    VIRTUE_SPIRITUALITY,
    VIRTUE_HUMILITY,
    VIRTUE_MAX
};

enum BaseVirtue: unsigned char {
    VIRTUE_NONE = 0x00,
    VIRTUE_TRUTH = 0x01,
    VIRTUE_LOVE = 0x02,
    VIRTUE_COURAGE = 0x04
};

enum Reagent: unsigned char {
    REAGENT_ASH,
    REAGENT_GINSENG,
    REAGENT_GARLIC,
    REAGENT_SILK,
    REAGENT_MOSS,
    REAGENT_PEARL,
    REAGENT_NIGHTSHADE,
    REAGENT_MANDRAKE,
    REAGENT_MAX
};

#define SPELL_MAX 26

enum Item: unsigned short {
    ITEM_SKULL = 0x01,
    ITEM_SKULL_DESTROYED = 0x02,
    ITEM_CANDLE = 0x04,
    ITEM_BOOK = 0x08,
    ITEM_BELL = 0x10,
    ITEM_KEY_C = 0x20,
    ITEM_KEY_L = 0x40,
    ITEM_KEY_T = 0x80,
    ITEM_HORN = 0x100,
    ITEM_WHEEL = 0x200,
    ITEM_CANDLE_USED = 0x400,
    ITEM_BOOK_USED = 0x800,
    ITEM_BELL_USED = 0x1000
};

enum Stone: unsigned char {
    STONE_BLUE = 0x01,
    STONE_YELLOW = 0x02,
    STONE_RED = 0x04,
    STONE_GREEN = 0x08,
    STONE_ORANGE = 0x10,
    STONE_PURPLE = 0x20,
    STONE_WHITE = 0x40,
    STONE_BLACK = 0x80
};

enum Rune: unsigned char {
    RUNE_HONESTY = 0x01,
    RUNE_COMPASSION = 0x02,
    RUNE_VALOR = 0x04,
    RUNE_JUSTICE = 0x08,
    RUNE_SACRIFICE = 0x10,
    RUNE_HONOR = 0x20,
    RUNE_SPIRITUALITY = 0x40,
    RUNE_HUMILITY = 0x80
};


/**
 * The Ultima IV savegame player record data.
 * NOT binary identical to on-disk file because of alignment, see below
 */
struct SaveGamePlayerRecord {
    bool write(std::FILE *f) const;
    bool read(std::FILE *f);
    void init();

    unsigned short hp;
    unsigned short hp_max;
    unsigned short xp;
    unsigned short str, dex, intel;
    unsigned short mp;
    unsigned short unknown;
    WeaponType weapon;
    ArmorType armor;
    char name[16];
    SexType sex;
    ClassType klass;
    StatusType status;
};


/**
 * How Ultima IV stores monster information in MONSTERS.SAV and OUTMONST.SAV
 * The actual on-disk format has one 32-byte table for tile,
 * followed by one 32-byte table for x, and so on.
 * (struct of arrays, not array of structs,
 * a heritage from the MOS6502 based original Apple II version)
 */
struct SaveGameMonsterRecord {
    unsigned char tile;
    unsigned char x;
    unsigned char y;
    unsigned char previous_tile;
    unsigned char previous_x;
    unsigned char previous_y;
    unsigned char z;
    unsigned char unused;
};


/**
 * Represents the on-disk contents of PARTY.SAV field-by-field,
 * though struct is NOT binary identical to file because PARTY.SAV has
 * unaligned 16-bit and 32-bit fields, which is not useful in-memory
 * on modern CPUs.
 */
struct SaveGame {
    bool write(std::FILE *f) const;
    bool read(std::FILE *f);
    void init(const SaveGamePlayerRecord *avatarInfo);

    unsigned int unknown1;
    unsigned int moves;
    SaveGamePlayerRecord players[8];
    unsigned int food;
    unsigned short gold;
    unsigned short karma[VIRTUE_MAX];
    unsigned short torches;
    unsigned short gems;
    unsigned short keys;
    unsigned short sextants;
    unsigned short armor[ARMOR_MAX];
    unsigned short weapons[WEAPON_MAX];
    unsigned short reagents[REAGENT_MAX];
    unsigned short mixtures[SPELL_MAX];
    unsigned short items;
    unsigned char x, y;
    unsigned char stones;
    unsigned char runes;
    unsigned short members;
    unsigned short transport;
    union {
        unsigned short balloon_state;
        unsigned short torch_duration;
    };
    unsigned short trammel_phase;
    unsigned short felucca_phase;
    unsigned short ship_hull;
    unsigned short lord_british_intro;
    unsigned short last_camp;
    unsigned short last_reagent;
    unsigned short last_meditation;
    unsigned short last_virtue;
    unsigned char dungeon_x, dungeon_y;
    unsigned short orientation;
    unsigned short dungeon_level;
    unsigned short location;
};

bool saveGameMonstersWrite(
    const SaveGameMonsterRecord *monsterTable, std::FILE *f
);
bool saveGameMonstersRead(SaveGameMonsterRecord *monsterTable, std::FILE *f);

#endif // SAVEGAME_H
