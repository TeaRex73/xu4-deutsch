/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstring>

#include "portal.h"

#include "annotation.h"
#include "city.h"
#include "context.h"
#include "dungeon.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "names.h"
#include "screen.h"
#include "shrine.h"
#include "tile.h"

Portal::~Portal()
{
    delete retroActiveDest;
}


/**
 * Creates a dungeon ladder portal based on the action given
 */
void createDngLadder(
    Location *location, PortalTriggerAction action, Portal *p
)
{
    if (!p) {
        return;
    } else {
        p->destid = location->map->id;
        if ((action == ACTION_KLIMB) && (location->coords.z == 0)) {
            p->exitPortal = true;
            p->destid = 1;
        } else {
            p->exitPortal = false;
        }
        p->message = "";
        p->portalConditionsMet = nullptr;
        p->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
        p->retroActiveDest = nullptr;
        p->saveLocation = false;
        p->start = location->coords;
        p->start.z += (action == ACTION_KLIMB) ? -1 : 1;
    }
}


/**
 * Finds a portal at the given (x,y,z) coords that will work with the action
 * given and uses it.  If in a dungeon and trying to use a ladder, it creates
 * a portal based on the ladder and uses it.
 */
bool usePortalAt(
    Location *location, MapCoords coords, PortalTriggerAction action
)
{
    Map *destination;
    char msg[32] = {};
    const Portal *portal = location->map->portalAt(coords, action);
    Portal dngLadder = {};
    /* didn't find a portal there */
    if (!portal) {
        /* if it's a dungeon, then ladders are predictable.
           Create one! */
        if (location->context == CTX_DUNGEON) {
            Dungeon *dungeon = dynamic_cast<Dungeon *>(location->map);
            if ((action & ACTION_KLIMB) && dungeon->ladderUpAt(coords)) {
                createDngLadder(location, action, &dngLadder);
            } else if ((action & ACTION_DESCEND)
                       && dungeon->ladderDownAt(coords)) {
                createDngLadder(location, action, &dngLadder);
            } else {
                return false;
            }
            portal = &dngLadder;
        } else {
            return false;
        }
    }
    /* conditions not met for portal to work */
    if (portal
        && portal->portalConditionsMet
        && !(*portal->portalConditionsMet)(portal)) {
        return false;
    }
    /* must klimb or descend on foot! */
    else if (c->transportContext & ~TRANSPORT_FOOT
             && ((action == ACTION_KLIMB)
                 || (action == ACTION_DESCEND))) {
        screenMessage("NUR ZU FUSS!\n");
        return true;
    }
    destination = mapMgr->get(portal->destid);
    if (portal->message.empty()) {
        switch (action) {
        case ACTION_DESCEND:
            std::sprintf(
                msg, "Abw{rts\nauf Ebene %d\n", portal->start.z + 1
            );
            break;
        case ACTION_KLIMB:
            if (portal->exitPortal) {
                std::sprintf(msg, "Aufw{rts\nVERLASSE...\n");
            } else {
                std::sprintf(
                    msg, "Aufw{rts\nauf Ebene %d\n", portal->start.z + 1
                );
            }
            break;
        case ACTION_ENTER:
            switch (destination->type) {
            case Map::CITY:
            {
                City *city = dynamic_cast<City *>(destination);
                screenMessage("%s betreten\n\n", city->type.c_str());
                break;
            }
            case Map::SHRINE:
                screenMessage("Schrein betreten\n\n");
                break;
            case Map::DUNGEON:
                screenMessage("H|hle betreten\n\n");
                break;
            default:
                break;
            }
            if ((destination->type == Map::CITY)
                || (destination->type == Map::DUNGEON)) {
                std::string name;
                name = destination->getName();
                for (unsigned int i = 0; i < (16 - name.length()) / 2; i++) {
                    screenMessage(" ");
                }
                screenMessage("%s\n\n", name.c_str());
            }
            break;
        case ACTION_NONE:
        default:
            break;
        } // switch
    }
    /* check the transportation requisites of the portal */
    if (c->transportContext & ~portal->portalTransportRequisites) {
        screenMessage("NUR ZU FUSS!\n");
        return true;
    }
    /* ok, we know the portal is going to work -- now display the
       custom message, if any */
    else if (!portal->message.empty() || std::strlen(msg)) {
        screenMessage(
            "%s", portal->message.empty() ? msg : portal->message.c_str()
        );
    }
    /* portal just exits to parent map */
    if (portal->exitPortal) {
        game->exitToParentMap();
        musicMgr->play();
        return true;
    } else if (portal->destid == location->map->id) {
        location->coords = portal->start;
    } else {
        game->setMap(destination, portal->saveLocation, portal);
        musicMgr->play();
    }
    /* if the portal changes the map retroactively, do it here */
    /*
     * note that we use c->location instead of location, since
     * location has probably been invalidated above
     */
    if (portal->retroActiveDest && c->location->prev) {
        c->location->prev->coords = portal->retroActiveDest->coords;
        c->location->prev->map =
            mapMgr->get(portal->retroActiveDest->mapid);
    }
    if (destination->type == Map::SHRINE) {
        Shrine *shrine = dynamic_cast<Shrine *>(destination);
        shrine->enter();
    }
    return true;
} // usePortalAt
