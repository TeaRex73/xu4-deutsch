/*
 * $Id$
 */

#ifndef CHEAT_H
#define CHEAT_H

#include <string>

#include "controller.h"


class GameController;

class CheatMenuController:public WaitableController<void *> {
public:
    explicit CheatMenuController(GameController *game);
    CheatMenuController(const CheatMenuController &) = delete;
    CheatMenuController(CheatMenuController &&) = delete;
    CheatMenuController &operator=(const CheatMenuController &) = delete;
    CheatMenuController &operator=(CheatMenuController &&) = delete;
    virtual bool keyPressed(int key) override;

private:
    static void summonCreature(const std::string &name);
    GameController *game;
};


/**
 * This class controls the wind option from the cheat menu.  It
 * handles setting the wind direction as well as locking/unlocking.
 * The value field of WaitableController isn't used.
 */
class WindCmdController:public WaitableController<void *> {
public:
    virtual bool keyPressed(int key) override;
};

#endif /* CHEAT_H */
