/*
 * $Id$
 */

#ifndef ITEM_H
#define ITEM_H

#include <string>

#include "types.h"

class Map;
class Coords;

enum SearchCondition {
    SC_NONE = 0x00,
    SC_NEWMOONS = 0x01,
    SC_FULLAVATAR = 0x02,
    SC_REAGENTDELAY = 0x04
};

struct ItemLocation {
    const char *name;
    const char *shortname;
    const char *locationLabel;
    bool (*isItemInInventory)(int item);
    void (*putItemInInventory)(int item);
    void (*useItem)(int item);
    int data;
    unsigned char conditions;
};

typedef void (*DestroyAllCreaturesCallback)();

void itemSetDestroyAllCreaturesCallback(DestroyAllCreaturesCallback callback);
const ItemLocation *itemAtLocation(const Map *map, const Coords &coords);
void itemUse(const std::string &shortname);
bool isAbyssOpened(const class Portal *);

#endif // ifndef ITEM_H
