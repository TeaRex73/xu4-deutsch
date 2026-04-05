/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <ctime>
#include <string>
#include <vector>

#include "direction.h"
#include "names.h"
#include "script.h"

class Aura;
class Party;
class StatsArea;
struct SaveGame;

typedef enum {
    TRANSPORT_FOOT = 0x1,
    TRANSPORT_HORSE = 0x2,
    TRANSPORT_SHIP = 0x4,
    TRANSPORT_BALLOON = 0x8,
    TRANSPORT_FOOT_OR_HORSE = TRANSPORT_FOOT | TRANSPORT_HORSE,
    TRANSPORT_ANY = 0xffff
} TransportContext;


/**
 * Context class
 */
class Context:public Script::Provider {
public:
    Context();
    virtual ~Context();
    Context(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;
    Party *party;
    SaveGame *saveGame;
    class Location *location;
    int line, col;
    StatsArea *stats;
    int moonPhase;
    int windDirection;
    bool windLock;
    Aura *aura;
    int horseSpeed;
    int opacity;
    TransportContext transportContext;
    std::time_t lastCommandTime;
    bool willPassTurn;
    class Object *lastShip;


    /**
     * Provides scripts with information
     */
    virtual std::string translate(std::vector<std::string> &parts) override
    {
        if (parts.size() == 1 && parts[0] == "wind") {
            return getDirectionName(static_cast<Direction>(windDirection));
        }
        return "";
    }
};

extern Context *c;

#endif // ifndef CONTEXT_H
