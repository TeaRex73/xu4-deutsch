/*
 * $Id$
 */

#ifndef DUNGEON_H
#define DUNGEON_H

#include <vector>

#include "combat.h"
#include "map.h"
#include "types.h"

#define DNGROOM_NTRIGGERS 4

enum StatsBonusType {
    STATSBONUS_INT = 0x1,
    STATSBONUS_DEX = 0x2,
    STATSBONUS_STR = 0x4
};

struct Trigger {
    unsigned char tile;
    unsigned char x, y;
    unsigned char change_x1, change_y1, change_x2, change_y2;
};

class DngRoom {
public:
    DngRoom()
        :triggers(),
         creature_tiles(),
         creature_start_x(),
         creature_start_y(),
         party_north_start_x(),
         party_north_start_y(),
         party_east_start_x(),
         party_east_start_y(),
         party_south_start_x(),
         party_south_start_y(),
         party_west_start_x(),
         party_west_start_y(),
         map_data(),
         buffer()
    {
    }

    Trigger triggers[DNGROOM_NTRIGGERS];
    unsigned char creature_tiles[16];
    unsigned char creature_start_x[16];
    unsigned char creature_start_y[16];
    unsigned char party_north_start_x[8];
    unsigned char party_north_start_y[8];
    unsigned char party_east_start_x[8];
    unsigned char party_east_start_y[8];
    unsigned char party_south_start_x[8];
    unsigned char party_south_start_y[8];
    unsigned char party_west_start_x[8];
    unsigned char party_west_start_y[8];
    MapData map_data;/* This is OK to change to MapData since sizeof(DngRoom)
                        or anything like it is not being used. */
    unsigned char buffer[7];
};


/**
 * Dungeon tokens
 */
enum DungeonToken {
    DUNGEON_CORRIDOR = 0x00,
    DUNGEON_LADDER_UP = 0x10,
    DUNGEON_LADDER_DOWN = 0x20,
    DUNGEON_LADDER_UPDOWN = 0x30,
    DUNGEON_CHEST = 0x40,
    DUNGEON_CEILING_HOLE = 0x50,
    DUNGEON_FLOOR_HOLE = 0x60,
    DUNGEON_MAGIC_ORB = 0x70,
    DUNGEON_TRAP = 0x80,
    DUNGEON_FOUNTAIN = 0x90,
    DUNGEON_FIELD = 0xA0,
    DUNGEON_ALTAR = 0xB0,
    DUNGEON_DOOR = 0xC0,
    DUNGEON_ROOM = 0xD0,
    DUNGEON_SECRET_DOOR = 0xE0,
    DUNGEON_WALL = 0xF0
};

class Dungeon:public Map {
public:
    Dungeon()
        :name(),
         n_rooms(0),
         dataSubTokens(),
         rooms(nullptr),
         roomMaps(nullptr),
         currentRoom(0),
         tempData(),
         tempDataSubTokens()
    {
    }

    Dungeon(const Dungeon &) = delete;
    Dungeon(Dungeon &&) = delete;
    Dungeon &operator=(const Dungeon &) = delete;
    Dungeon &operator=(Dungeon &&) = delete;
    virtual ~Dungeon();
    virtual std::string getName() override;
    DungeonToken tokenForTile(MapTile tile) const;
    DungeonToken currentToken() const;
    unsigned char currentSubToken() const;
    DungeonToken tokenAt(const MapCoords &coords) const;
    unsigned char subTokenAt(const MapCoords &coords) const;
    bool ladderUpAt(const MapCoords &coords);
    bool ladderDownAt(const MapCoords &coords);
    bool validTeleportLocation(const MapCoords &coords) const;
    std::string name;
    unsigned int n_rooms;
    std::vector<unsigned char> dataSubTokens;
    DngRoom *rooms;
    CombatMap **roomMaps;
    int currentRoom;
    MapData tempData;
    std::vector<unsigned char> tempDataSubTokens;
};


/**
 * Dungeon sub-tokens
 */
enum TrapType {
    TRAP_WINDS = 0x0,
    TRAP_FALLING_ROCK = 0x1,
    TRAP_PIT = 0xE
};

enum FountainType {
    FOUNTAIN_NORMAL = 0x0,
    FOUNTAIN_HEALING = 0x1,
    FOUNTAIN_ACID = 0x2,
    FOUNTAIN_CURE = 0x3,
    FOUNTAIN_POISON = 0x4
};

enum FieldType {
    FIELD_POISON = 0x0,
    FIELD_ENERGY = 0x1,
    FIELD_FIRE = 0x2,
    FIELD_SLEEP = 0x3
};

void dungeonSearch();
void dungeonDrinkFountain();
void dungeonTouchOrb();
bool dungeonHandleTrap(TrapType trap);
bool isDungeon(Map *punknown);

#endif // ifndef DUNGEON_H
