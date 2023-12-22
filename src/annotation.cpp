/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>

#include "annotation.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "map.h"
#include "settings.h"


/**
 * Annotation class implementation
 */


/**
 * Constructors
 */
Annotation::Annotation(
    const Coords &coords, MapTile tile, bool visual, bool coverUp
)
    :coords(coords), tile(tile), visual(visual), ttl(-1), coverUp(coverUp)
{
}


/**
 * Members
 */
void Annotation::debug_output() const
{
    std::printf("x: %d\n", coords.x);
    std::printf("y: %d\n", coords.y);
    std::printf("z: %d\n", coords.z);
    std::printf("kachel: %u\n", tile.getId());
    std::printf("sichtbar: %s\n", visual ? "Ja" : "Nein");
}


/**
 * Operators
 */
bool Annotation::operator==(const Annotation &a) const
{
    return ((coords == a.getCoords()) && (tile == a.tile)) ? true : false;
}


/**
 * AnnotationMgr implementation
 */


/**
 * Constructors
 */
AnnotationMgr::AnnotationMgr()
    :annotations()
{
}


/**
 * Members
 */


/**
 * Adds an annotation to the current map
 */
Annotation *AnnotationMgr::add(
    const Coords &coords, MapTile tile, bool visual, bool coverUp
)
{
    /* new annotations go to the front so they're handled "on top" */
    annotations.push_front(Annotation(coords, tile, visual, coverUp));
    return &annotations.front();
}


/**
 * Returns all annotations found at the given map coordinates
 */
Annotation::List AnnotationMgr::allAt(const Coords &coords) const
{
    Annotation::List list;
    Annotation::List::const_iterator i;
    for (i = annotations.cbegin(); i != annotations.cend(); ++i) {
        if (i->getCoords() == coords) {
            list.push_back(*i);
        }
    }
    return list;
}


/**
 * Returns pointers to all annotations found at the given map coordinates
 */
std::list<const Annotation *> AnnotationMgr::ptrsToAllAt(
    const Coords &coords
) const
{
    std::list<const Annotation *> list;
    Annotation::List::const_iterator i;
    for (i = annotations.cbegin(); i != annotations.cend(); ++i) {
        if (i->getCoords() == coords) {
            list.push_back(&(*i));
        }
    }
    return list;
}


/**
 * Removes all annotations on the map
 */
void AnnotationMgr::clear()
{
    annotations.clear();
}


/**
 * Passes a turn for annotations and removes any
 * annotations whose TTL has expired
 */
void AnnotationMgr::passTurn()
{
    Annotation::List::iterator i;
    for (i = annotations.begin(); i != annotations.end(); ++i) {
        if (i->getTTL() == 0) {
            i = annotations.erase(i);
            if (i == annotations.end()) {
                break;
            }
        } else if (i->getTTL() > 0) {
            i->passTurn();
        }
    }
}


/**
 * Removes an annotation from the current map
 */
void AnnotationMgr::remove(const Coords &coords, MapTile tile)
{
    Annotation look_for(coords, tile);
    remove(look_for);
}

void AnnotationMgr::remove(const Annotation &a)
{
    Annotation::List::const_iterator i;
    i = std::find(annotations.cbegin(), annotations.cend(), a);
    if (i != annotations.cend()) {
        annotations.erase(i);
    }
}


/**
 * Removes an entire list of annotations
 */
void AnnotationMgr::remove(const Annotation::List &l)
{
    Annotation::List::const_iterator i;
    for (i = l.cbegin(); i != l.cend(); ++i) {
        remove(*i);
    }
}


/**
 * Returns the number of annotations on the map
 */
int AnnotationMgr::size() const
{
    return annotations.size();
}
