/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <map>
#include <utility>

#include "moongate.h"

#include "coords.h"
#include "error.h"


/* map moon phase to map coordinates */
typedef std::map<int, Coords> MoongateList;

static MoongateList gates;

void moongateAdd(int phase, const Coords &coords)
{
    if (!gates.insert(MoongateList::value_type(phase, coords)).second) {
        errorFatal("Error: A moongate for phase %d already exists", phase);
    }
}

const Coords *moongateGetGateCoordsForPhase(const int phase)
{
    const auto moongate = gates.find(phase);
    if (moongate != gates.end()) {
        return &moongate->second;
    }
    return nullptr;
}

bool moongateFindActiveGateAt(
    const int trammel, const int felucca, const Coords &src, Coords &dest
)
{
    const Coords *moongate_coords = moongateGetGateCoordsForPhase(trammel);
    if (moongate_coords && src == *moongate_coords) {
        moongate_coords = moongateGetGateCoordsForPhase(felucca);
        if (moongate_coords) {
            dest = *moongate_coords;
            return true;
        }
    }
    return false;
}

bool moongateIsEntryToShrineOfSpirituality(const int trammel, const int felucca)
{
    return trammel == 4 && felucca == 4;
}
