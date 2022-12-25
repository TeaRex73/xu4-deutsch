/*
 * $Id$
 */

#ifndef PORTAL_H
#define PORTAL_H

#include <string>
#include "context.h"
#include "map.h"

class Map;
class Location;
struct _Portal;

typedef enum {
    ACTION_NONE = 0x0,
    ACTION_ENTER = 0x1,
    ACTION_KLIMB = 0x2,
    ACTION_DESCEND = 0x4,
    ACTION_EXIT_NORTH = 0x8,
    ACTION_EXIT_EAST = 0x10,
    ACTION_EXIT_SOUTH = 0x20,
    ACTION_EXIT_WEST = 0x40
} PortalTriggerAction;

typedef bool (*PortalConditionsMet)(const Portal *p);

class PortalDestination {
public:
    PortalDestination()
        :coords(), mapid(0)
    {
    }

    MapCoords coords;
    MapId mapid;
};

class Portal {
public:
    Portal()
        :coords(),
         destid(0),
         start(),
         trigger_action(ACTION_NONE),
         portalConditionsMet(nullptr),
         retroActiveDest(nullptr),
         saveLocation(false),
         message(),
         portalTransportRequisites(TRANSPORT_ANY),
         exitPortal(false)
    {
    }

    ~Portal();
    Portal(const Portal &) = delete;
    Portal(Portal &&) = delete;
    Portal &operator=(const Portal &) = delete;
    Portal &operator=(Portal &&) = delete;

    MapCoords coords;
    MapId destid;
    MapCoords start;
    PortalTriggerAction trigger_action;
    PortalConditionsMet portalConditionsMet;
    PortalDestination *retroActiveDest;
    bool saveLocation;
    std::string message;
    TransportContext portalTransportRequisites;
    bool exitPortal;
};

void createDngLadder(
    Location *location, PortalTriggerAction action, Portal *p
);

bool usePortalAt(
    Location *location, MapCoords coords,PortalTriggerAction action
);

#endif // ifndef PORTAL_H
