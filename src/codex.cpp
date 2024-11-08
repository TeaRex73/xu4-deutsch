/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
#include <cstring>
#include <vector>

#include "codex.h"

#include "context.h"
#include "event.h"
#include "game.h"
#include "item.h"
#include "imagemgr.h"
#include "names.h"
#include "savegame.h"
#include "screen.h"
#include "stats.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"

using namespace std;

int codexInit();
void codexDelete();
void codexEject(CodexEjectCode code);
void codexHandleWOP(const string &word);
void codexHandleVirtues(const string &virtue);
void codexHandleInfinity(const string &answer);
void codexImpureThoughts();

/**
 * Key handlers
 */
bool codexHandleInfinityAnyKey(int key, void *data);
bool codexHandleEndgameAnyKey(int key, void *data);

vector<string> codexVirtueQuestions;
vector<string> codexEndgameText1;
vector<string> codexEndgameText2;

/**
 * Initializes the Chamber of the Codex sequence (runs from codexStart())
 */
int codexInit() {
    U4FILE *codextext;

    codextext = u4fopen("codex.ger");
    if (!codextext)
        return 0;

    codexVirtueQuestions = u4read_stringtable(codextext, 0, 11);
    codexEndgameText1 = u4read_stringtable(codextext, -1, 7);
    codexEndgameText2 = u4read_stringtable(codextext, -1, 5);

    u4fclose(codextext);

    return 1;
}

/**
 * Frees all memory associated with the Codex sequence
 */
void codexDelete() {
    codexVirtueQuestions.clear();
    codexEndgameText1.clear();
    codexEndgameText2.clear();
}

/**
 * Begins the Chamber of the Codex sequence
 */
void codexStart() {

    c->willPassTurn = false;
    codexInit();

    /**
     * disable the whirlpool cursor and black out the screen
     */
    screenDisableCursor();
    screenUpdate(&game->mapArea, false, true);

    /**
     * make the avatar alone
     */
    c->stats->setView(STATS_PARTY_OVERVIEW);
    c->stats->update(true);     /* show just the avatar */
    screenRedrawScreen();

    /**
     * change the view mode so the dungeon doesn't get shown
     */
    gameSetViewMode(VIEW_CODEX);

    screenMessage("\n\n\n\nPl|tztlich ist es dunkel, und du findest dich allein in einer leeren Kammer wieder.\n");
    EventHandler::sleep(4000);

    /**
     * check to see if you have the 3-part key
     */
    if ((c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) != (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        codexEject(CODEX_EJECT_NO_3_PART_KEY);
        return;
    }

    screenDrawImageInMapArea(BKGD_KEY);
    screenRedrawMapArea();

    screenMessage("\nDu benutzt deinen Dreiteiligen Schl}ssel.\n");
    EventHandler::sleep(3000);

    screenMessage("\nEine Stimme erschallt:\n\"Wie lautet das Wort des Durchlasses?\"\n\n");

    /**
     * Get the Word of Passage
     */
    codexHandleWOP(gameGetInput());
}

/**
 * Ejects you from the chamber of the codex (and the Abyss, for that matter)
 * with the correct message.
 */
void codexEject(CodexEjectCode code) {
    struct {
        int x, y;
    } startLocations[] = {
        { 231, 136 },
        { 83, 105 },
        { 35, 221 },
        { 59, 44 },
        { 158, 21 },
        { 105, 183 },
        { 23, 129 },
        { 186, 171 }
    };

    switch(code) {
    case CODEX_EJECT_NO_3_PART_KEY:
        screenMessage("\nDu hast den Dreiteiligen Schl}ssel nicht.\n\n");
        break;
    case CODEX_EJECT_NO_FULL_PARTY:
      screenMessage("\nDu hast nicht in allen acht Tugenden deine F}hrung erwiesen.\n\n");
      EventHandler::sleep(2000);
      screenMessage("\nDurchla~ wird nicht gew{hrt.\n\n");
      break;
    case CODEX_EJECT_NO_FULL_AVATAR:
        screenMessage("\nDu bist nicht bereit.\n");
        EventHandler::sleep(2000);
        screenMessage("\nDurchla~ wird nicht gew{hrt.\n\n");
        break;
    case CODEX_EJECT_BAD_WOP:
        screenMessage("\nDurchla~ wird nicht gew{hrt.\n\n");
        break;
    case CODEX_EJECT_HONESTY:
    case CODEX_EJECT_COMPASSION:
    case CODEX_EJECT_VALOR:
    case CODEX_EJECT_JUSTICE:
    case CODEX_EJECT_SACRIFICE:
    case CODEX_EJECT_HONOR:
    case CODEX_EJECT_SPIRITUALITY:
    case CODEX_EJECT_HUMILITY:
    case CODEX_EJECT_TRUTH:
    case CODEX_EJECT_LOVE:
    case CODEX_EJECT_COURAGE:
        screenMessage("\nDeine Queste ist noch nicht vollendet.\n\n");
        break;
    case CODEX_EJECT_BAD_INFINITY:
        screenMessage("\nDu kennst die wahre Natur des Universums nicht.\n\n");
        break;
    default:
        screenMessage("\nUh-oh, Du bist gerade allzu nahe an die L|sung des Spiels herangekommen.\nB\\SER AVATAR!\n");
        break;
    }

    EventHandler::sleep(2000);

    /* free memory associated with the Codex */
    codexDelete();

    /* re-enable the cursor and show it */
    screenEnableCursor();
    screenShowCursor();

    /* return view to normal and exit the Abyss */
    gameSetViewMode(VIEW_NORMAL);
    game->exitToParentMap();
    musicMgr->play();

    /**
     * if being ejected because of a missed virtue question,
     * then teleport the party to the starting location for
     * that virtue.
     */
    if (code >= CODEX_EJECT_HONESTY && code <= CODEX_EJECT_HUMILITY) {
        int virtue = code - CODEX_EJECT_HONESTY;
        c->location->coords.x = startLocations[virtue].x;
        c->location->coords.y = startLocations[virtue].y;
    }

    /* finally, finish the turn */
    c->location->turnCompleter->finishTurn();
    eventHandler->setController(game);
    c->lastCommandTime = (long)time(NULL);
    c->willPassTurn = true;
}

/**
 * Handles entering the Word of Passage
 */
void codexHandleWOP(const string &word) {
    static int tries = 1;
    int i;

    eventHandler->popKeyHandler();

    /* slight pause before continuing */
    screenMessage("\n");
    screenDisableCursor();
    EventHandler::sleep(1000);

    /* entered correctly */
    if (strcasecmp(deumlaut(word).c_str(), "veramocor") == 0) {
        tries = 1; /* reset 'tries' in case we need to enter this again later */

        /* eject them if they don't have all 8 party members */
        if (c->saveGame->members != 8) {
            codexEject(CODEX_EJECT_NO_FULL_PARTY);
            return;
        }

        /* eject them if they're not a full avatar at this point */
        for (i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] != 0) {
                codexEject(CODEX_EJECT_NO_FULL_AVATAR);
                return;
            }
        }

        screenMessage("\nDurchla~ wird gew{hrt.\n");
        EventHandler::sleep(4000);

        screenEraseMapArea();
        screenRedrawMapArea();

        /* Ask the Virtue questions */
        screenMessage("\n\nDie Stimme fragt:\n");
        EventHandler::sleep(2000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[0].c_str());

        codexHandleVirtues(gameGetInput());

        return;
    }

    /* entered incorrectly - give 3 tries before ejecting */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("\"Wie lautet das Wort des Durchlasses?\"\n\n");
        codexHandleWOP(gameGetInput());
    }

    /* 3 tries are up... eject! */
    else {
        tries = 1;
        codexEject(CODEX_EJECT_BAD_WOP);
    }
}

/**
 * Handles naming of virtues in the Chamber of the Codex
 */
void codexHandleVirtues(const string &virtue) {
    static const char *codexImageNames[] = {
        BKGD_HONESTY, BKGD_COMPASSN, BKGD_VALOR, BKGD_JUSTICE,
        BKGD_SACRIFIC, BKGD_HONOR, BKGD_SPIRIT, BKGD_HUMILITY,
        BKGD_TRUTH, BKGD_LOVE, BKGD_COURAGE
    };

    static int current = 0;
    static int tries = 1;

    eventHandler->popKeyHandler();

    /* slight pause before continuing */
    screenMessage("\n");
    screenDisableCursor();
    EventHandler::sleep(1000);

    /* answered with the correct one of eight virtues */
    if ((current < VIRT_MAX) &&
        (strcasecmp(deumlaut(virtue).c_str(), deumlaut(getVirtueName(static_cast<Virtue>(current))).c_str()) == 0)) {

        screenDrawImageInMapArea(codexImageNames[current]);
        screenRedrawMapArea();

        current++;
        tries = 1;

        EventHandler::sleep(2000);

        if (current == VIRT_MAX) {
            screenMessage("\nDu bist wohlversiert in den Tugenden des Avatars.\n");
            EventHandler::sleep(5000);
        }

        screenMessage("\n\nDie Stimme fragt:\n");
        EventHandler::sleep(2000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[current].c_str());
        codexHandleVirtues(gameGetInput());
    }

    /* answered with the correct base virtue (truth, love, courage) */
    else if ((current >= VIRT_MAX) &&
             (strcasecmp(deumlaut(virtue).c_str(), deumlaut(getBaseVirtueName(static_cast<BaseVirtue>(1 << (current - VIRT_MAX)))).c_str()) == 0)) {

        screenDrawImageInMapArea(codexImageNames[current]);
        screenRedrawMapArea();

        current++;
        tries = 1;

        if (current < VIRT_MAX+3) {
            screenMessage("\n\nDie Stimme fragt:\n");
            EventHandler::sleep(2000);
            screenMessage("\n%s\n\n", codexVirtueQuestions[current].c_str());
            codexHandleVirtues(gameGetInput());
        }
        else {
            screenMessage("\nDer Boden erbebt unter deinen F}~en.\n");
            EventHandler::sleep(1000);
            screenShake(10);

            EventHandler::sleep(3000);
            screenEnableCursor();
            screenMessage("\n]ber dem Get|se fragt die Stimme:\n\nWenn alle acht Tugenden des Avatars sich vereinen zu, und abgeleitet sind von, den Drei Prinzipien der Wahrheit, der Liebe und des Mutes...");
            eventHandler->pushKeyHandler(&codexHandleInfinityAnyKey);
        }
    }

    /* give them 3 tries to enter the correct virtue, then eject them! */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("%s\n\n", codexVirtueQuestions[current].c_str());
        codexHandleVirtues(gameGetInput());
    }

    /* failed 3 times... eject! */
    else {
        codexEject(static_cast<CodexEjectCode>(CODEX_EJECT_HONESTY + current));

        tries = 1;
        current = 0;
    }
}

bool codexHandleInfinityAnyKey(int key, void *data) {
    eventHandler->popKeyHandler();

    screenMessage("\n\nWelches ist dann Das Eine, welches umfa~t, und das Ganze ist,\n\naller unabweisbaren Wahrheit, aller unendlichen Liebe, und allen unbeugsamen Mutes?\n\n");
    codexHandleInfinity(gameGetInput());
    return true;
}

void codexHandleInfinity(const string &answer) {
    static int tries = 1;

    eventHandler->popKeyHandler();
    /* slight pause before continuing */
    screenMessage("\n");
    screenDisableCursor();
    EventHandler::sleep(1000);

    if (strcasecmp(deumlaut(answer).c_str(), "infinity") == 0) {
        EventHandler::sleep(2000);
        screenShake(10);

        screenEnableCursor();
        screenMessage("\n%s", codexEndgameText1[0].c_str());
        eventHandler->pushKeyHandler(&codexHandleEndgameAnyKey);
    }
    else if (tries++ < 3) {
        codexImpureThoughts();
	screenMessage("\n]ber dem Get|se fragt die Stimme:\n\nWenn alle acht Tugenden des Avatars sich vereinen zu, und abgeleitet sind von, den Drei Prinzipien der Wahrheit, der Liebe und des Mutes...");
        eventHandler->pushKeyHandler(&codexHandleInfinityAnyKey);
    }
    else codexEject(CODEX_EJECT_BAD_INFINITY);
}

bool codexHandleEndgameAnyKey(int key, void *data) {
    static int index = 1;

    eventHandler->popKeyHandler();

    if (index < 10) {

        if (index < 7) {
            if (index == 6) {
                screenEraseMapArea();
                screenRedrawMapArea();
            }
            screenMessage("%s", codexEndgameText1[index].c_str());
        }
        else if (index == 7) {
            screenDrawImageInMapArea(BKGD_STONCRCL);
            screenRedrawMapArea();
            screenMessage("\n\n%s", codexEndgameText2[0].c_str());
        }
        else if (index > 7)
            screenMessage("%s", codexEndgameText2[index-7].c_str());

        index++;
        eventHandler->pushKeyHandler(&codexHandleEndgameAnyKey);
    }
    else {
        /* CONGRATULATIONS!... you have completed the game in x turns */
        screenDisableCursor();
        screenMessage("%s%d%s", codexEndgameText2[index-7].c_str(), c->saveGame->moves, codexEndgameText2[index-6].c_str());
        eventHandler->pushKeyHandler(&KeyHandler::ignoreKeys);
    }

    return true;
}

/**
 * Pretty self-explanatory
 */
void codexImpureThoughts() {
    screenMessage("\nDeine Gedanken sind nicht rein.\nIch frage wiederum.\n");
    EventHandler::sleep(2000);
}
