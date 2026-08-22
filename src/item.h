/*
 * $Id$
 */

#ifndef ITEM_H
#define ITEM_H

#include <string>

class Coords;
class Map;
class Portal;


enum SearchCondition {
    SC_NONE = 0x00,
    SC_NEW_MOONS = 0x01,
    SC_FULL_AVATAR = 0x02,
    SC_REAGENT_DELAY = 0x04
};

struct ItemLocation {
    const char *name;
    const char *shortname;
    const char *locationLabel;
    bool (*isItemInInventory)(int item);
    void (*putItemInInventory)(int item);
    void (*useItem)(int item);
    int data;
    unsigned int conditions;
};

typedef void (*DestroyAllCreaturesCallback)();

void itemSetDestroyAllCreaturesCallback(DestroyAllCreaturesCallback callback);
const ItemLocation *itemAtLocation(const Map *map, const Coords &coords);
void itemUse(const std::string &shortname);
bool isAbyssOpened(const Portal *);

#endif // ITEM_H
