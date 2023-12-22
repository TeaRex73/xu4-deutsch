/*
 * $Id$
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <list>

#include "coords.h"
#include "types.h"

class Annotation;


/**
 * Annotation are updates to a map.
 * There are three types of annotations:
 * - permanent: lasts until annotationClear() is called
 * - turn based: lasts a given number of cycles
 * - time based: lasts a given number of time units (1/4 seconds)
 */
class Annotation {
public:
    typedef std::list<Annotation> List;
    Annotation(
        const Coords &coords,
        MapTile tile,
        bool visual = false,
        bool coverUp = false
    );
    void debug_output() const;

    const Coords &getCoords() const
    {
        return coords;    /**< Returns coordinates of annotation */
    }

    MapTile getTile() const
    {
        return tile;      /**< Returns the annotation's tile */
    }

    bool isVisualOnly() const
    {
        return visual;    /**< Tells if visual-only annotation */
    }

    int getTTL() const
    {
        return ttl; /**< Returns number of turns of annotation */
    }

    bool isCoverUp() const
    {
        return coverUp;
    }

    void setCoords(const Coords &c)
    {
        coords = c;       /**< Sets coords for the annotation */
    }

    void setTile(MapTile t)
    {
        tile = t;         /**< Sets tile for the annotation */
    }

    void setVisualOnly(bool v)
    {
        visual = v;       /**< Sets if annotation is visual-only */
    }

    void setTTL(int turns)
    {
        ttl = turns;      /**< Sets number of turns of annotation */
    }

    void passTurn()
    {
        if (ttl > 0) {
            ttl--;         /**< Passes a turn for the annotation */
        }
    }

    bool operator==(const Annotation &) const;

private:
    Coords coords;
    MapTile tile;
    bool visual;
    int ttl;
    bool coverUp;
};


/**
 * Manages annotations for the current map.  This includes
 * adding and removing annotations, as well as finding annotations
 * and managing their existence.
 */
class AnnotationMgr {
public:
    AnnotationMgr();
    Annotation *add(
        const Coords &coords,
        MapTile tile,
        bool visual = false,
        bool coverUp = false
    );
    Annotation::List allAt(const Coords &coords) const;
    std::list<const Annotation *> ptrsToAllAt(const Coords &coords) const;
    void clear();
    void passTurn();
    void remove(const Coords &coords, MapTile tile);
    void remove(const Annotation &a);
    void remove(const Annotation::List &l);
    int size() const;

private:
    Annotation::List annotations;
};

#endif // ifndef ANNOTATION_H
