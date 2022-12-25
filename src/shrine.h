/*
 * $Id$
 */

#ifndef SHRINE_H
#define SHRINE_H

#include "map.h"
#include "portal.h"
#include "savegame.h"

#define SHRINE_MEDITATION_INTERVAL 100
#define MEDITATION_MANTRAS_PER_CYCLE 16

class Shrine:public Map {
public:
    Shrine()
        :name(),
         virtue(VIRT_MAX),
         mantra()
    {
    }

    virtual std::string getName();
    Virtue getVirtue() const;
    std::string getMantra() const;
    void setVirtue(Virtue v);
    void setMantra(std::string mantra);
    void enter();
    void enhancedSequence();
    void meditationCycle();
    void askMantra();
    void eject();
    void showVision(bool elevated);

private:
    std::string name;
    Virtue virtue;
    std::string mantra;
};

bool shrineCanEnter(const Portal *p);
bool isShrine(Map *punknown);

#endif // ifndef SHRINE_H
