/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "u4.h"

#include "shrine.h"

#include "annotation.h"
#include "context.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "monster.h"
#include "music.h"
#include "names.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "types.h"

int shrineHandleVirtue(string *message);
int shrineHandleCycles(int choice);
void shrineMeditationCycle();
void shrineTimer(void *data);
int shrineHandleMantra(string *message);
void shrineEject();

/**
 * Key handlers
 */ 
bool shrineVision(int key, void *data);
bool shrineEjectOnKey(int key, void *data);

const Shrine *shrine;
string virtueBuffer;
int cycles, completedCycles;
int elevated;
string mantraBuffer;
int reps;
string *shrineAdvice = NULL;

int shrineCanEnter(const Portal *p) {
    if (!playerCanEnterShrine(mapMgrGetById(p->destid)->shrine->virtue)) {
        screenMessage("Thou dost not bear the rune of entry!  A strange force keeps you out!\n");
        return 0;
    }
    return 1;
}

void shrineEnter(const Shrine *s) {
    U4FILE *avatar;
    Object *obj;

    if (!shrineAdvice) {
        avatar = u4fopen("avatar.exe");
        if (!avatar)
            return;
        shrineAdvice = u4read_stringtable(avatar, 93682, 24);
        u4fclose(avatar);
    }

    shrine = s;    

    /* Add-on shrine sequence START */
    if (settings.enhancements && settings.enhancementsOptions.u5shrines) {
        /* replace the 'static' avatar tile with grass */        
        c->location->map->annotations->add(MapCoords(5, 6, c->location->coords.z), GRASS_TILE, true);

        screenDisableCursor();
        screenMessage("You approach\nthe ancient\nshrine...\n");
        gameUpdateScreen(); eventHandlerSleep(1000);
        
        obj = mapAddMonsterObject(c->location->map, monsterById(BEGGAR_ID), MapCoords(5, 10, c->location->coords.z));
        obj->setTile(AVATAR_TILE);

        gameUpdateScreen(); eventHandlerSleep(400);        
        obj->move(DIR_NORTH); gameUpdateScreen(); eventHandlerSleep(400);
        obj->move(DIR_NORTH); gameUpdateScreen(); eventHandlerSleep(400);
        obj->move(DIR_NORTH); gameUpdateScreen(); eventHandlerSleep(400);
        c->location->map->annotations->remove(MapCoords(5, 6, c->location->coords.z), GRASS_TILE);
        obj->move(DIR_NORTH); gameUpdateScreen(); eventHandlerSleep(800);
        obj->setTile(monsterById(BEGGAR_ID)->tile); gameUpdateScreen();
        
        screenMessage("\n...and kneel before the altar.\n");        
        eventHandlerSleep(1000);
        screenEnableCursor();
        screenMessage("\nUpon which virtue dost thou meditate?\n");        
    }
    /* Add-on shrine sequence END */
    else  
        screenMessage("You enter the ancient shrine and sit before the altar...\nUpon which virtue dost thou meditate?\n");

    gameGetInput(&shrineHandleVirtue, &virtueBuffer);
}

int shrineHandleVirtue(string *message) {
    GetChoiceActionInfo *info;

    eventHandlerPopKeyHandler();

    screenMessage("\n\nFor how many Cycles (0-3)? ");

    info = new GetChoiceActionInfo;
    info->choices = "0123\015\033";
    info->handleChoice = &shrineHandleCycles;
    eventHandlerPushKeyHandlerWithData(&keyHandlerGetChoice, info);

    return 1;
}

int shrineHandleCycles(int choice) {
    eventHandlerPopKeyHandler();

    if (choice == '\033' || choice == '\015')
        cycles = 0;
    else
        cycles = choice - '0';
    completedCycles = 0;

    screenMessage("%c\n\n", cycles + '0');

    if (strncasecmp(virtueBuffer.c_str(), getVirtueName(shrine->virtue), 6) != 0 || cycles == 0) {
        screenMessage("Thou art unable to focus thy thoughts on this subject!\n");
        shrineEject();
    } else {
        if (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) >= 0x10000) || (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff) != c->saveGame->lastmeditation)) {
            screenMessage("Begin Meditation\n");
            shrineMeditationCycle();
        }
        else { 
            screenMessage("Thy mind is still weary from thy last Meditation!\n");
            shrineEject();
        }
    }

    return 1;
}

void shrineMeditationCycle() {
    /* find our interval for meditation */
    int interval = (settings.shrineTime * 1000) / MEDITATION_MANTRAS_PER_CYCLE;
    interval -= (interval % eventTimerGranularity);
    if (interval < eventTimerGranularity)
        interval = eventTimerGranularity;

    reps = 0;

    c->saveGame->lastmeditation = (c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff;

    screenDisableCursor();
    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&shrineTimer, interval);
}

void shrineTimer(void *data) {
    if (reps++ >= MEDITATION_MANTRAS_PER_CYCLE) {
        eventHandlerRemoveTimerCallback(&shrineTimer);
        eventHandlerPopKeyHandler();

        screenMessage("\nMantra: ");

        gameGetInput(&shrineHandleMantra, &mantraBuffer, 3);
        screenRedrawScreen();
    }
    else {
        screenDisableCursor();
        screenMessage(".");
        screenRedrawScreen();
    }
}

int shrineHandleMantra(string *message) {
    eventHandlerPopKeyHandler();

    screenMessage("\n");

    if (strcasecmp(mantraBuffer.c_str(), shrine->mantra) != 0) {
        playerAdjustKarma(KA_BAD_MANTRA);
        screenMessage("Thou art not able to focus thy thoughts with that Mantra!\n");
        shrineEject();
    }
    else if (--cycles > 0) {
        completedCycles++;
        playerAdjustKarma(KA_MEDITATION);
        shrineMeditationCycle();
    }
    else {
        completedCycles++;
        playerAdjustKarma(KA_MEDITATION);

        elevated = completedCycles == 3 && playerAttemptElevation(shrine->virtue);
        if (elevated)
            screenMessage("\nThou hast achieved partial Avatarhood in the Virtue of %s\n\n",
                          getVirtueName(shrine->virtue));
        else
            screenMessage("\nThy thoughts are pure. "
                          "Thou art granted a vision!\n");
        eventHandlerPushKeyHandler(&shrineVision);
    }

    return 1;
}

bool shrineVision(int key, void *data) {
    if (elevated) {
        screenMessage("Thou art granted a vision!\n");
        gameSetViewMode(VIEW_RUNE);
        screenDrawBackgroundInMapArea((BackgroundType)(BKGD_SHRINE_HON + shrine->virtue));
    }
    else {
        screenMessage("\n%s", shrineAdvice[shrine->virtue * 3 + completedCycles - 1].c_str());
    }
    eventHandlerPopKeyHandler();
    eventHandlerPushKeyHandler(&shrineEjectOnKey);
    return true;
}

bool shrineEjectOnKey(int key, void *data) {
    gameSetViewMode(VIEW_NORMAL);
    eventHandlerPopKeyHandler();
    shrineEject();
    return true;
}

void shrineEject() {
    gameExitToParentMap();
    musicPlay();
    (*c->location->finishTurn)();
}