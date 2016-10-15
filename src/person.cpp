/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cctype>
#include <string>
#include <cstring>
#include <vector>
#include <cstdio>

#include "u4.h"

#include "person.h"

#include "city.h"
#include "context.h"
#include "conversation.h"
#include "debug.h"
#include "event.h"
#include "game.h" // Included for ReadPlayerController
#include "io.h"
#include "location.h"
#include "music.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "settings.h"
#include "stats.h"
#include "types.h"
#include "u4file.h"
#include "utils.h"
#include "script.h"

int chars_needed(
    const char *s, int columnmax, int linesdesired, int *real_lines
);


/**
 * Returns true of the object that 'punknown' points
 * to is a person object
 */
bool isPerson(Object *punknown)
{
    Person *p;
    if ((p = dynamic_cast<Person *>(punknown)) != NULL) {
        return true;
    } else {
        return false;
    }
}


/**
 * Splits a piece of response text into screen-sized chunks.
 */
std::list<std::string> replySplit(const std::string &text)
{
    std::string str = text;
    int pos, real_lines;
    std::list<std::string> reply;
    /* skip over any initial newlines */
    if ((pos = str.find("\n\n")) == 0) {
        str = str.substr(pos + 1);
    }
    unsigned int num_chars = chars_needed(
        str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines
    );
    /* we only have one chunk, no need to split it up */
#if 0
    unsigned int len = str.length();
    if (num_chars == len) {
        reply.push_back(str);
    } else {
#endif
        std::string pre = str.substr(0, num_chars);
        /* add the first chunk to the list */
        reply.push_back(pre);
        /* skip over any initial newlines */
#if 0
        if ((pos = str.find("\n\n")) == 0) {
            str = str.substr(pos + 1);
        }
#endif
        while (num_chars != str.length()) {
            /* go to the rest of the text */
            str = str.substr(num_chars);
            /* skip over any initial newlines */
#if 0
            if ((pos = str.find("\n\n")) == 0) {
                str = str.substr(pos + 1);
            }
#endif
            /* find the next chunk and add it */
            num_chars = chars_needed(
                str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines
            );
            pre = str.substr(0, num_chars);
            reply.push_back(pre);
        }
#if 0
    }
#endif
    return reply;
}  // replySplit

Person::Person(MapTile tile)
    :Creature(tile), start(0, 0)
{
    setType(Object::PERSON);
    dialogue = NULL;
    npcType = NPC_EMPTY;
}

Person::Person(const Person *p)
    :Creature(p->tile)
{
    *this = *p;
}

bool Person::canConverse() const
{
    return isVendor() || dialogue != NULL;
}

bool Person::isVendor() const
{
    return npcType >= NPC_VENDOR_WEAPONS && npcType <= NPC_VENDOR_STABLE;
}

std::string Person::getName() const
{
    if (dialogue) {
        return dialogue->getName();
    } else if (npcType == NPC_EMPTY) {
        return Creature::getName();
    } else {
        return "(unnamed person)";
    }
}

void Person::goToStartLocation()
{
    setCoords(start);
}

void Person::setDialogue(Dialogue *d)
{
    dialogue = d;
    if (tile.getTileType()->getName() == "beggar") {
        npcType = NPC_TALKER_BEGGAR;
    } else if (tile.getTileType()->getName() == "guard") {
        npcType = NPC_TALKER_GUARD;
    } else {
        npcType = NPC_TALKER;
    }
}

void Person::setNpcType(PersonNpcType t)
{
    npcType = t;
    ASSERT(!isVendor() || dialogue == NULL, "vendor has dialogue");
}

std::list<std::string> Person::getConversationText(
    Conversation *cnv, const char *inquiry
)
{
    std::string text;
    /*
     * a convsation with a vendor
     */
    if (isVendor()) {
        static const std::string ids[] = {
            "Weapons",
            "Armor",
            "Food",
            "Tavern",
            "Reagents",
            "Healer",
            "Inn",
            "Guild",
            "Stable"
        };
        Script *script = cnv->script;
        /**
         * We aren't currently running a script, load the
         appropriate one!
        */
        if (cnv->state == Conversation::INTRO) {
            // unload the previous script if it wasn't
            // already unloaded
            if (script->getState() != Script::STATE_UNLOADED) {
                script->unload();
            }
            script->load(
                "vendorScript.xml",
                ids[npcType - NPC_VENDOR_WEAPONS],
                "vendor",
                c->location->map->getName()
            );
            script->run("intro");
            while (script->getState() != Script::STATE_DONE) {
                // Gather input for the script
                if (script->getState() == Script::STATE_INPUT) {
                    switch (script->getInputType()) {
                    case Script::INPUT_CHOICE:
                    {
                        const std::string &choices = script->getChoices();
                        // Get choice
                        char val = ReadChoiceController::get(choices);
                        if (std::isspace(val) || (val == '\033')) {
                            script->unsetVar(script->getInputName());
                        } else {
                            std::string s_val;
                            s_val.resize(1);
                            s_val[0] = val;
                            script->setVar(script->getInputName(), s_val);
                        }
                        break;
                    }
                    case Script::INPUT_KEYPRESS:
                        ReadChoiceController::get(" \015\033");
                        break;
                    case Script::INPUT_NUMBER:
                    {
                        int val = ReadIntController::get(
                            script->getInputMaxLen(),
                            TEXT_AREA_X + c->col,
                            TEXT_AREA_Y + c->line
                        );
                        script->setVar(script->getInputName(), val);
                        break;
                    }
                    case Script::INPUT_STRING:
                    {
                        std::string str = ReadStringController::get(
                            script->getInputMaxLen(),
                            TEXT_AREA_X + c->col,
                            TEXT_AREA_Y + c->line
                        );
                        if (str.size()) {
                            lowercase(str);
                            str = str.substr(0, 4);
                            script->setVar(script->getInputName(), str);
                        } else {
                            script->unsetVar(script->getInputName());
                        }
                        break;
                    }
                    case Script::INPUT_PLAYER:
                    {
                        ReadPlayerController getPlayerCtrl;
                        eventHandler->pushController(&getPlayerCtrl);
                        int player = getPlayerCtrl.waitFor();
                        if (player != -1) {
                            std::string player_str =
                                std::to_string(player + 1);
                            script->setVar(script->getInputName(), player_str);
                        } else {
                            script->unsetVar(script->getInputName());
                        }
                        break;
                    }
                    default:
                        break;
                    } // switch
                      // Continue running the script!
                    c->line++;
                    script->_continue();
                } // if
            } // while
        } // if
        // Unload the script
        script->unload();
        cnv->state = Conversation::DONE;
    }
    /*
     * a conversation with a non-vendor
     */
    else {
        text = "\n\n\n";
        switch (cnv->state) {
        case Conversation::INTRO:
            text = getIntro(cnv);
            break;
        case Conversation::TALK:
            text += getResponse(cnv, inquiry) + "\n";
            break;
        case Conversation::CONFIRMATION:
            ASSERT(
                npcType == NPC_LORD_BRITISH, "invalid state: %d", cnv->state
            );
            text += lordBritishGetQuestionResponse(cnv, inquiry);
            break;
        case Conversation::ASK:
        case Conversation::ASKYESNO:
            ASSERT(
                npcType != NPC_HAWKWIND,
                "invalid state for hawkwind conversation"
            );
            text += talkerGetQuestionResponse(cnv, inquiry);
            break;
        case Conversation::GIVEBEGGAR:
            ASSERT(
                npcType == NPC_TALKER_BEGGAR, "invalid npc type: %d", npcType
            );
            text = beggarGetQuantityResponse(cnv, inquiry);
            break;
        case Conversation::FULLHEAL:
        case Conversation::ADVANCELEVELS:
            /* handled elsewhere */
            break;
        default:
            ASSERT(0, "invalid state: %d", cnv->state);
        }
    }
    return replySplit(text);
} // Person::getConversationText


/**
 * Get the prompt shown after each reply.
 */
std::string Person::getPrompt(Conversation *cnv)
{
    if (isVendor()) {
        return "";
    }
    std::string prompt;
    if (cnv->state == Conversation::ASK) {
        prompt = uppercase(getQuestion(cnv));
    } else if (cnv->state == Conversation::GIVEBEGGAR) {
        prompt = "Wie viel-";
    } else if (cnv->state == Conversation::CONFIRMATION) {
        prompt = uppercase(
            "\nEr fragt:\nGeht es dir gut?\n\nDeine Antwort:\n?"
        );
    } else if (cnv->state != Conversation::ASKYESNO) {
        prompt = uppercase(dialogue->getPrompt());
    }
    return prompt;
}


/**
 * Returns the valid keyboard choices for a given conversation.
 */
const char *Person::getChoices(Conversation *cnv)
{
    if (isVendor()) {
        return cnv->script->getChoices().c_str();
    }
    switch (cnv->state) {
    case Conversation::CONFIRMATION:
    case Conversation::CONTINUEQUESTION:
        return "nj\015 \033";
    case Conversation::PLAYER:
        return "012345678\015 \033";
    default:
        ASSERT(0, "invalid state: %d", cnv->state);
    }
    return NULL;
}

std::string Person::getIntro(Conversation *cnv)
{
    if (npcType == NPC_EMPTY) {
        cnv->state = Conversation::DONE;
        return std::string("KOMISCH, KEINE ANTWORT!\n");
    }
    // As far as I can tell, about 50% of the time they tell you their
    // name in the introduction
    Response *intro;
    if (xu4_random(2) == 0) {
        intro = dialogue->getIntro();
    } else {
        intro = dialogue->getLongIntro();
    }
    cnv->state = Conversation::TALK;
    std::string text = processResponse(cnv, intro);
    return text;
}

std::string Person::processResponse(Conversation *cnv, Response *response)
{
    std::string text;
    const std::vector<ResponsePart> &parts = response->getParts();
    for (std::vector<ResponsePart>::const_iterator i = parts.begin();
         i != parts.end();
         i++) {
        // check for command triggers
        if (i->isCommand()) {
            runCommand(cnv, *i);
        }
        // otherwise, append response part to reply
        else {
            text += *i;
        }
    }
    return text;
}

void Person::runCommand(Conversation *cnv, const ResponsePart &command)
{
    if (command == ResponsePart::ASK) {
        cnv->question = dialogue->getQuestion();
        cnv->state = Conversation::ASK;
    } else if (command == ResponsePart::END) {
        cnv->state = Conversation::DONE;
    } else if (command == ResponsePart::ATTACK) {
        cnv->state = Conversation::ATTACK;
    } else if (command == ResponsePart::BRAGGED) {
        c->party->adjustKarma(KA_BRAGGED);
    } else if (command == ResponsePart::HUMBLE) {
        c->party->adjustKarma(KA_HUMBLE);
    } else if (command == ResponsePart::ADVANCELEVELS) {
        cnv->state = Conversation::ADVANCELEVELS;
    } else if (command == ResponsePart::HEALCONFIRM) {
        cnv->state = Conversation::CONFIRMATION;
    } else if (command == ResponsePart::STARTMUSIC_LB) {
        musicMgr->lordBritish();
    } else if (command == ResponsePart::STARTMUSIC_HW) {
        musicMgr->hawkwind();
    } else if (command == ResponsePart::STARTMUSIC_SILENCE) {
        musicMgr->pause();
    } else if (command == ResponsePart::STOPMUSIC) {
        musicMgr->play();
    } else if (command == ResponsePart::HAWKWIND) {
        c->party->adjustKarma(KA_HAWKWIND);
    } else {
        ASSERT(
            0,
            "unknown command trigger in dialogue response: %s\n",
            std::string(command).c_str()
        );
    }
} // Person::runCommand

std::string Person::getResponse(Conversation *cnv, const char *inquiry)
{
    std::string reply;
    Virtue v;
    const ResponsePart &action = dialogue->getAction();
    reply = "\n";
    /* Does the person take action during the conversation? */
    if (action == ResponsePart::END) {
        runCommand(cnv, action);
        return uppercase(dialogue->getPronoun() + " wendet sich ab!\n");
    } else if (action == ResponsePart::ATTACK) {
        runCommand(cnv, action);
        return uppercase(getName() + " sagt:\nPa~ auf! Narr!");
    }
    if ((npcType == NPC_TALKER_BEGGAR)
        && ((xu4_strncasecmp(inquiry, "gib", 3) == 0)
            || (xu4_strncasecmp(inquiry, "gebe", 4) == 0))) {
        reply = "\b";
        cnv->state = Conversation::GIVEBEGGAR;
    } else if ((xu4_strncasecmp(inquiry, "begl", 4) == 0)
               && c->party->canPersonJoin(getName(), &v)) {
        CannotJoinError join = c->party->join(getName());
        if (join == JOIN_SUCCEEDED) {
            reply = dialogue->getPronoun()
                + " sagt:\nEs ist mir eine Ehre, dich zu begleiten!";
            c->location->map->removeObject(this);
        musicMgr->play();
            cnv->state = Conversation::DONE;
        } else {
            reply = dialogue->getPronoun() + " sagt:\nDu bist nicht ";
            reply += (join == JOIN_NOT_VIRTUOUS) ?
                getVirtueAdjective(v) :
                "erfahren";
            reply += " genug fuer mich, um dich zu begleiten.";
        }
    } else if ((*dialogue)[inquiry]) {
        Dialogue::Keyword *kw = (*dialogue)[inquiry];
        reply = processResponse(cnv, kw->getResponse());
    } else if (settings.debug && (xu4_strncasecmp(inquiry, "dump", 4) == 0)) {
        std::vector<std::string> words = split(inquiry, " \t");
        if (words.size() <= 1) {
            reply = dialogue->dump("");
        } else {
            reply = dialogue->dump(words[1]);
        }
    } else {
        reply = processResponse(cnv, dialogue->getDefaultAnswer());
    }
    return uppercase(reply);
} // Person::getResponse

std::string Person::talkerGetQuestionResponse(
    Conversation *cnv, const char *answer
)
{
    bool valid = false;
    bool yes;
    char ans = xu4_tolower(answer[0]);
    if ((ans == 'j') || (ans == 'n')) {
        valid = true;
        yes = ans == 'j';
    }
    if (!valid) {
        cnv->state = Conversation::ASKYESNO;
        return uppercase(
            dialogue->getPronoun()  + " fragt:\nJa oder nein?\n"
        ) + "\nDeine Antwort:\n?";
    }
    cnv->state = Conversation::TALK;
    return processResponse(cnv, cnv->question->getResponse(yes)) + "\n";
}

std::string Person::beggarGetQuantityResponse(
    Conversation *cnv, const char *response
)
{
    std::string reply;
    cnv->quant = (int)std::strtol(response, NULL, 10);
    cnv->state = Conversation::TALK;
    if (cnv->quant > 0) {
        if (c->party->donate(cnv->quant)) {
            reply = "\n\n\n";
            reply += dialogue->getPronoun();
            reply += " sagt:\nOh danke dir! Ich werde deine Freundlichkeit "
                "nie vergessen!\n";
        } else {
            reply = "\n\n\nSo viel Gold hast du nicht!\n";
        }
    } else {
        reply = "\n";
    }
    return uppercase(reply);
}

std::string Person::lordBritishGetQuestionResponse(
    Conversation *cnv, const char *answer
)
{
    std::string reply;
    cnv->state = Conversation::TALK;
    if (xu4_tolower(answer[0]) == 'j') {
        reply = "Er sagt:\nDas ist gut.\n\n";
    } else if (xu4_tolower(answer[0]) == 'n') {
        reply = "Er sagt:\nLa~ mich deine Wunden heilen!\n\n";
        cnv->state = Conversation::FULLHEAL;
    } else {
        reply = "Er sagt:\nDamit kann ich dir nicht helfen.\n\n";
    }
    return uppercase(reply);
}

std::string Person::getQuestion(Conversation *cnv)
{
    return uppercase("\n" + cnv->question->getText())
        + "\n\nDeine Antwort:\n?";

}

/**
 * Returns the number of characters needed to get to
 * the next line of text (based on column width).
 */
int chars_to_next_line(const char *s, int columnmax)
{
    int chars = -1;
    if (std::strlen(s) > 0) {
        int lastbreak = columnmax;
        chars = 0;
        for (const char *str = s; *str; str++) {
            if (*str == '\n') {
                return str - s;
            } else if ((*str == ' ') || (*str == '-')) {
                lastbreak = (str - s);
            } else if (++chars >= columnmax) {
                return lastbreak;
            }
        }
    }
    return chars;
}


/**
 * Counts the number of lines (of the maximum width given by
 * columnmax) in the string.
 */
int linecount(const std::string &s, int columnmax)
{
    int lines = 0;
    unsigned int ch = 0;
    while (ch < s.length()) {
        ch += chars_to_next_line(s.c_str() + ch, columnmax);
        if (ch < s.length()) {
            ch++;
        }
        lines++;
    }
    return lines;
}


/**
 * Returns the number of characters needed to produce a
 * valid screen of text (given a column width and row height)
 */
int chars_needed(
    const char *s, int columnmax, int linesdesired, int *real_lines
)
{
    int chars = 0, totalChars = 0;
    char *new_str = xu4_strdup(s), *str = new_str;
    // try breaking text into paragraphs first
    std::string text = s;
    std::string paragraphs;
    unsigned int pos;
    int lines = 0;
    while ((pos = text.find("\n\n")) < text.length()) {
        std::string p = text.substr(0, pos);
        lines += linecount(p.c_str(), columnmax);
        if (lines <= linesdesired) {
            paragraphs += p + "\n";
        } else {
            break;
        }
        text = text.substr(pos + 1);
    }
    // Seems to be some sort of clang compilation bug in this code,
    // that causes this addition to not work correctly.
    int totalPossibleLines = lines + linecount(text.c_str(), columnmax);
    if (totalPossibleLines <= linesdesired) {
        paragraphs += text;
    }
    if (!paragraphs.empty()) {
        *real_lines = lines;
        std::free(new_str);
        return paragraphs.length();
    } else {
        // reset variables and try another way
        lines = 1;
    }
    // gather all the line breaks
    while ((chars = chars_to_next_line(str, columnmax)) >= 0) {
        if (++lines >= linesdesired) {
            break;
        }
        int num_to_move = chars;
        if (*(str + num_to_move) == '\n') {
            num_to_move++;
        }
        totalChars += num_to_move;
        str += num_to_move;
    }
    std::free(new_str);
    *real_lines = lines;
    return totalChars;
} // chars_needed
