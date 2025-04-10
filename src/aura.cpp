/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "aura.h"

Aura::Aura()
    :type(NONE), duration(0)
{
}

void Aura::setDuration(int d)
{
    if (duration >= d) return;
    duration = d;
    setChanged();
    notifyObservers(nullptr);
}

void Aura::set(Type t, int d)
{
    if (type == t && duration >= d) return;
    type = t;
    duration = d;
    setChanged();
    notifyObservers(nullptr);
}

void Aura::setType(Type t)
{
    if (type == t) return;
    type = t;
    setChanged();
    notifyObservers(nullptr);
}

void Aura::passTurn()
{
    if (duration > 0) {
        duration--;
        if (duration == 0) {
            type = NONE;
            setChanged();
            notifyObservers(nullptr);
        }
    }
}
