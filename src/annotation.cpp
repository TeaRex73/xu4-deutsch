/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <cstdio>
#include <list>

#include "annotation.h"
#include "coords.h"
#include "types.h"

/**
 * Annotation class implementation
 */


/**
 * Constructors
 */
Annotation::Annotation(
    const Coords &coords, MapTile tile, const bool visual, const bool coverUp
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
    return coords == a.getCoords() && tile == a.tile;
}


/**
 * AnnotationMgr implementation
 */


/**
 * Constructors
 */
AnnotationMgr::AnnotationMgr() = default;


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
    annotations.emplace_front(coords, tile, visual, coverUp);
    return &annotations.front();
}


/**
 * Returns all annotations found at the given map coordinates
 */
Annotation::List AnnotationMgr::allAt(const Coords &coords) const
{
    Annotation::List list;
    for (const auto &annotation: annotations) {
        if (annotation.getCoords() == coords) {
            list.push_back(annotation);
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
    for (const auto &annotation: annotations) {
        if (annotation.getCoords() == coords) {
            list.push_back(&annotation);
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
    annotations.remove_if(
        [&](const Annotation &v) -> bool {
            return v.getTTL() == 0;
        }
    );
    std::for_each(
        annotations.begin(),
        annotations.end(),
        [&](Annotation &v) -> void {
            v.passTurn();
        }
    );
}


/**
 * Removes an annotation from the current map
 */
void AnnotationMgr::remove(const Coords &coords, MapTile tile)
{
    const Annotation look_for(coords, tile);
    remove(look_for);
}

void AnnotationMgr::remove(const Annotation &a)
{
    const auto i = std::find(annotations.cbegin(), annotations.cend(), a);
    if (i != annotations.cend()) {
        annotations.erase(i);
    }
}


/**
 * Removes an entire list of annotations
 */
void AnnotationMgr::remove(const Annotation::List &l)
{
    for (const auto &i: l) {
        remove(i);
    }
}


/**
 * Returns the number of annotations on the map
 */
std::size_t AnnotationMgr::size() const
{
    return annotations.size();
}
