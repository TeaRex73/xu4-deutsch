/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "u4.h"

#include "camp.h"

#include "annotation.h"
#include "city.h"
#include "combat.h"
#include "context.h"
#include "conversation.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "creature.h"
#include "music.h"
#include "names.h"
#include "object.h"
#include "person.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"

void campTimer(void *data);
void campEnd(void);
int campHeal(HealType heal_type);
void innTimer(void *data);

CampController::CampController()
{
    MapId id;
    /* setup camp (possible, but not for-sure combat situation */
    if (c->location->context & CTX_DUNGEON) {
        id = MAP_CAMP_DNG;
    } else {
        id = MAP_CAMP_CON;
    }
    map = getCombatMap(mapMgr->get(id));
    game->setMap(map, true, nullptr, this);
}

void CampController::init(Creature *m)
{
    CombatController::init(m);
    camping = true;
}

void CampController::begin()
{
    // make sure everyone's asleep
    for (int i = 0; i < c->party->size(); i++) {
        c->party->member(i)->putToSleep(false);
    }
    CombatController::begin();
    musicMgr->pause();
    screenMessage("AUSRUHEN...\n");
    screenDisableCursor();
    EventHandler::wait_msecs(settings.campTime * 1000);
    screenEnableCursor();
    /* Is the party ambushed during their rest? */
    if (settings.campingAlwaysCombat || (xu4_random(8) == 0)) {
        const Creature *m = creatureMgr->randomAmbushing();
        musicMgr->play();
        screenMessage("HINTERHALT!\n");
        /* create an ambushing creature (so it leaves a chest) */
        setCreature(
            c->location->prev->map->addCreature(m, c->location->prev->coords)
        );
        /* fill the creature table with creatures and place them */
        fillCreatureTable(m);
        placeCreatures();
        /* creatures go first! */
        finishTurn();
    } else {
        /* Wake everyone up! */
        for (int i = 0; i < c->party->size(); i++) {
            c->party->member(i)->wakeUp();
        }
        /* Make sure we've waited long enough for camping
           to be effective */
        bool healed = false;
        if (((c->saveGame->moves / CAMP_HEAL_INTERVAL) >= 0x10000)
            || (((c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff)
                != c->saveGame->lastcamp)) {
            healed = heal();
        }
        screenMessage(healed ?
                      "SPIELER GEHEILT!\n" :
                      "KEINE WIRKUNG.\n");
        c->saveGame->lastcamp =
            (c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff;
        eventHandler->popController();
        game->exitToParentMap();
        musicMgr->play();
    }
} // CampController::begin

void CampController::end(bool adjustKarma)
{
    // wake everyone up!
    for (int i = 0; i < c->party->size(); i++) {
        c->party->member(i)->wakeUp();
    }
    CombatController::end(adjustKarma);
}

bool CampController::heal()
{
    // restore each party member to max mp, and restore some hp
    bool healed = false;
    for (int i = 0; i < c->party->size(); i++) {
        PartyMember *m = c->party->member(i);
        m->setMp(m->getMaxMp());
        if ((m->getHp() < m->getMaxHp()) && m->heal(HT_CAMPHEAL)) {
            healed = true;
        }
    }
    return healed;
}

InnController::InnController()
{
    map = nullptr;
    /*
     * Normally in cities, only one opponent per encounter; inns
     * override this to get the regular encounter size.
     */
    forceStandardEncounterSize = true;
}

void InnController::begin()
{
    /* first, show the avatar before sleeping */
    gameUpdateScreen();
    /* in the original, the vendor music plays straight
       through sleeping */
#if 0 // Not in German version
    if (settings.enhancements) {
        musicMgr->pause(); /* Stop Music */
    }
#endif
    // EventHandler::wait_msecs(INN_FADE_OUT_TIME);
    // make sure everyone's asleep
    for (int i = 0; i < c->party->size(); i++) {
        c->party->member(i)->putToSleep(false);
    }
    /* show the sleeping avatar */
    c->party->setTransport(
        c->location->map->tileset->getByName("corpse")->getId()
    );
    gameUpdateScreen();
    screenDisableCursor();
    EventHandler::wait_msecs(settings.innTime * 1000);
    screenEnableCursor();
    /* restore the avatar to normal */
    c->party->setTransport(
        c->location->map->tileset->getByName("avatar")->getId()
    );
    gameUpdateScreen();
    /* the party is always healed */
    heal();
    /* Is there a special encounter during your stay? */
    // mwinterrowd suggested code, based on u4dos
    if (c->party->member(0)->isDead()
        || (!settings.innAlwaysCombat && (xu4_random(8) != 0))) {
        bool metIsaac = maybeMeetIsaac();
        /* Wake everyone up! */
        for (int i = 0; i < c->party->size(); i++) {
            c->party->member(i)->wakeUp();
        }
        /* The "eerie noise" text goes here (in u4apple2, u4dos doesn't
           have it at all), not in a non-existing rat enconter */
        screenMessage(
            metIsaac ?
            "\nDU ERWACHST VON EINEM SCHAURIGEN GER[USCHE!\n\n" :
            "\nMORGEN!\n"
        );
    } else {
        maybeAmbush();
    }
    screenPrompt();
    musicMgr->play();
} // InnController::begin

bool InnController::heal()
{
    // restore each party member to max mp, and restore some hp
    bool healed = false;
    for (int i = 0; i < c->party->size(); i++) {
        PartyMember *m = c->party->member(i);
        m->setMp(m->getMaxMp());
        if ((m->getHp() < m->getMaxHp()) && m->heal(HT_INNHEAL)) {
            healed = true;
        }
    }
    return healed;
}

bool InnController::maybeMeetIsaac()
{
    // Does Isaac the Ghost pay a visit to the Avatar?
    // He does so in 1 of 4 cases in the inn of Skara Brae
    if ((c->location->map->id == MAP_SKARABRAE) && (xu4_random(4) == 0)) {
        City *city = dynamic_cast<City *>(c->location->map);
        if ((city->extraDialogues.size() == 1)
            && (city->extraDialogues[0]->getName() == "Isaac")) {
            Coords coords(27, xu4_random(3) + 10, c->location->coords.z);
            // If Isaac is already around, just bring him back to the inn
            for (ObjectDeque::const_iterator i =
                     c->location->map->objects.cbegin();
                 i != c->location->map->objects.cend();
                 ++i) {
                Person *p = dynamic_cast<Person *>(*i);
                if (p && (p->getName() == "Isaac")) {
                    p->setCoords(coords);
                    return true;
                }
            }
            // Otherwise, we need to create Isaac
            Person *Isaac;
            Isaac = new Person(creatureMgr->getById(GHOST_ID)->getTile());
            Isaac->setMovementBehavior(MOVEMENT_WANDER);
            Isaac->setDialogue(city->extraDialogues[0]);
            Isaac->getStart() = coords;
            Isaac->setPrevTile(Isaac->getTile());
            // Add Isaac near the Avatar
            city->addPerson(Isaac);
            return true;
        }
    }
    return false;
} // InnController::maybeMeetIsaac

void InnController::maybeAmbush()
{
    MapId mapid;
    Creature *creature;

    /* Wake up the Avatar! */
    c->party->member(0)->wakeUp();

    /* Rats seem much more rare than meeting
       rogues in the streets
       - in fact, rat encounters in the inn don't exist at all in u4dos
       or u4apple2, removing them. The "eerie noise" text belongs to
       meeting Isaac, not to an encouter */

    /* While strolling down the street,
       attacked by rogues! */
    mapid = MAP_INN_CON;
    creature = c->location->map->addCreature(
        creatureMgr->getById(ROGUE_ID),
        c->location->coords
    );
    screenMessage(
        "\nMITTEN IN DER NACHT, W[HREND EINES KLEINEN SPAZIERGANGS...\n\n"
    );
    map = getCombatMap(mapMgr->get(mapid));
    game->setMap(map, true, nullptr, this);
    init(creature);
    showCombatMessage(false);
    CombatController::begin();
} // InnController::maybeAmbush

void InnController::awardLoot()
{
    // never get a chest from inn combat
}
