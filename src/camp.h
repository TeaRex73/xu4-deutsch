/*
 * $Id$
 */

#ifndef CAMP_H
#define CAMP_H

#include "combat.h"

/* Number of moves before camping will heal */
#define CAMP_HEAL_INTERVAL 100

class CampController:public CombatController {
public:
    CampController();
    virtual void init(Creature *m) override;
    virtual void begin() override;
    virtual void end(bool adjustKarma) override;

private:
    static bool heal();
};

class InnController:public CombatController {
public:
    InnController();
    virtual void begin() override;
    virtual void awardLoot() override;

private:
    static bool heal();
    static bool maybeMeetIsaac();
    void maybeAmbush();
};

#endif // ifndef CAMP_H
