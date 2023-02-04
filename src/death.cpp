/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <atomic>
#include <ctime>
#include "u4.h"

#include "death.h"

#include "map.h"
#include "annotation.h"
#include "city.h"
#include "context.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "music.h"
#include "player.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "stats.h"
#include "utils.h"

#define REVIVE_WORLD_X 86
#define REVIVE_WORLD_Y 107
#define REVIVE_CASTLE_X 19
#define REVIVE_CASTLE_Y 8

static int timerCount;
static unsigned int timerMsg;
std::atomic_bool deathSequenceRunning(false);

static void deathTimer(void *data);
static void deathRevive(void);

const struct {
    int timeout; /* pause in seconds */
    const char *text; /* text of message */
} deathMsgs[] = {
    { 5, "\n\nAber warte..." },
    { 5, "\nWo bin ich?..." },
    { 5, "\nBin ich tot?..." },
    { 5, "\nJenseits?..." },
    { 5, "\nDU H\\RST:\n    %s" },
    { 5, "\nIch f}hle Bewegung..." },
    {
        5,
        "\n\n\nLORD BRITISH SAGT:\n"
        "ICH HABE DEINEN GEIST UND EINIGE BESITZT]MER "
        "AUS DER LEERE GEZOGEN. SEI IN ZUKUNFT VORSICHTIGER!\n\n\020"
    }
};

#define N_MSGS (sizeof(deathMsgs) / sizeof(deathMsgs[0]))

void deathStart(int delay)
{
    if (deathSequenceRunning.exchange(true)) {
        return;
    }
    game->paused = true;
    c->willPassTurn = false;
    // stop playing music
    musicMgr->pause();
    soundStop();
    timerCount = 0;
    timerMsg = 0;
    gameSetViewMode(VIEW_DEAD);
    screenMessage("\n\nDUNKELHEIT...");
    WaitController waitCtrl(delay * settings.gameCyclesPerSecond);
    eventHandler->pushController(&waitCtrl);
    waitCtrl.wait();
    eventHandler->pushKeyHandler(&KeyHandler::ignoreKeys);
    screenDisableCursor();
    eventHandler->getTimer()->add(
        &deathTimer, settings.gameCyclesPerSecond
    );
}

static void deathTimer(void *)
{
    timerCount++;
    if ((timerMsg < N_MSGS)
        && (timerCount > deathMsgs[timerMsg].timeout)) {
        screenMessage(
            deathMsgs[timerMsg].text,
            uppercase(c->party->member(0)->getName()).c_str()
        );
        screenHideCursor();
        timerCount = 0;
        timerMsg++;
        if (timerMsg >= N_MSGS) {
            eventHandler->getTimer()->remove(&deathTimer);
            deathRevive();
        }
    }
}

static void deathRevive()
{
    while (!c->location->map->isWorldMap() && c->location->prev != nullptr) {
        game->exitToParentMap();
    }
    eventHandler->setController(game);
    gameSetViewMode(VIEW_NORMAL);
    /* Move our world map location to Lord British's Castle */
    c->location->coords = c->location->map->portals[0]->coords;
    /* Now, move the avatar into the castle and put him
       in front of Lord British */
    game->setMap(mapMgr->get(100), 1, nullptr);
    c->location->coords.x = REVIVE_CASTLE_X;
    c->location->coords.y = REVIVE_CASTLE_Y;
    c->location->coords.z = 0;
    c->aura->set();
    c->horseSpeed = 0;
    musicMgr->play();
    c->party->reviveParty();
    screenEnableCursor();
    screenShowCursor();
    c->stats->setView(STATS_PARTY_OVERVIEW);
    screenRedrawScreen();
    c->lastCommandTime = std::time(nullptr);
    c->willPassTurn = true;
    game->paused = false;
    deathSequenceRunning = false;
} // deathRevive
