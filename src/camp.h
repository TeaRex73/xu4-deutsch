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
    void init(Creature *m) override;
    void begin() override;
    void end(bool adjustKarma) override;

private:
    static bool heal();
};

class InnController:public CombatController {
public:
    InnController();
    void begin() override;
    void awardLoot() override;

private:
    static bool heal();
    static bool maybeMeetIsaac();
    void maybeAmbush();
};

#endif // CAMP_H
