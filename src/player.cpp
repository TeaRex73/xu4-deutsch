/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstring>

#include "player.h"

#include "annotation.h"
#include "armor.h"
#include "combat.h"
#include "context.h"
#include "debug.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "names.h"
#include "tilemap.h"
#include "tileset.h"
#include "types.h"
#include "utils.h"
#include "weapon.h"

bool isPartyMember(Object *punknown)
{
    if (dynamic_cast<PartyMember *>(punknown) != nullptr) {
        return true;
    } else {
        return false;
    }
}


/**
 * PartyMember class implementation
 */
PartyMember::PartyMember(Party *p, SaveGamePlayerRecord *pr)
    :Creature(tileForClass(pr->klass)), player(pr), party(p)
{
    setType(Object::PARTYMEMBER);
    /* FIXME: we need to rename movement behaviors */
    setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
    this->ranged = Weapon::get(pr->weapon)->getRange() ? 1 : 0;
    PartyMember::setStatus(pr->status);
}

PartyMember::~PartyMember()
{
}


/**
 * Notify the party that this player has changed somehow
 */
void PartyMember::notifyOfChange()
{
    if (party) {
        party->notifyOfChange(this);
    }
}


/**
 * Provides some translation information for scripts
 */
std::string PartyMember::translate(std::vector<std::string> &parts)
{
    if (parts.size() == 0) {
        return "";
    } else if (parts.size() == 1) {
        if (parts[0] == "hp") {
            return xu4_to_string(getHp());
        } else if (parts[0] == "max_hp") {
            return xu4_to_string(getMaxHp());
        } else if (parts[0] == "mp") {
            return xu4_to_string(getMp());
        } else if (parts[0] == "max_mp") {
            return xu4_to_string(getMaxMp());
        } else if (parts[0] == "str") {
            return xu4_to_string(getStr());
        } else if (parts[0] == "dex") {
            return xu4_to_string(getDex());
        } else if (parts[0] == "int") {
            return xu4_to_string(getInt());
        } else if (parts[0] == "exp") {
            return xu4_to_string(getExp());
        } else if (parts[0] == "name") {
            return getName();
        } else if (parts[0] == "weapon") {
            return getWeapon()->getName();
        } else if (parts[0] == "armor") {
            return getArmor()->getName();
        } else if (parts[0] == "sex") {
            std::string var = " ";
            SexType s = getSex();
            if (s == SEX_MALE) {
                var[0] = 'm';
            } else if (s == SEX_FEMALE) {
                var[0] = 'f';
            }
            return var;
        } else if (parts[0] == "class") {
            return getClassNameEnglish(getClass());
        } else if (parts[0] == "level") {
            return xu4_to_string(getRealLevel());
        }
    } else if (parts.size() == 2) {
        if (parts[0] == "needs") {
            if (parts[1] == "cure") {
                if (getStatus() == STAT_POISONED) {
                    return "true";
                } else {
                    return "false";
                }
            } else if ((parts[1] == "heal") || (parts[1] == "fullheal")) {
                if (getHp() < getMaxHp()) {
                    return "true";
                } else {
                    return "false";
                }
            } else if (parts[1] == "resurrect") {
                if (getStatus() == STAT_DEAD) {
                    return "true";
                } else {
                    return "false";
                }
            }
        }
    }
    return "";
} // PartyMember::translate

int PartyMember::getHp() const
{
    return player->hp;
}


/**
 * Determine the most magic points a character could have
 * given his class and intelligence.
 */
int PartyMember::getMaxMp() const
{
    int max_mp = -1;
    switch (player->klass) {
    case CLASS_MAGE:
        /*  mage: 200% of int */
        max_mp = player->intel * 2;
        break;
    case CLASS_DRUID:
        /* druid: 150% of int */
        max_mp = player->intel * 3 / 2;
        break;
    case CLASS_BARD:
    case CLASS_PALADIN:
    case CLASS_RANGER:
        /* bard, paladin, ranger: 100% of int */
        max_mp = player->intel;
        break;
    case CLASS_TINKER:
        /* tinker: 50% of int */
        max_mp = player->intel / 2;
        break;
    case CLASS_FIGHTER:
    case CLASS_SHEPHERD:
        /* fighter, shepherd: no mp at all */
        max_mp = 0;
        break;
    default:
        U4ASSERT(0, "invalid player class: %d", player->klass);
    }

    /* mp always maxes out at 99 */
    if (max_mp > 99) {
        max_mp = 99;
    }
    return max_mp;
} // PartyMember::getMaxMp

const Weapon *PartyMember::getWeapon() const
{
    return Weapon::get(player->weapon);
}

const Armor *PartyMember::getArmor() const
{
    return Armor::get(player->armor);
}

std::string PartyMember::getName() const
{
    return player->name;
}

SexType PartyMember::getSex() const
{
    return player->sex;
}

ClassType PartyMember::getClass() const
{
    return player->klass;
}

CreatureState PartyMember::getState() const
{
    if (getHp() <= 0) {
        return MSTAT_DEAD;
    } else if (getHp() < 24) {
        return MSTAT_FLEEING;
    } else {
        return MSTAT_BARELYWOUNDED;
    }
}


/**
 * Determine what level a character has.
 */
int PartyMember::getRealLevel() const
{
    return player->hpMax / 100;
}


/**
 * Determine the highest level a character could have with the number
 * of experience points he has.
 */
int PartyMember::getMaxLevel() const
{
    int level = 1;
    int next = 100;
    while (player->xp >= next && level < 8) {
        level++;
        next <<= 1;
    }
    return level;
}

/**
 * Adds a status effect to the player
 */
void PartyMember::addStatus(StatusType s)
{
    Creature::addStatus(s);
    player->status = status;
    switch (player->status) {
    case STAT_GOOD:
    case STAT_POISONED:
        setTile(tileForClass(getClass()));
        break;
    case STAT_SLEEPING:
    case STAT_DEAD:
        setTile(Tileset::findTileByName("corpse")->getId());
        break;
    default:
        U4ASSERT(
            0,
            "Invalid Status %d in PartyMember::addStatus",
            static_cast<int>(player->status)
        );
    }
    notifyOfChange();
}

/**
 * Unconditionally sets a status effect for the player
 */
void PartyMember::setStatus(StatusType s)
{
    Creature::setStatus(s);
    player->status = status;
    switch (player->status) {
    case STAT_GOOD:
    case STAT_POISONED:
        setTile(tileForClass(getClass()));
        break;
    case STAT_SLEEPING:
    case STAT_DEAD:
        setTile(Tileset::findTileByName("corpse")->getId());
        break;
    default:
        U4ASSERT(
            0,
            "Invalid Status %d in PartyMember::setStatus",
            static_cast<int>(player->status)
        );
    }
    notifyOfChange();
}

/**
 * Adjusts the player's mp by 'pts'
 */
void PartyMember::adjustMp(int pts)
{
    AdjustValueMax(player->mp, pts, getMaxMp());
    notifyOfChange();
}


/**
 * Advances the player to the next level if they have enough experience
 */
void PartyMember::advanceLevel()
{
    if (getRealLevel() == getMaxLevel()) {
        return;
    }
    setStatus(STAT_GOOD);
    player->hpMax = getMaxLevel() * 100;
    player->hp = player->hpMax;
    /* improve stats by 1-8 each */
    player->str += xu4_random(8) + 1;
    player->dex += xu4_random(8) + 1;
    player->intel += xu4_random(8) + 1;
    /* cap at 50 */
    if (player->str > 50) {
        player->str = 50;
    }
    if (player->dex > 50) {
        player->dex = 50;
    }
    if (player->intel > 50) {
        player->intel = 50;
    }
    if (party) {
        party->setChanged();
        PartyEvent event(PartyEvent::ADVANCED_LEVEL, this);
        event.player = this;
        party->notifyObservers(event);
    }
} // PartyMember::advanceLevel


/**
 * Apply an effect to the party member
 */
void PartyMember::applyEffect(TileEffect effect)
{
    if (getStatus() == STAT_DEAD) {
        return;
    }
    switch (effect) {
    case EFFECT_NONE:
        break;
    case EFFECT_LAVA:
    case EFFECT_FIRE:
        soundPlay(SOUND_PC_STRUCK, false);
        applyDamage(xu4_random(30)); // From u4apple2
        break;
    case EFFECT_SLEEP:
        putToSleep();
        break;
    case EFFECT_POISONFIELD:
    case EFFECT_POISON:
        if (getStatus() != STAT_POISONED) {
            soundPlay(SOUND_POISON_EFFECT, false);
            addStatus(STAT_POISONED);
        }
        break;
    case EFFECT_ELECTRICITY:
        break;
    default:
        U4ASSERT(0, "invalid effect: %d", effect);
    }
    if (effect != EFFECT_NONE) {
        notifyOfChange();
    }
} // PartyMember::applyEffect


/**
 * Award a player experience points.  Maxs out the players xp at 9999.
 */
void PartyMember::awardXp(int xp)
{
    AdjustValueMax(player->xp, xp, 9999);
    notifyOfChange();
}


/**
 * Perform a certain type of healing on the party member
 */
bool PartyMember::heal(HealType type)
{
    switch (type) {
    case HT_NONE:
        return true;
    case HT_CURE:
        if (getStatus() != STAT_POISONED) {
            return false;
        }
        removeStatus(STAT_POISONED);
        break;
    case HT_FULLHEAL:
        if ((getStatus() == STAT_DEAD) || (player->hp == player->hpMax)) {
            return false;
        }
        player->hp = player->hpMax;
        break;
    case HT_RESURRECT:
        if (getStatus() != STAT_DEAD) {
            return false;
        }
        setStatus(STAT_GOOD);
        break;
    case HT_HEAL:
        if ((getStatus() == STAT_DEAD) || (player->hp == player->hpMax)) {
            return false;
        }
        player->hp += 75 + xu4_random(24);
        break;
    case HT_CAMPHEAL:
        if ((getStatus() == STAT_DEAD) || (player->hp == player->hpMax)) {
            return false;
        }
        player->hp += 99 + (xu4_random(0x100) & 0x77);
        break;
    case HT_INNHEAL:
        if ((getStatus() == STAT_DEAD) || (player->hp == player->hpMax)) {
            return false;
        }
        player->hp += 100 + (xu4_random(50) * 2);
        break;
    default:
        return false;
    } // switch
    if (player->hp > player->hpMax) {
        player->hp = player->hpMax;
    }
    notifyOfChange();
    return true;
} // PartyMember::heal


/**
 * Remove status effects from the party member
 */
void PartyMember::removeStatus(StatusType s)
{
    Creature::removeStatus(s);
    player->status = status;
    switch (player->status) {
    case STAT_GOOD:
    case STAT_POISONED:
        setTile(tileForClass(getClass()));
                break;
    case STAT_SLEEPING:
    case STAT_DEAD:
        setTile(Tileset::findTileByName("corpse")->getId());
        break;
    default:
        U4ASSERT(
            0,
            "Invalid Status %d in PartyMember::removeStatus",
            static_cast<int>(player->status)
        );
    }
    notifyOfChange();
}

void PartyMember::setHp(int hp)
{
    player->hp = hp;
    notifyOfChange();
}

void PartyMember::setMp(int mp)
{
    player->mp = mp;
    notifyOfChange();
}

EquipError PartyMember::setArmor(const Armor *a)
{
    ArmorType type = a->getType();
    if ((type != ARMR_NONE) && (party->saveGame->armor[type] < 1)) {
        return EQUIP_NONE_LEFT;
    }
    if (!a->canWear(getClass())) {
        return EQUIP_CLASS_RESTRICTED;
    }
    ArmorType oldArmorType = getArmor()->getType();
    if (oldArmorType != ARMR_NONE) {
        party->saveGame->armor[oldArmorType]++;
    }
    if (type != ARMR_NONE) {
        party->saveGame->armor[type]--;
    }
    player->armor = type;
    notifyOfChange();
    return EQUIP_SUCCEEDED;
}

EquipError PartyMember::setWeapon(const Weapon *w)
{
    WeaponType type = w->getType();
    if ((type != WEAP_HANDS) && (party->saveGame->weapons[type] < 1)) {
        return EQUIP_NONE_LEFT;
    }
    if (!w->canReady(getClass())) {
        return EQUIP_CLASS_RESTRICTED;
    }
    WeaponType old = getWeapon()->getType();
    if (old != WEAP_HANDS) {
        party->saveGame->weapons[old]++;
    }
    if (type != WEAP_HANDS) {
        party->saveGame->weapons[type]--;
    }
    player->weapon = type;
    notifyOfChange();
    return EQUIP_SUCCEEDED;
}


/**
 * Applies damage to a player, and changes status to dead if hit
 * points drop below zero.
 *
 * Byplayer is ignored for now, since it should always be false for U4.  (Is
 * there anything special about being killed by a party member in U5?)  Also
 * keeps interface consistent for virtual base function Creature::applydamage()
 */
bool PartyMember::applyDamage(int damage, bool)
{
    int newHp = player->hp;
    if (getStatus() == STAT_DEAD) {
        return false;
    }
    newHp -= damage;
    if (newHp < 0) {
        setStatus(STAT_DEAD);
        newHp = 0;
    }
    player->hp = newHp;
    notifyOfChange();
    if (isCombatMap(c->location->map) && (getStatus() == STAT_DEAD)) {
        if (party) {
            const Coords &p = getCoords();
            Map *map = getMap();
            map->annotations->add(
                p,
                Tileset::findTileByName("corpse")->getId()
            )->setTTL(party->size() * 2);
            party->setChanged();
            PartyEvent event(PartyEvent::PLAYER_KILLED, this);
            event.player = this;
            party->notifyObservers(event);
        }
        /* remove yourself from the map */
        remove();
        return false;
    }
    return true;
} // PartyMember::applyDamage

int PartyMember::getAttackBonus() const
{
    if (Weapon::get(player->weapon)->alwaysHits()) {
        return 1;
    }
    return static_cast<int>(xu4_random(256) < (128 + 2 * player->dex));
}

int PartyMember::getDefense(bool needsMystic) const
{
    return static_cast<int>(
        xu4_random(256) < Armor::get(player->armor)->getDefense(needsMystic)
    );
}

bool PartyMember::dealDamage(Creature *m, int damage)
{
    /* we have to record these now, because if we
       kill the target, it gets destroyed */
    int m_xp = m->getXp();
    if (!Creature::dealDamage(m, damage)) {
        /* half the time you kill an evil creature you get
           a karma boost */
        awardXp(m_xp);
        return false;
    }
    return true;
}


/**
 * Calculate damage for an attack.
 */
int PartyMember::getDamage() const
{
    int maxDamage;
    maxDamage = Weapon::get(player->weapon)->getDamage();
    if (!Weapon::get(player->weapon)->rangedOnly()) {
        maxDamage += player->str;
    }
    if (maxDamage > 255) {
        maxDamage = 255;
    }
    return xu4_random(maxDamage);
}


/**
 * Returns the tile that will be displayed when the party
 * member's attack hits
 */
const std::string &PartyMember::getHitTile() const
{
    return getWeapon()->getHitTile();
}


/**
 * Returns the tile that will be displayed when the party
 * member's attack fails
 */
const std::string &PartyMember::getMissTile() const
{
    return getWeapon()->getMissTile();
}

bool PartyMember::isDead() const
{
    return getStatus() == STAT_DEAD;
}

bool PartyMember::isDisabled() const
{
    return (getStatus() == STAT_GOOD || getStatus() == STAT_POISONED) ?
        false :
        true;
}


/**
 * Lose the equipped weapon for the player (flaming oil, ranged daggers, etc.)
 * Returns the number of weapons left of that type, including the one in
 * the players hand
 */
int PartyMember::loseWeapon()
{
    int weapon = player->weapon;
    notifyOfChange();
    if (party->saveGame->weapons[weapon] > 0) {
        return (--party->saveGame->weapons[weapon]) + 1;
    } else {
        player->weapon = WEAP_HANDS;
        return 0;
    }
}


/**
 * Put the party member to sleep
 */
void PartyMember::putToSleep(bool sound)
{
    if (getStatus() != STAT_DEAD) {
        if (sound) {
            soundPlay(SOUND_SLEEP, false);
        }
        addStatus(STAT_SLEEPING);
    }
}


/**
 * Wakes up the party member
 */
void PartyMember::wakeUp()
{
    removeStatus(STAT_SLEEPING);
}

MapTile PartyMember::tileForClass(int klass)
{
    const char *name = nullptr;
    switch (klass) {
    case CLASS_MAGE:
        name = "mage";
        break;
    case CLASS_BARD:
        name = "bard";
        break;
    case CLASS_FIGHTER:
        name = "fighter";
        break;
    case CLASS_DRUID:
        name = "druid";
        break;
    case CLASS_TINKER:
        name = "tinker";
        break;
    case CLASS_PALADIN:
        name = "paladin";
        break;
    case CLASS_RANGER:
        name = "ranger";
        break;
    case CLASS_SHEPHERD:
        name = "shepherd";
        break;
    default:
        U4ASSERT(0, "invalid class %d in tileForClass", klass);
    }
    const Tile *tile = Tileset::get("base")->getByName(name);
    U4ASSERT(tile, "no tile found for class %d", klass);
    return tile->getId();
} // PartyMember::tileForClass


/**
 * Party class implementation
 */
Party::Party(SaveGame *s)
    :members(), saveGame(s), transport(0), torchduration(0), activePlayer(-1)
{
    if ((MAP_DECEIT <= saveGame->location)
        && (saveGame->location <= MAP_ABYSS)) {
        torchduration = saveGame->torchduration;
    }
    for (int i = 0; i < saveGame->members; i++) {
        // add the members to the party
        members.push_back(new PartyMember(this, &saveGame->players[i]));
    }
    // set the party's transport (transport value stored in savegame
    // hardcoded to index into base tilemap)
    setTransport(TileMap::get("base")->translate(saveGame->transport));
}

Party::~Party()
{
}


/**
 * Notify the party that something about it has changed
 */
void Party::notifyOfChange(PartyMember *pm, PartyEvent::Type eventType)
{
    setChanged();
    PartyEvent event(eventType, pm);
    notifyObservers(event);
}

std::string Party::translate(std::vector<std::string> &parts)
{
    if (parts.size() == 0) {
        return "";
    } else if (parts.size() == 1) {
        // Translate some different items for the script
        if (parts[0] == "transport") {
            if (c->transportContext & TRANSPORT_FOOT) {
                return "foot";
            }
            if (c->transportContext & TRANSPORT_HORSE) {
                return "horse";
            }
            if (c->transportContext & TRANSPORT_SHIP) {
                return "ship";
            }
            if (c->transportContext & TRANSPORT_BALLOON) {
                return "balloon";
            }
        } else if (parts[0] == "gold") {
            return xu4_to_string(saveGame->gold);
        } else if (parts[0] == "food") {
            return xu4_to_string(saveGame->food);
        } else if (parts[0] == "members") {
            return xu4_to_string(size());
        } else if (parts[0] == "keys") {
            return xu4_to_string(saveGame->keys);
        } else if (parts[0] == "torches") {
            return xu4_to_string(saveGame->torches);
        } else if (parts[0] == "gems") {
            return xu4_to_string(saveGame->gems);
        } else if (parts[0] == "sextants") {
            return xu4_to_string(saveGame->sextants);
        } else if (parts[0] == "party_members") {
            return xu4_to_string(saveGame->members);
        } else if (parts[0] == "moves") {
            return xu4_to_string(saveGame->moves);
        }
    } else { // parts.size() >= 2
        if (parts[0].find_first_of("member") == 0) {
            // Make a new parts list, but remove the first item
            std::vector<std::string> new_parts = parts;
            new_parts.erase(new_parts.begin());
            // Find the member we'll be working with
            std::string str = parts[0];
            std::size_t pos = str.find_first_of("1234567890");
            if (pos != std::string::npos) {
                str = str.substr(pos);
                int p_member =
                    static_cast<int>(std::strtol(str.c_str(), nullptr, 10));
                // Make the party member translate its
                // own stuff
                if (p_member > 0) {
                    return member(p_member - 1)->translate(new_parts);
                }
            }
        } else if (parts.size() == 2) {
            if (parts[0] == "weapon") {
                const Weapon *w = Weapon::get(parts[1]);
                if (w) {
                    return xu4_to_string(saveGame->weapons[w->getType()]);
                }
            } else if (parts[0] == "armor") {
                const Armor *a = Armor::get(parts[1]);
                if (a) {
                    return xu4_to_string(saveGame->armor[a->getType()]);
                }
            }
        }
    }
    return "";
} // Party::translate


void Party::adjustFood(int food)
{
    unsigned int oldFood = saveGame->food;
    AdjustValue(saveGame->food, food, 999900, 0);
    if ((saveGame->food / 100) != (oldFood / 100)) {
        notifyOfChange();
    }
}

void Party::adjustGold(int gold)
{
    AdjustValue(saveGame->gold, gold, 9999, 0);
    notifyOfChange();
}


/**
 * Adjusts the avatar's karma level for the given action.  Notify
 * observers with a lost eighth event if the player has lost
 * avatarhood.
 */
void Party::adjustKarma(KarmaAction action)
{
    bool timeLimited = false;
    int v, newKarma[VIRT_MAX], maxVal[VIRT_MAX];
    /*
     * make a local copy of all virtues, and adjust it according to
     * the game rules
     */
    for (v = 0; v < VIRT_MAX; v++) {
        newKarma[v] = saveGame->karma[v] == 0 ? 100 : saveGame->karma[v];
        maxVal[v] = saveGame->karma[v] == 0 ? 100 : 99;
    }
    switch (action) {
    case KA_FOUND_ITEM:
        AdjustValueMax(newKarma[VIRT_HONOR], 5, maxVal[VIRT_HONOR]);
        break;
    case KA_STOLE_CHEST:
        AdjustValueMin(newKarma[VIRT_HONESTY], -1, 1);
        AdjustValueMin(newKarma[VIRT_JUSTICE], -1, 1);
        AdjustValueMin(newKarma[VIRT_HONOR], -1, 1);
        break;
    case KA_GAVE_ALL_TO_BEGGAR:
        // When donating all,
        // you get +3 HONOR in Apple 2, but not in in U4DOS.
        // That is arguably a bug, SACRIFICE should be it.
        // TODO: Make this a configuration option.
        AdjustValueMax(newKarma[VIRT_SACRIFICE], 3, maxVal[VIRT_SACRIFICE]);
        /* FALLTHROUGH */
    case KA_GAVE_TO_BEGGAR:
        // In U4DOS, we only get +2 COMPASSION,
        // no HONOR or SACRIFICE even if
        // donating all.
        timeLimited = true;
        AdjustValueMax(newKarma[VIRT_COMPASSION], 2, maxVal[VIRT_COMPASSION]);
        break;
    case KA_BRAGGED:
        AdjustValueMin(newKarma[VIRT_HUMILITY], -5, 1);
        break;
    case KA_HUMBLE:
        timeLimited = true;
        AdjustValueMax(newKarma[VIRT_HUMILITY], 5, maxVal[VIRT_HUMILITY]);
        break;
    case KA_HAWKWIND:
    case KA_MEDITATION:
        timeLimited = true;
        AdjustValueMax(
            newKarma[VIRT_SPIRITUALITY], 3, maxVal[VIRT_SPIRITUALITY]
        );
        break;
    case KA_BAD_MANTRA:
        AdjustValueMin(newKarma[VIRT_SPIRITUALITY], -3, 1);
        break;
    case KA_ATTACKED_GOOD:
        AdjustValueMin(newKarma[VIRT_COMPASSION], -5, 1);
        AdjustValueMin(newKarma[VIRT_JUSTICE], -5, 1);
        AdjustValueMin(newKarma[VIRT_HONOR], -5, 1);
        break;
    case KA_FLED_EVIL:
        AdjustValueMin(newKarma[VIRT_VALOR], -2, 1);
        break;
    case KA_HEALTHY_FLED_EVIL:
        AdjustValueMin(newKarma[VIRT_VALOR], -2, 1);
        AdjustValueMin(newKarma[VIRT_SACRIFICE], -2, 1);
        break;
    case KA_KILLED_EVIL:
        AdjustValueMax(
            newKarma[VIRT_VALOR], xu4_random(2), maxVal[VIRT_VALOR]
        ); /* gain one valor half the time, zero the rest */
        break;
    case KA_FLED_GOOD:
        AdjustValueMax(
            newKarma[VIRT_COMPASSION], 2, maxVal[VIRT_COMPASSION]
        );
        AdjustValueMax(
            newKarma[VIRT_JUSTICE], 2, maxVal[VIRT_JUSTICE]
        );
        break;
    case KA_SPARED_GOOD:
        AdjustValueMax(
            newKarma[VIRT_COMPASSION], 1, maxVal[VIRT_COMPASSION]
        );
        AdjustValueMax(
            newKarma[VIRT_JUSTICE], 1, maxVal[VIRT_JUSTICE]
        );
        break;
    case KA_DONATED_BLOOD:
        AdjustValueMax(
            newKarma[VIRT_SACRIFICE], 5, maxVal[VIRT_SACRIFICE]
        );
        break;
    case KA_DIDNT_DONATE_BLOOD:
        AdjustValueMin(newKarma[VIRT_SACRIFICE], -5, 1);
        break;
    case KA_CHEAT_REAGENTS:
        AdjustValueMin(newKarma[VIRT_HONESTY], -10, 1);
        AdjustValueMin(newKarma[VIRT_JUSTICE], -10, 1);
        AdjustValueMin(newKarma[VIRT_HONOR], -10, 1);
        break;
    case KA_DIDNT_CHEAT_REAGENTS:
        timeLimited = true;
        AdjustValueMax(newKarma[VIRT_HONESTY], 2, maxVal[VIRT_HONESTY]);
        AdjustValueMax(newKarma[VIRT_JUSTICE], 2, maxVal[VIRT_JUSTICE]);
        AdjustValueMax(newKarma[VIRT_HONOR], 2, maxVal[VIRT_HONOR]);
        break;
    case KA_USED_SKULL:
        /* using the skull is very, very bad... */
        for (v = 0; v < VIRT_MAX; v++) {
            AdjustValueMin(newKarma[v], -5, 1);
        }
        break;
    case KA_DESTROYED_SKULL:
        /* ...but destroying it is very, very good */
        for (v = 0; v < VIRT_MAX; v++) {
            AdjustValueMax(newKarma[v], 10, maxVal[v]);
        }
        break;
    } // switch
      /*
       * check if enough time has passed since last virtue award if
       * action is time limited -- if not, throw away new values
       */
    if (timeLimited) {
        if (((saveGame->moves / 16) >= 0x10000)
            || (((saveGame->moves / 16) & 0xFFFF) != saveGame->lastvirtue)) {
            saveGame->lastvirtue = (saveGame->moves / 16) & 0xFFFF;
        } else {
            return;
        }
    }
    /* something changed */
    notifyOfChange();
    /*
     * return to u4dos compatibility and handle losing of eighths
     */
    for (v = 0; v < VIRT_MAX; v++) {
        if (maxVal[v] == 100) { /* already an avatar */
            if (newKarma[v] < 100) { /* but lost it */
                saveGame->karma[v] = newKarma[v];
                setChanged();
                PartyEvent event(PartyEvent::LOST_EIGHTH, 0);
                notifyObservers(event);
            } else {
                saveGame->karma[v] = 0;
                /* return to u4dos compatibility */
            }
        } else {
            saveGame->karma[v] = newKarma[v];
        }
    }
} // Party::adjustKarma


/**
 * Apply effects to the entire party
 */
void Party::applyEffect(TileEffect effect)
{
    int i;

    switch (effect) {
    case EFFECT_LAVA:
    case EFFECT_FIRE:
        if (c->transportContext == TRANSPORT_SHIP) {
            gameDamageShip(-1, 10);
        } // u4apple2: party damage happens in addition to ship damage
        if (c->transportContext != TRANSPORT_BALLOON) {
            gameDamageParty(0, 24);
        }
        break;
    default:
        for (i = 0; i < size(); i++) {
            switch (effect) {
            case EFFECT_NONE:
            case EFFECT_ELECTRICITY:
                members[i]->applyEffect(effect);
                break;
            case EFFECT_SLEEP:
            case EFFECT_POISONFIELD:
                if (xu4_random(2) == 0) {
                    members[i]->applyEffect(effect);
                }
                break;
            case EFFECT_POISON:
                if (xu4_random(8) == 0) {
                    members[i]->applyEffect(effect);
                    break;
                }
            default:
                break;
            }
        }
    }
}

/**
 * Attempt to elevate in the given virtue
 */
bool Party::attemptElevation(Virtue virtue)
{
    if (saveGame->karma[virtue] == 99) {
        saveGame->karma[virtue] = 0;
        notifyOfChange();
        return true;
    } else {
        return false;
    }
}


/**
 * Burns a torch's duration down a certain number of turns
 */
void Party::burnTorch(int turns)
{
    torchduration -= turns;
    if (torchduration <= 0) {
        torchduration = 0;
    }
    saveGame->torchduration = torchduration;
    notifyOfChange();
}


/**
 * Returns true if the party can enter the shrine
 */
bool Party::canEnterShrine(Virtue virtue) const
{
    if (saveGame->runes & (1 << static_cast<int>(virtue))) {
        return true;
    } else {
        return false;
    }
}


/**
 * Returns true if the person can join the party
 */
bool Party::canPersonJoin(const std::string &name, Virtue *v) const
{
    int i;
    if (name.empty()) {
        return false;
    }
    for (i = 1; i < 8; i++) {
        if (name == saveGame->players[i].name) {
            if (v) {
                *v = static_cast<Virtue>(saveGame->players[i].klass);
            }
            return true;
        }
    }
    return false;
}


/**
 * Damages the party's ship
 */
void Party::damageShip(unsigned int pts)
{
    saveGame->shiphull -= pts;
    if (static_cast<short>(saveGame->shiphull) < 0) {
        saveGame->shiphull = 0;
    }
    notifyOfChange();
}


/**
 * Donates 'quantity' gold. Returns true if the donation succeeded,
 * or false if there was not enough gold to make the donation
 */
bool Party::donate(int quantity)
{
    if (quantity > saveGame->gold) {
        return false;
    }
    adjustGold(-quantity);
    if (saveGame->gold > 0) {
        adjustKarma(KA_GAVE_TO_BEGGAR);
    } else {
        adjustKarma(KA_GAVE_ALL_TO_BEGGAR);
    }
    return true;
}


/**
 * Ends the party's turn
 */
void Party::endTurn()
{
    int i;

    /* u4apple2:
       Moves don't increase during combat. This is important
       for the game balance because of the eras of encounters.
       Increasing moves in combat makes the game get harder too
       quickly.
    */
    if ((c->location->context & CTX_NON_COMBAT) == c->location->context) {
        saveGame->moves++;
    }

    for (i = 0; i < size(); i++) {
        /* Handle player status (only for non-combat turns) */
        if ((c->location->context & CTX_NON_COMBAT) == c->location->context) {
            /* party members eat food (also not during combat) */
            if (!members[i]->isDead()) {
                adjustFood(-1);
            }
            switch (members[i]->getStatus()) {
            case STAT_SLEEPING:
                if (xu4_random(8) == 0) {
                    members[i]->wakeUp();
                }
                break;
            case STAT_POISONED:
                /* FIXME:
                 * shouldn't play poison damage sound
                 * in combat, yet if the PC takes damage
                 * just befor combat begins, the sound is
                 * played after the combat screen appears
                 */
                soundPlay(SOUND_POISON_DAMAGE, false);
                members[i]->applyDamage(2);
                break;
            default:
                break;
            }
        }
        /* regenerate magic points */
        if (!members[i]->isDisabled()
            && (members[i]->getMp() < members[i]->getMaxMp())) {
            saveGame->players[i].mp++;
        }
    }
    /* The party is starving! */
    if ((saveGame->food == 0)
        && ((c->location->context & CTX_NON_COMBAT) == c->location->context)) {
        setChanged();
        PartyEvent event(PartyEvent::STARVING, 0);
        notifyObservers(event);
    }
    /* heal ship (25% chance it is healed each turn) */
    if ((c->location->context == CTX_WORLDMAP)
        && (saveGame->shiphull < 50)
        && (xu4_random(4) == 0)) {
        healShip(1);
    }
} // Party::endTurn


/**
 * Adds a chest worth of gold to the party's inventory
 */
int Party::getChest()
{
    /* int gold = xu4_random(80) + xu4_random(8) + 10; */
    /* above is from u4dos, following is from u4apple2 */
    int gold = xu4_random(100);
    adjustGold(gold);
    return gold;
}


/**
 * Returns the number of turns a currently lit torch will last
 * (or 0 if no torch lit)
 */
int Party::getTorchDuration() const
{
    return torchduration;
}


/**
 * Heals the ship's hull strength by 'pts' points
 */
void Party::healShip(unsigned int pts)
{
    saveGame->shiphull += pts;
    if (saveGame->shiphull > 50) {
        saveGame->shiphull = 50;
    }
    notifyOfChange();
}


/**
 * Returns true if the balloon is currently in the air
 */
bool Party::isFlying() const
{
    return saveGame->balloonstate && torchduration <= 0;
}


/**
 * Whether or not the party can make an action.
 */
bool Party::isImmobilized() const
{
    int i;
    bool immobile = true;

    for (i = 0; i < saveGame->members; i++) {
        if (!members[i]->isDisabled()) {
            immobile = false;
        }
    }
    return immobile;
}


/**
 * Whether or not all the party members are dead.
 */
bool Party::isDead() const
{
    int i;
    bool dead = true;

    for (i = 0; i < saveGame->members; i++) {
        if (!members[i]->isDead()) {
            dead = false;
        }
    }
    return dead;
}


/**
 * Returns true if the person with that name
 * is already in the party
 */
bool Party::isPersonJoined(const std::string &name) const
{
    int i;
    if (name.empty()) {
        return false;
    }
    for (i = 1; i < saveGame->members; i++) {
        if (name == saveGame->players[i].name) {
            return true;
        }
    }
    return false;
}


/**
 * Attempts to add the person to the party.
 * Returns JOIN_SUCCEEDED if successful.
 */
CannotJoinError Party::join(const std::string &name)
{
    int i;
    SaveGamePlayerRecord tmp;
    for (i = saveGame->members; i < 8; i++) {
        if (name == saveGame->players[i].name) {
            /* ensure avatar is experienced enough */
            if (saveGame->members + 1 >
                (saveGame->players[0].hpMax / 100)) {
                return JOIN_NOT_EXPERIENCED;
            }
            /* ensure character has enough karma */
            if ((saveGame->karma[saveGame->players[i].klass] > 0)
                && (saveGame->karma[saveGame->players[i].klass] < 40)) {
                return JOIN_NOT_VIRTUOUS;
            }
            tmp = saveGame->players[saveGame->members];
            saveGame->players[saveGame->members] = saveGame->players[i];
            saveGame->players[i] = tmp;
            members.push_back(
                new PartyMember(this, &saveGame->players[saveGame->members++])
            );
            setChanged();
            PartyEvent event(PartyEvent::MEMBER_JOINED, members.back());
            notifyObservers(event);
            return JOIN_SUCCEEDED;
        }
    }
    return JOIN_NOT_EXPERIENCED;
} // Party::join


/**
 * Lights a torch with a default duration of 100
 */
bool Party::lightTorch(int duration, bool loseTorch)
{
    if (loseTorch) {
        if (c->saveGame->torches == 0) {
            return false;
        }
        c->saveGame->torches--;
    }
    torchduration += duration;
    saveGame->torchduration = torchduration;
    notifyOfChange();
    return true;
}


/**
 * Extinguishes a torch
 */
void Party::quenchTorch()
{
    torchduration = saveGame->torchduration = 0;
    notifyOfChange();
}


/**
 * Revives the party after the entire party has been killed
 */
void Party::reviveParty()
{
    for (int i = 0; i < size(); i++) {
        members[i]->wakeUp();
        members[i]->setStatus(STAT_GOOD);
        saveGame->players[i].hp = saveGame->players[i].hpMax;
    }
    for (int i = ARMR_NONE + 1; i < ARMR_MAX; i++) {
        saveGame->armor[i] = 0;
    }
    for (int i = WEAP_HANDS + 1; i < WEAP_MAX; i++) {
        saveGame->weapons[i] = 0;
    }
    saveGame->food = 20099;
    saveGame->gold = 200;
    setTransport(Tileset::findTileByName("avatar")->getId());
    setChanged();
    PartyEvent event(PartyEvent::PARTY_REVIVED, 0);
    notifyObservers(event);
} // Party::reviveParty

MapTile Party::getTransport() const
{
    return transport;
}

void Party::setTransport(MapTile tile)
{
    // transport value stored in savegame hardcoded to index
    // into base tilemap
    saveGame->transport = TileMap::get("base")->untranslate(tile);
    U4ASSERT(
        saveGame->transport != 0,
        "could not generate valid savegame transport for tile with id %u\n",
        tile.getId()
    );
    transport = tile;
    if (tile.getTileType()->isHorse()) {
        c->transportContext = TRANSPORT_HORSE;
    } else if (tile.getTileType()->isShip()) {
        c->transportContext = TRANSPORT_SHIP;
    } else if (tile.getTileType()->isBalloon()) {
        c->transportContext = TRANSPORT_BALLOON;
    } else {
        c->transportContext = TRANSPORT_FOOT;
    }
    notifyOfChange();
}

void Party::setShipHull(int str)
{
    int newStr = str;

    AdjustValue(newStr, 0, 99, 0);
    if (saveGame->shiphull != newStr) {
        saveGame->shiphull = newStr;
        notifyOfChange();
    }
}

Direction Party::getDirection() const
{
    return transport.getDirection();
}

void Party::setDirection(Direction dir)
{
    transport.setDirection(dir);
    setTransport(transport);
}

void Party::adjustReagent(int reagent, int amt)
{
    int oldVal = c->saveGame->reagents[reagent];
    AdjustValue(c->saveGame->reagents[reagent], amt, 99, 0);
    if (oldVal != c->saveGame->reagents[reagent]) {
        notifyOfChange();
    }
}

int Party::getReagent(int reagent)
{
    return c->saveGame->reagents[reagent];
}

unsigned short *Party::getReagentPtr(int reagent)
{
    return &c->saveGame->reagents[reagent];
}

void Party::setActivePlayer(int p)
{
    activePlayer = p;
    setChanged();
    PartyEvent event(
        PartyEvent::ACTIVE_PLAYER_CHANGED,
        activePlayer < 0 ? 0 : members[activePlayer]
    );
    notifyObservers(event);
}

int Party::getActivePlayer() const
{
    return activePlayer;
}

void Party::swapPlayers(int p1, int p2)
{
    U4ASSERT(p1 < saveGame->members, "p1 out of range: %d", p1);
    U4ASSERT(p2 < saveGame->members, "p2 out of range: %d", p2);
    SaveGamePlayerRecord tmp_rec = saveGame->players[p1];
    saveGame->players[p1] = c->saveGame->players[p2];
    c->saveGame->players[p2] = tmp_rec;

    syncMembers();

    if (p1 == activePlayer) {
        activePlayer = p2;
    } else if (p2 == activePlayer) {
        activePlayer = p1;
    }
#if 0
    PartyMember *tmp_memb = members[p1];
    members[p1] = members[p2];
    members[p2] = tmp_memb;
#endif
    members[p1]->player = &(saveGame->players[p1]);
    members[p2]->player = &(saveGame->players[p2]);
    notifyOfChange();
}

void Party::syncMembers()
{
    members.clear();
    for (int i = 0; i < saveGame->members; i++) {
        // add the members to the party
        members.push_back(new PartyMember(this, &saveGame->players[i]));
    }
}


/**
 * Returns the size of the party
 */
int Party::size() const
{
    return members.size();
}


/**
 * Returns a pointer to the party member indicated
 */
PartyMember *Party::member(int index) const
{
    if (index >= 0 && index < static_cast<int>(members.size())) {
        return members[index];
    } else {
        return nullptr;
    }
}
