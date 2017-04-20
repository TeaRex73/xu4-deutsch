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
    CheatMenuController(GameController *game);
	CheatMenuController(const CheatMenuController &) = delete;
	CheatMenuController(CheatMenuController &&) = delete;
	CheatMenuController &operator=(const CheatMenuController &) = delete;
	CheatMenuController &operator=(CheatMenuController &&) = delete;
    bool keyPressed(int key);

private:
    void summonCreature(const std::string &name);
    GameController *game;
};


/**
 * This class controls the wind option from the cheat menu.  It
 * handles setting the wind direction as well as locking/unlocking.
 * The value field of WaitableController isn't used.
 */
class WindCmdController:public WaitableController<void *> {
public:
    bool keyPressed(int key);
};

#endif /* CHEAT_H */
