/*
 * $Id$
 */

#ifndef PERSON_H
#define PERSON_H

#include <list>
#include <string>

#include "creature.h"
#include "map.h"

class Conversation;
class Dialogue;
class MapTile;
class Object;
class Response;
class ResponsePart;


typedef enum {
    NPC_EMPTY,
    NPC_TALKER,
    NPC_TALKER_BEGGAR,
    NPC_TALKER_GUARD,
    NPC_TALKER_COMPANION,
    NPC_VENDOR_WEAPONS,
    NPC_VENDOR_ARMOR,
    NPC_VENDOR_FOOD,
    NPC_VENDOR_TAVERN,
    NPC_VENDOR_REAGENTS,
    NPC_VENDOR_HEALER,
    NPC_VENDOR_INN,
    NPC_VENDOR_GUILD,
    NPC_VENDOR_STABLE,
    NPC_LORD_BRITISH,
    NPC_HAWKWIND,
    NPC_MAX
} PersonNpcType;

class Person:public Creature {
public:
    explicit Person(MapTile tile);
    Person(const Person &p) = default;
    Person(Person &&p) noexcept = default;
    Person &operator=(const Person &p) = default;
    Person &operator=(Person &&p) = default;
    ~Person() override = default;
    bool canConverse() const;
    bool isVendor() const;
    std::string getName() const override;
    void goToStartLocation();
    void setDialogue(Dialogue *d);

    MapCoords &getStart()
    {
        return start;
    }

    PersonNpcType getNpcType() const
    {
        return npcType;
    }

    void setNpcType(PersonNpcType t);
    std::list<std::string> getConversationText(
        Conversation *cnv, const char *inquiry
    ) const;
    std::string getPrompt(const Conversation *cnv) const;
    // const char *getChoices(Conversation *cnv);
    std::string getIntro(Conversation *cnv) const;
    std::string processResponse(Conversation *cnv, Response *response) const;
    void runCommand(Conversation *cnv, const ResponsePart &command) const;
    std::string getResponse(Conversation *cnv, const char *inquiry) const;
    std::string talkerGetQuestionResponse(
        Conversation *cnv, const char *answer
    ) const;
    std::string beggarGetQuantityResponse(
        Conversation *cnv, const char *response
    ) const;
    static std::string lordBritishGetQuestionResponse(
        Conversation *cnv, const char *answer
    );
    static std::string getQuestion(const Conversation *cnv);

private:
    Dialogue *dialogue;
    MapCoords start;
    PersonNpcType npcType;
};

bool isPerson(Object *p_unknown);
std::list<std::string> replySplit(const std::string &text);
int linecount(const std::string &s, int column_max);

#endif // PERSON_H
