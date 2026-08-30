/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <deque>
#include <string>
#include <vector>

#include "city.h"

#include "context.h"
#include "conversation.h" // IWYU pragma: keep
#include "object.h"
#include "person.h"
#include "player.h"
#include "types.h"

City::~City()
{
    for (const auto *person: persons) {
        delete person;
    }
    for (const auto *personrole: personroles) {
        delete personrole;
    }
    for (const auto *normalDialogue: normalDialogues) {
        delete normalDialogue;
    }
    for (const auto *extraDialogue: extraDialogues) {
        delete extraDialogue;
    }
    for (const auto *personObject: personObjects) {
        removeObject(personObject);
    }
}


/**
 * Returns the name of the city
 */
std::string City::getName()
{
    return name;
}


/**
 * Adds a person object to the map
 */
Person *City::addPerson(const Person *person)
{
    // Make a copy of the person before adding them, so
    // things like angering the guards, etc. will be
    // forgotten the next time you visit :)
    if (person) {
        auto *p = new Person(*person);
        /* set the start coordinates for the person */
        p->setMap(this);
        p->goToStartLocation();
        personObjects.push_back(p);
        objects.push_back(p);
        return p;
    }
    return nullptr;
}


/**
 * Add people to the map
 */
void City::addPeople()
{
    // Make sure the city has no people in it already
    removeAllPeople();
    for (const auto *p: persons) {
        if (p->getTile() != 0
            && !c->party->isPersonJoined(p->getName())) {
            addPerson(p);
        }
    }
}


/**
 * Removes all people from the current map
 */
void City::removeAllPeople()
{
    for (auto obj = objects.begin(); obj != objects.end();) {
        if (isPerson(*obj)) {
            obj = removeObject(obj);
        } else {
            ++obj;
        }
    }
}


/**
 * Returns the person object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns nullptr.
 */
Person *City::personAt(const Coords &coords) const {
    Object *obj = objectAt(coords);
    if (isPerson(obj)) {
        return dynamic_cast<Person *>(obj);
    }
    return nullptr;
}


/**
 * Returns true if the Map pointed to by 'pUnknown'
 * is a City map
 */
bool isCity(Map *pUnknown)
{
    if (dynamic_cast<City *>(pUnknown) != nullptr) {
        return true;
    }
    return false;
}
