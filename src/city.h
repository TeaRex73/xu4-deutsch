/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

#include <list>
#include <string>
#include <vector>


class Person;
class Dialogue;

#include "map.h"

struct PersonRole {
    int role;
    int id;
};

typedef std::vector<Person *> PersonList;
typedef std::list<PersonRole *> PersonRoleList;

class City:public Map {
public:
    City();
    virtual ~City();
    virtual std::string getName() const override;
    Person *addPerson(Person *person);
    void addPeople();
    void removeAllPeople();
    Person *personAt(const Coords &coords);
    std::string name;
    std::string cityType;
    PersonList persons;
    std::string tlk_fname;
    PersonRoleList personroles;
    std::vector<Person *> personObjects;
    std::vector<Dialogue *> normalDialogues;
    std::vector<Dialogue *> extraDialogues;
};

bool isCity(Map *punknown);

#endif // ifndef CITY_H
