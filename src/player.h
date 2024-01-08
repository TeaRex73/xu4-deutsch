 /*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <list>
#include <string>
#include <vector>

#include "creature.h"
#include "direction.h"
#include "observable.h"
#include "savegame.h"
#include "script.h"
#include "tile.h"
#include "types.h"

class Armor;
class Party;
class Weapon;


typedef std::vector<class PartyMember *> PartyMemberVector;

#define ALL_PLAYERS -1

enum KarmaAction {
    KA_FOUND_ITEM,
    KA_STOLE_CHEST,
    KA_GAVE_TO_BEGGAR,
    KA_GAVE_ALL_TO_BEGGAR,
    KA_BRAGGED,
    KA_HUMBLE,
    KA_HAWKWIND,
    KA_MEDITATION,
    KA_BAD_MANTRA,
    KA_ATTACKED_GOOD,
    KA_FLED_EVIL,
    KA_FLED_GOOD,
    KA_HEALTHY_FLED_EVIL,
    KA_KILLED_EVIL,
    KA_SPARED_GOOD,
    KA_DONATED_BLOOD,
    KA_DIDNT_DONATE_BLOOD,
    KA_CHEAT_REAGENTS,
    KA_DIDNT_CHEAT_REAGENTS,
    KA_USED_SKULL,
    KA_DESTROYED_SKULL
};

enum HealType {
    HT_NONE,
    HT_CURE,
    HT_FULLHEAL,
    HT_RESURRECT,
    HT_HEAL,
    HT_CAMPHEAL,
    HT_INNHEAL
};

enum InventoryItem {
    INV_NONE,
    INV_WEAPON,
    INV_ARMOR,
    INV_FOOD,
    INV_REAGENT,
    INV_GUILDITEM,
    INV_HORSE
};

enum CannotJoinError {
    JOIN_SUCCEEDED,
    JOIN_NOT_EXPERIENCED,
    JOIN_NOT_VIRTUOUS
};

enum EquipError {
    EQUIP_SUCCEEDED,
    EQUIP_NONE_LEFT,
    EQUIP_CLASS_RESTRICTED
};


/**
 * PartyMember class
 */
class PartyMember:public Creature, public Script::Provider {
public:
    friend class Party;
    PartyMember(Party *p, SaveGamePlayerRecord *pr);
    PartyMember(const PartyMember &p) = default;
    PartyMember(PartyMember &&p) = default;
    PartyMember &operator=(const PartyMember &p) = default;
    PartyMember &operator=(PartyMember &&p) = default;
    virtual ~PartyMember();
    void notifyOfChange();
    // Used to translate script values into something useful
    virtual std::string translate(std::vector<std::string> &parts) override;

    virtual int getHp() const override;

    int getMaxHp() const
    {
        return player->hpMax;
    }

    int getExp() const
    {
        return player->xp;
    }

    int getStr() const
    {
        return player->str;
    }

    int getDex() const
    {
        return player->dex;
    }

    int getInt() const
    {
        return player->intel;
    }

    int getMp() const
    {
        return player->mp;
    }

    int getMaxMp() const;
    const Weapon *getWeapon() const;
    const Armor *getArmor() const;
    virtual std::string getName() const override;
    SexType getSex() const;
    ClassType getClass() const;
    virtual CreatureState getState() const override;
    int getRealLevel() const;
    int getMaxLevel() const;
    virtual void addStatus(StatusType s) override;
    virtual void setStatus(StatusType s) override;
    void adjustMp(int pts);
    void advanceLevel();
    void applyEffect(TileEffect effect);
    void awardXp(int xp);
    bool heal(HealType type);
    virtual void removeStatus(StatusType s) override;
    virtual void setHp(int hp) override;
    void setMp(int mp);
    EquipError setArmor(const Armor *a);
    EquipError setWeapon(const Weapon *w);
    virtual bool applyDamage(int damage, bool byplayer = false) override;
    virtual int getAttackBonus() const override;
    virtual int getDefense(bool needsMystic) const override;
    virtual bool dealDamage(Creature *m, int damage) override;
    virtual int getDamage() const override;
    virtual const std::string &getHitTile() const override;
    virtual const std::string &getMissTile() const override;
    bool isDead() const;
    bool isDisabled() const;
    int  loseWeapon();
    virtual void putToSleep(bool sound = true) override;
    virtual void wakeUp() override;

protected:
    static MapTile tileForClass(int klass);
    SaveGamePlayerRecord *player;
    class Party *party;
};


/**
 * Party class
 */
class PartyEvent {
public:
    enum Type {
        GENERIC,
        LOST_EIGHTH,
        ADVANCED_LEVEL,
        STARVING,
        TRANSPORT_CHANGED,
        PLAYER_KILLED,
        ACTIVE_PLAYER_CHANGED,
        MEMBER_JOINED,
        PARTY_REVIVED,
        INVENTORY_ADDED,
    };

    PartyEvent(Type type, PartyMember *partyMember)
        :type(type), player(partyMember)
    {
    }

    Type type;
    PartyMember *player;
};

typedef std::vector<PartyMember *> PartyMemberVector;


class Party
    :public Observable<Party *, PartyEvent &>,
     public Script::Provider {
public:
    friend class PartyMember;
    explicit Party(SaveGame *s);
    Party(const Party &) = delete;
    Party(Party &&) = delete;
    Party &operator=(const Party &) = delete;
    Party &operator=(Party &&) = delete;
    virtual ~Party();
    void notifyOfChange(
        PartyMember *pm = nullptr,
        PartyEvent::Type eventType = PartyEvent::GENERIC
    );
    // Used to translate script values into something useful
    virtual std::string translate(std::vector<std::string> &parts) override;
    void adjustFood(int food);
    void adjustGold(int gold);
    void adjustKarma(KarmaAction action);
    void applyEffect(TileEffect effect);
    bool attemptElevation(Virtue virtue);
    void burnTorch(int turns = 1);
    bool canEnterShrine(Virtue virtue) const;
    bool canPersonJoin(const std::string &name, Virtue *v) const;
    void damageShip(unsigned int pts);
    bool donate(int quantity);
    void endTurn();
    int  getChest();
    int  getTorchDuration() const;
    void healShip(unsigned int pts);
    bool isFlying() const;
    bool isImmobilized() const;
    bool isDead() const;
    bool isPersonJoined(const std::string &name) const;
    CannotJoinError join(const std::string &name);
    bool lightTorch(int duration = 100, bool loseTorch = true);
    void quenchTorch();
    void reviveParty();
    MapTile getTransport() const;
    void setTransport(MapTile tile);
    void setShipHull(int str);
    Direction getDirection() const;
    void setDirection(Direction dir);
    void adjustReagent(int reagent, int amt);
    static int getReagent(int reagent);
    static unsigned short *getReagentPtr(int reagent);
    void setActivePlayer(int p);
    int getActivePlayer() const;
    void swapPlayers(int p1, int p2);
    int size() const;
    PartyMember *member(int index) const;

private:
    void syncMembers();
    PartyMemberVector members;
    SaveGame *saveGame;
    MapTile transport;
    int torchduration;
    int activePlayer;
};

bool isPartyMember(Object *punknown);

#endif // ifndef PLAYER_H
