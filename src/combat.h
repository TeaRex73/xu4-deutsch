/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include "controller.h"
#include "coords.h"
#include "creature.h"
#include "direction.h"
#include "game.h"
#include "map.h"
#include "observer.h"
#include "player.h"
#include "savegame.h"
#include "types.h"

class CombatMap;
class MoveEvent;
class Object;
class Tile;
class Weapon;


#define AREA_CREATURES 16
#define AREA_PLAYERS 8

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
    // can't be copied
    CombatController(const CombatController &) = delete;
    CombatController(CombatController &&) = delete;
    CombatController &operator=(const CombatController &) = delete;
    CombatController &operator=(CombatController &&) = delete;

    explicit CombatController(CombatMap *m);
    explicit CombatController(MapId id);
    ~CombatController() override;

    bool isCombatController() const override
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
    PartyMember *getCurrentPlayer() const;
    void setExitDir(Direction d);
    void setCreature(Creature *);
    void setWinOrLose(bool wOrL = true);
    void showCombatMessage(bool show = true);
    virtual void init(Creature *m);
    void initDungeonRoom(int room, Direction from);
    void applyCreatureTileEffects() const;
    virtual void begin();
    virtual void end(bool adjustKarma);
    void fillCreatureTable(const Creature *creat);
    int initialNumberOfCreatures(const Creature *creat) const;
    bool isWon() const;
    bool isLost() const;
    void moveCreatures();
    void placeCreatures() const;
    void placePartyMembers();
    bool setActivePlayer(int player);
    static bool attackHit(
        const Creature *attacker, const Creature *defender, bool harder
    );
    virtual void awardLoot();
    void attack() const;
    bool attackAt(
        const Coords &coords,
        PartyMember *attacker,
        int dir,
        int range,
        int distance
    ) const;
    bool rangedAttack(const Coords &coords, Creature *attacker) const;
    void rangedMiss(const Coords &coords, const Creature *attacker) const;

    void returnWeaponToOwner(
        const Coords &coords, int distance, int dir, const Weapon *weapon
    ) const;
    bool keyPressed(int key) override;
    void finishTurn() override;
    void movePartyMember(const MoveEvent &event) const;
    void update(Party *party, PartyEvent &event) override;

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
};


/**
 * CombatMap class
 */
class CombatMap:public Map {
public:
    CombatMap();
    CreatureVector getCreatures() const;
    PartyMemberVector getPartyMembers() const;
    PartyMember *partyMemberAt(const Coords &coords) const;
    Creature *creatureAt(const Coords &coords) const;
    static MapId mapForTile(
        const Tile *groundTile, const Tile *transport, const Object *obj
    );

    bool isDungeonRoom() const
    {
        return dungeonRoom;
    }

    bool isAltarRoom() const
    {
        return altarRoom != VIRTUE_NONE;
    }

    bool isContextual() const
    {
        return contextual;
    }

    BaseVirtue getAltarRoom() const
    {
        return altarRoom;
    }

    void setAltarRoom(const BaseVirtue ar)
    {
        altarRoom = ar;
    }

    void setDungeonRoom(const bool d)
    {
        dungeonRoom = d;
    }

    void setContextual(const bool c)
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


CombatMap *getCombatMap(Map *pUnknown = nullptr);

#endif // COMBAT_H
