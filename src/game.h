/*
 * $Id$
 */

#ifndef GAME_H
#define GAME_H

#include <vector>

#include "controller.h"
#include "event.h"
#include "map.h"
#include "observer.h"
#include "sound.h"
#include "tileview.h"
#include "types.h"

class Map;
struct Portal;
class Creature;
class Location;
class MoveEvent;
class Party;
class PartyEvent;
class PartyMember;

typedef enum {
    VIEW_NORMAL,
    VIEW_GEM,
    VIEW_RUNE,
    VIEW_DUNGEON,
    VIEW_DEAD,
    VIEW_CODEX,
    VIEW_MIXTURES
} ViewMode;

/**
 * A controller to read a player number.
 */
class ReadPlayerController:public ReadChoiceController {
public:
    ReadPlayerController();
    ~ReadPlayerController();
    virtual bool keyPressed(int key) override;
    int getPlayer() const;
    virtual int waitFor() override;
};


/**
 * A controller to handle input for commands requiring a letter
 * argument in the range 'a' - lastValidLetter.
 */
class AlphaActionController:public WaitableController<int> {
public:
    AlphaActionController(char letter, const std::string &p)
        :lastValidLetter(letter), prompt(p)
    {
    }

    virtual bool keyPressed(int key) override;
    static int get(
        char lastValidLetter,
        const std::string &prompt,
        EventHandler *eh = nullptr
    );

private:
    char lastValidLetter;
    std::string prompt;
};


/**
 * Controls interaction while Ztats are being displayed.
 */
class ZtatsController:public WaitableController<void *> {
public:
    virtual bool keyPressed(int key) override;
};


class TurnCompleter {
public:
    virtual ~TurnCompleter()
    {
    }

    virtual void finishTurn() = 0;
};


/**
 * The main game controller that handles basic game flow and keypresses.
 *
 * @todo
 *  <ul>
 *      <li>separate the dungeon specific stuff
 *          into another class (subclass?)</li>
 *  </ul>
 */
class GameController
    :public Controller,
     public Observer<Party *, PartyEvent &>,
     public Observer<Location *, MoveEvent &>,
     public TurnCompleter {
public:
    GameController();
    ~GameController();
    /* controller functions */
    virtual bool keyPressed(int key) override;
    virtual void timerFired() override;
    /* main game functions */
    void init();
    static void initScreen();
    static void initScreenWithoutReloadingState();
    void setMap(
        Map *map,
        bool saveLocation,
        const Portal *portal,
        TurnCompleter *turnCompleter = nullptr
    );
    bool exitToParentMap(bool shouldQuenchTorch = true);
    virtual void finishTurn() override;
    virtual void update(Party *party, PartyEvent &event) override;
    virtual void update(Location *location, MoveEvent &event) override;
    static void initMoons();
    static void updateMoons(bool showmoongates);
    static void flashTile(
        const Coords &coords, MapTile tile, int frames
    );
    static void flashTile(
        const Coords &coords, const std::string &tilename, int timeFactor
    );
    static void doScreenAnimationsWhilePausing(int timeFactor);
    TileView mapArea;
    bool paused;
    int pausedTimer;

private:
    void avatarMoved(MoveEvent &event);
    void avatarMovedInDungeon(const MoveEvent &event);
    static void creatureCleanup(bool allCreatures = false);
    static void checkBridgeTrolls();
    static void checkRandomCreatures();
    static void checkSpecialCreatures(Direction dir);
    bool checkMoongates();
    static bool createBalloon(Map *map);
};

extern GameController *game;
/* map and screen functions */
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen();
/* spell functions */
void castSpell(int player = -1);
void gameSpellEffect(int spell, int player, Sound sound);
/* action functions */
void destroy();
void attack();
void board();
void fire();
void getChest(int player = -1);
void holeUp();
void jimmy();
void opendoor();
bool gamePeerCity(int city, void *data);
void peer(bool useGem = true);
void talk();
bool fireAt(const Coords &coords, bool originAvatar);
Direction gameGetDirection();
void readyWeapon(int player = -1);
/* checking functions */
void gameCheckHullIntegrity();
/* creature functions */
bool creatureRangeAttack(const Coords &coords, const Creature *m);
void gameCreatureCleanup();
bool gameSpawnCreature(const class Creature *m);
/* etc */
std::string gameGetInput(int maxlen = 32);
int gameGetPlayer(
    bool canBeDisabled, bool canBeActivePlayer, bool zeroIsValid
);
void gameGetPlayerForCommand(
    bool (*commandFn)(int player), bool canBeDisabled, bool canBeActivePlayer
);
void gameDamageParty(int minDamage, int maxDamage);
void gameDamageShip(int minDamage, int maxDamage);
void gameSetActivePlayer(int player);
std::vector<Coords> gameGetDirectionalActionPath(
    int dirmask,
    int validDirections,
    const Coords &origin,
    int minDistance,
    int maxDistance,
    bool (*blockedPredicate)(const Tile *tile),
    bool includeBlocked
);
void gameDestroyAllCreatures(void);

#endif // ifndef GAME_H
