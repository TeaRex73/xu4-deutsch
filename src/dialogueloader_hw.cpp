/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include <vector>

#include "dialogueloader_hw.h"

#include "context.h"
#include "conversation.h"
#include "debug.h"
#include "dialogueloader.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "u4file.h"
#include "utils.h"


static Response *hawkwindGetAdvice(const DynamicResponse *dynResp);
static Response *hawkwindGetIntro(const DynamicResponse *dynResp);

/* Hawkwind text indexes */
#define HW_SPEAK_ONLY_WITH 40
#define HW_RETURN_WHEN 41
#define HW_IS_REVIVED 42
#define HW_WELCOME 43
#define HW_GREETING1 44
#define HW_GREETING2 45
#define HW_PROMPT 46
#define HW_DEFAULT 49
#define HW_ALREADY_AVATAR 50
#define HW_GOTO_SHRINE 51
#define HW_BYE 52

static std::vector<std::string> hawkwindText;
DialogueLoader *U4HWDialogueLoader::instance = registerLoader(
    new U4HWDialogueLoader, "application/x-u4hwtlk"
);


/**
 * A special case dialogue loader for Hawkwind.
 */
Dialogue *U4HWDialogueLoader::load(void *)
{
    U4FILE *hawkwind = nullptr;
    switch (c->party->member(0)->getSex()) {
    case SEX_MALE:
        hawkwind = u4fopen("hawkwinm.ger");
        break;
    case SEX_FEMALE:
        hawkwind = u4fopen("hawkwinf.ger");
        break;
    default:
        U4ASSERT(0, "Invalid Sex %d!", c->party->member(0)->getSex());
    }
    if (!hawkwind) {
        return nullptr;
    }
    hawkwindText = u4read_stringtable(hawkwind, 0, 53);
    u4fclose(hawkwind);
    auto *dlg = new Dialogue();
    dlg->setTurnAwayProb(0);
    dlg->setName(uppercase("Hawkwind"));
    dlg->setPronoun(uppercase("Er"));
    dlg->setPrompt("\n\n" + uppercase(hawkwindText[HW_PROMPT]) + "\n?");
    Response *intro = new DynamicResponse(&hawkwindGetIntro);
    dlg->setIntro(intro);
    dlg->setLongIntro(intro);
    dlg->setDefaultAnswer(
        new Response("\n" + uppercase(hawkwindText[HW_DEFAULT]))
    );
    for (int v = 0; v < VIRTUE_MAX; v++) {
        std::string virtue(getVirtueName(static_cast<Virtue>(v)));
        lowercase(virtue);
        if (virtue.size() > 4) {
            virtue.resize(4);
        }
        dlg->addKeyword(
            virtue, new DynamicResponse(&hawkwindGetAdvice, virtue)
        );
    }
    auto *bye = new Response(uppercase(hawkwindText[HW_BYE]) + "\n");
    bye->add(ResponsePart::STOP_MUSIC);
    bye->add(ResponsePart::END);
    dlg->addKeyword("ade", bye);
    dlg->addKeyword("", bye);
    dlg->addKeyword("kein", bye);
    return dlg;
} // U4HWDialogueLoader::load


/**
 * Asking Hawkwind about Virtues - the only thing he can talk about
 */
Response *hawkwindGetAdvice(const DynamicResponse *dynResp)
{
    std::string text;
    int virtue = -1, virtueLevel = -1;
    /* check if asking about a virtue */
    for (int v = 0; v < VIRTUE_MAX; v++) {
        if (xu4_strncasecmp(
                dynResp->getParam().c_str(),
                getVirtueName(static_cast<Virtue>(v)), 4
            ) == 0) {
            virtue = v;
            virtueLevel = c->saveGame->karma[v];
            break;
        }
    }
    if (virtue != -1) {
        text = "\n";
        if (virtueLevel == 0) {
            text += hawkwindText[HW_ALREADY_AVATAR] /* + "\n" */;
        } else if (virtueLevel < 80) {
            text += hawkwindText[virtueLevel / 20 * 8 + virtue];
        } else if (virtueLevel < 99) {
            text += hawkwindText[3 * 8 + virtue];
        } else { /* virtueLevel >= 99 */
            text += hawkwindText[4 * 8 + virtue]
                + " "
                + hawkwindText[HW_GOTO_SHRINE];
        }
    } else {
        text = std::string("\n") + hawkwindText[HW_DEFAULT];
    }
    return new Response(uppercase(text));
} // hawkwindGetAdvice

Response *hawkwindGetIntro(const DynamicResponse *)
{
    auto *intro = new Response("");

    if (c->party->member(0)->getStatus() == STAT_SLEEPING
        || c->party->member(0)->getStatus() == STAT_DEAD) {
        intro->add(uppercase(
                       "\n\n"
                       + hawkwindText[HW_SPEAK_ONLY_WITH]
                       + " "
                       + c->party->member(0)->getName()
                       + " "
                       + hawkwindText[HW_RETURN_WHEN]
                       + " "
                       + c->party->member(0)->getName()
                       + " "
                       + hawkwindText[HW_IS_REVIVED] + "\n"
                   ));
        intro->add(ResponsePart::STOP_MUSIC);
        intro->add(ResponsePart::END);
    } else {
        intro->add(ResponsePart::START_MUSIC_HW);
        intro->add(ResponsePart::HAWKWIND);
        intro->add(uppercase(
                       "\n\n"
                       + hawkwindText[HW_WELCOME]
                       + " "
                       + c->party->member(0)->getName()
                       + ", "
                       + hawkwindText[HW_GREETING1]
                       + "\n\n"
                       + hawkwindText[HW_GREETING2]
                       + "\n?"
                   ));
    }
    return intro;
}
