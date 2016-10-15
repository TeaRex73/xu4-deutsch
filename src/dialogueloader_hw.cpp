/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include <cstring>
#include <vector>

#include "context.h"
#include "conversation.h"
#include "dialogueloader_hw.h"
#include "player.h"
#include "savegame.h"
#include "u4file.h"
#include "utils.h"



Response *hawkwindGetAdvice(const DynamicResponse *kw);
Response *hawkwindGetIntro(const DynamicResponse *dynResp);

/* Hawkwind text indexes */
#define HW_SPEAKONLYWITH 40
#define HW_RETURNWHEN 41
#define HW_ISREVIVED 42
#define HW_WELCOME 43
#define HW_GREETING1 44
#define HW_GREETING2 45
#define HW_PROMPT 46
#define HW_DEFAULT 49
#define HW_ALREADYAVATAR 50
#define HW_GOTOSHRINE 51
#define HW_BYE 52

std::vector<std::string> hawkwindText;
DialogueLoader *U4HWDialogueLoader::instance = DialogueLoader::registerLoader(
    new U4HWDialogueLoader, "application/x-u4hwtlk"
);


/**
 * A special case dialogue loader for Hawkwind.
 */
Dialogue *U4HWDialogueLoader::load(void *source)
{
    U4FILE *hawkwind = NULL;
    switch (c->party->member(0)->getSex()) {
    case SEX_MALE:
        hawkwind = u4fopen("hawkwinm.ger");
        break;
    case SEX_FEMALE:
        hawkwind = u4fopen("hawkwinf.ger");
        break;
    default:
        ASSERT(0, "Invalid Sex %d!", c->party->member(0)->getSex());
    }
    if (!hawkwind) {
        return NULL;
    }
    hawkwindText = u4read_stringtable(hawkwind, 0, 53);
    u4fclose(hawkwind);
    Dialogue *dlg = new Dialogue();
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
    for (int v = 0; v < VIRT_MAX; v++) {
        std::string virtue(getVirtueName((Virtue)v));
        lowercase(virtue);
        virtue = virtue.substr(0, 4);
        dlg->addKeyword(
            virtue, new DynamicResponse(&hawkwindGetAdvice, virtue)
        );
    }
    Response *bye = new Response(uppercase(hawkwindText[HW_BYE]) + "\n");
    bye->add(ResponsePart::STOPMUSIC);
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
    for (int v = 0; v < VIRT_MAX; v++) {
        if (xu4_strncasecmp(
                dynResp->getParam().c_str(), getVirtueName((Virtue)v), 4
            ) == 0) {
            virtue = v;
            virtueLevel = c->saveGame->karma[v];
            break;
        }
    }
    if (virtue != -1) {
        text = "\n";
        if (virtueLevel == 0) {
            text += hawkwindText[HW_ALREADYAVATAR] + "\n";
        } else if (virtueLevel < 80) {
            text += hawkwindText[(virtueLevel / 20) * 8 + virtue];
        } else if (virtueLevel < 99) {
            text += hawkwindText[3 * 8 + virtue];
        } else { /* virtueLevel >= 99 */
            text += hawkwindText[4 * 8 + virtue]
                + " "
                + hawkwindText[HW_GOTOSHRINE];
        }
    } else {
        text = std::string("\n") + hawkwindText[HW_DEFAULT];
    }
    return new Response(uppercase(text));
} // hawkwindGetAdvice

Response *hawkwindGetIntro(const DynamicResponse *dynResp)
{
    Response *intro = new Response("");
    
    if ((c->party->member(0)->getStatus() == STAT_SLEEPING)
        || (c->party->member(0)->getStatus() == STAT_DEAD)) {
        intro->add(uppercase(
                       "\n\n"
                       + hawkwindText[HW_SPEAKONLYWITH]
                       + " "
                       + c->party->member(0)->getName()
                       + " "
                       + hawkwindText[HW_RETURNWHEN]
                       + " "
                       + c->party->member(0)->getName()
                       + " "
                       + hawkwindText[HW_ISREVIVED] + "\n"
                   ));
        intro->add(ResponsePart::END);
    } else {
        intro->add(ResponsePart::STARTMUSIC_HW);
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
