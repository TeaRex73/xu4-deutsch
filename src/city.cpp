/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include <typeinfo>
#include "city.h"

#include "context.h"
#include "conversation.h"
#include "creature.h"
#include "object.h"
#include "person.h"
#include "player.h"


City::City()
    :Map(),
     name(),
     cityType(),
     persons(),
     tlk_fname(),
     personroles(),
     personObjects(),
     normalDialogues(),
     extraDialogues()
{
}

City::~City()
{
    for (PersonList::iterator i = persons.begin();
         i != persons.end();
         ++i) {
        delete *i;
    }
    for (PersonRoleList::iterator j = personroles.begin();
         j != personroles.end();
         ++j) {
        delete *j;
    }
    for (std::vector<Dialogue *>::iterator k = normalDialogues.begin();
         k != normalDialogues.end();
         ++k) {
        delete *k;
    }
    for (std::vector<Dialogue *>::iterator l = extraDialogues.begin();
         l != extraDialogues.end();
         ++l) {
        delete *l;
    }
    for (std::vector<Person *>::iterator m = personObjects.begin();
         m != personObjects.end();
         ++m) {
        removeObject(*m);
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
    Person *p = new Person(person);
    /* set the start coordinates for the person */
    p->setMap(this);
    p->goToStartLocation();
    personObjects.push_back(p);
    objects.push_back(p);
    return p;
}


/**
 * Add people to the map
 */
void City::addPeople()
{
    PersonList::const_iterator current;
    // Make sure the city has no people in it already
    removeAllPeople();
    for (current = persons.cbegin(); current != persons.cend(); ++current) {
        const Person *p = *current;
        if ((p->getTile() != 0)
            && !(c->party->canPersonJoin(p->getName(), nullptr)
                 && c->party->isPersonJoined(p->getName()))) {
            addPerson(p);
        }
    }
}


/**
 * Removes all people from the current map
 */
void City::removeAllPeople()
{
    ObjectDeque::iterator obj;
    for (obj = objects.begin(); obj != objects.end();) {
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
Person *City::personAt(const Coords &coords)
{
    Object *obj;
    obj = objectAt(coords);
    if (isPerson(obj)) {
        return dynamic_cast<Person *>(obj);
    } else {
        return nullptr;
    }
}


/**
 * Returns true if the Map pointed to by 'punknown'
 * is a City map
 */
bool isCity(Map *punknown)
{
    if (dynamic_cast<City *>(punknown) != nullptr) {
        return true;
    } else {
        return false;
    }
}
