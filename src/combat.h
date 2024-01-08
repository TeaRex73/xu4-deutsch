/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include <map>

#include "direction.h"
#include "map.h"
#include "controller.h"
#include "creature.h"
#include "game.h"
#include "object.h"
#include "observer.h"
#include "player.h"
#include "savegame.h"
#include "types.h"

#define AREA_CREATURES 16
#define AREA_PLAYERS 8

class CombatMap;
class Creature;
class MoveEvent;
class Weapon;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT
} CombatAction;


/**
 * CombatController class
 */
class CombatController
    :public Controller,
     public Observer<Party *, PartyEvent &>,
     public TurnCompleter {
public:
    //Uncopyable
    CombatController(const CombatController &) = delete;
    CombatController(CombatController &&) = delete;
    CombatController &operator=(const CombatController &) = delete;
    CombatController &operator=(CombatController &&) = delete;

    explicit CombatController(CombatMap *m);
    explicit CombatController(MapId id);
    virtual ~CombatController();

    virtual bool isCombatController() const override
    {
        return true;
    }

    bool isCamping() const;
    bool isWinOrLose() const;
    Direction getExitDir() const;
    unsigned char getFocus() const;
    CombatMap *getMap() const;
    Creature *getCreature() const;
    PartyMemberVector *getParty();
    PartyMember *getCurrentPlayer();
    void setExitDir(Direction d);
    void setCreature(Creature *);
    void setWinOrLose(bool worl = true);
    void showCombatMessage(bool show = true);
    virtual void init(Creature *m);
    void initDungeonRoom(int room, Direction from);
    void applyCreatureTileEffects();
    virtual void begin();
    virtual void end(bool adjustKarma);
    void fillCreatureTable(const Creature *creature);
    int initialNumberOfCreatures(const Creature *creature) const;
    bool isWon() const;
    bool isLost() const;
    void moveCreatures();
    void placeCreatures();
    void placePartyMembers();
    bool setActivePlayer(int player);
    static bool attackHit(
        const Creature *attacker, const Creature *defender, bool harder
    );
    virtual void awardLoot();
    void attack();
    bool attackAt(
        const Coords &coords,
        PartyMember *attacker,
        int dir,
        int range,
        int distance
    );
    bool rangedAttack(const Coords &coords, Creature *attacker);
    void rangedMiss(const Coords &coords, const Creature *attacker);
    bool returnWeaponToOwner(
        const Coords &coords, int distance, int dir, const Weapon *weapon
    );
#if 0
    static void attackFlash(
        const Coords &coords, MapTile tile, int timeFactor
    );
    static void attackFlash(
        const Coords &coords, const std::string &tilename, int timeFactor
    );
#endif
    static void doScreenAnimationsWhilePausing(int timeFactor);
    virtual bool keyPressed(int key) override;
    virtual void finishTurn() override;
    void movePartyMember(const MoveEvent &event);
    virtual void update(Party *party, PartyEvent &event) override;

protected:
    CombatController();
    CombatMap *map;
    PartyMemberVector party;
    unsigned char focus;
    const Creature *creatureTable[AREA_CREATURES];
    Creature *creature;
    bool camping;
    bool forceStandardEncounterSize;
    bool placePartyOnMap;
    bool placeCreaturesOnMap;
    bool winOrLose;
    bool showMessage;
    Direction exitDir;

private:
};


/**
 * CombatMap class
 */
class CombatMap:public Map {
public:
    CombatMap();
    CreatureVector getCreatures();
    PartyMemberVector getPartyMembers();
    PartyMember *partyMemberAt(const Coords &coords);
    Creature *creatureAt(const Coords &coords);
    static MapId mapForTile(
        const Tile *groundTile, const Tile *transport, const Object *obj
    );

    bool isDungeonRoom() const
    {
        return dungeonRoom;
    }

    bool isAltarRoom() const
    {
        return altarRoom != VIRT_NONE;
    }

    bool isContextual() const
    {
        return contextual;
    }

    BaseVirtue getAltarRoom() const
    {
        return altarRoom;
    }

    void setAltarRoom(BaseVirtue ar)
    {
        altarRoom = ar;
    }

    void setDungeonRoom(bool d)
    {
        dungeonRoom = d;
    }

    void setContextual(bool c)
    {
        contextual = c;
    }

    Coords creature_start[AREA_CREATURES];
    Coords player_start[AREA_PLAYERS];

protected:
    bool dungeonRoom;
    BaseVirtue altarRoom;
    bool contextual;
};


bool isCombatMap(Map *punknown);
CombatMap *getCombatMap(Map *punknown = nullptr);

#endif // ifndef COMBAT_H
