/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include <cstring>

#include "conversation.h"
#include "dialogueloader_tlk.h"
#include "u4file.h"
#include "utils.h"

DialogueLoader *U4TlkDialogueLoader::instance =
    DialogueLoader::registerLoader(
        new U4TlkDialogueLoader, "application/x-u4tlk"
    );


/**
 * A dialogue loader for standard u4dos .tlk files.
 */
Dialogue *U4TlkDialogueLoader::load(void *source)
{
    U4FILE *file = static_cast<U4FILE *>(source);
    enum QTrigger {
        NONE = 0,
        JOB = 3,
        HEALTH = 4,
        KEYWORD1 = 5,
        KEYWORD2 = 6
    };
    /* there's no dialogues left in the file */
    char tlk_buffer[384];
    if (u4fread(tlk_buffer, 1, sizeof(tlk_buffer), file)
        != sizeof(tlk_buffer)) {
        return nullptr;
    }
    char *ptr = &tlk_buffer[3];
    std::vector<std::string> strings;
    for (int i = 0; i < 12; i++) {
        strings.push_back(ptr);
        ptr += std::strlen(ptr) + 1;
    }
    Dialogue *dlg = new Dialogue();
    unsigned char prob = tlk_buffer[2];
    QTrigger qtrigger = QTrigger(tlk_buffer[0]);
    bool humilityTestQuestion = tlk_buffer[1] == 1;
    dlg->setTurnAwayProb(prob);
    dlg->setName(strings[0]);
    dlg->setPronoun(strings[1]);
    dlg->setPrompt("\nDein Begehr:\n?");
#if 0 // Not needed for corrected German files
    // Fix the actor description string, converting the first character
    // to lower-case.
    strings[2][0] = xu4_tolower(strings[2][0]);
    // ... then replace any newlines in the string with spaces
    std::size_t index = strings[2].find("\n");
    while (index != std::string::npos) {
        strings[2][index] = ' ';
        index = strings[2].find("\n");
    }
    // ... then append a period to the end of the string if one does
    // not already exist
    if (!std::ispunct(strings[2][strings[2].length() - 1])) {
        strings[2] = strings[2] + std::string(".");
    }
    // ... and finally, a few characters in the game have descriptions
    // that do not begin with a definite (the) or indefinite (a/an)
    // article.  On those characters, insert the appropriate article.
    if ((strings[0] == "Iolo")
        || (strings[0] == "Tracie")
        || (strings[0] == "Dupre")
        || (strings[0] == "Traveling Dan")) {
        strings[2] = std::string("a ") + strings[2];
    }
#endif // if 0
    std::string introBase = std::string("\nDu triffst ") + strings[2] + "\n";
    Response *intro = new Response(uppercase(introBase) + dlg->getPrompt());
    intro->add(ResponsePart::STARTMUSIC_SILENCE);
    dlg->setIntro(intro);
    Response *longIntro = new Response(
        uppercase(
            introBase +
            "\n" +
            dlg->getPronoun() +
            " sagt:\nIch bin "
            + dlg->getName()
            +
            ".\n"
            + dlg->getPrompt()
        )
    );
    longIntro->add(ResponsePart::STARTMUSIC_SILENCE);
    dlg->setLongIntro(longIntro);
    dlg->setDefaultAnswer(
        new Response(uppercase(
                         dlg->getPronoun()
                         + " sagt:\nDamit kann ich dir nicht helfen."
                     ))
    );
    Response *yes = new Response(
        uppercase(dlg->getPronoun() + " sagt:\n" + strings[8])
    );
    Response *no = new Response(
        uppercase(dlg->getPronoun() + " sagt:\n" + strings[9])
    );
    if (humilityTestQuestion) {
        yes->add(ResponsePart::BRAGGED);
        no->add(ResponsePart::HUMBLE);
    }
    dlg->setQuestion(
        new Dialogue::Question(
            uppercase(dlg->getPronoun() + " fragt:\n" + strings[7]), yes, no
        )
    );
    // one of the following four keywords triggers the speaker's question
    Response *job = new Response(
        uppercase(dlg->getPronoun() + " sagt:\n" + strings[3])
    );
    Response *health = new Response(
        uppercase(dlg->getPronoun() + " sagt:\n" + strings[4])
    );
    /* Ignore empty answers which have keyword "A   ", response "A"
       Otherwise those interfere with keyword "ADE" (bye). */
    Response *kw1 = NULL;
    if (strings[5] != "A") {
        kw1 = new Response(
            uppercase(dlg->getPronoun() + " sagt:\n" + strings[5])
        );
    }
    Response *kw2 = NULL;
    if (strings[6] != "A") {
        kw2 = new Response(
            uppercase(dlg->getPronoun() + " sagt:\n" + strings[6])
        );
    }
    switch (qtrigger) {
    case JOB:
        job->add(ResponsePart::ASK);
        break;
    case HEALTH:
        health->add(ResponsePart::ASK);
        break;
    case KEYWORD1:
        if (kw1) kw1->add(ResponsePart::ASK);
        break;
    case KEYWORD2:
        if (kw2) kw2->add(ResponsePart::ASK);
        break;
    case NONE:
    default:
        break;
    }
    dlg->addKeyword("beruf", job);
    dlg->addKeyword("was", job);
    dlg->addKeyword("gesundheit", health);
    dlg->addKeyword("wie", health);
    if (kw1) dlg->addKeyword(strings[10], kw1);
    if (kw2) dlg->addKeyword(strings[11], kw2);
    // NOTE: We let the talker's custom keywords override the standard
    // keywords like HEAL and LOOK.  This behavior differs from u4dos,
    // but fixes a couple conversation files which have keywords that
    // conflict with the standard ones (e.g. Calabrini in Moonglow has
    // HEAL for healer, which is unreachable in u4dos, but clearly
    // more useful than "Fine." for health).
    std::string look = std::string("Du siehst ") + strings[2];
    dlg->addKeyword("schauen", new Response(uppercase(look)));
    dlg->addKeyword("sieh", new Response(uppercase(look)));
    dlg->addKeyword("sehen", new Response(uppercase(look)));
    Response *name = new Response(uppercase(
                                      dlg->getPronoun()
                                      + " sagt:\nIch bin "
                                      + dlg->getName()
                                      + "."
                                  ));
    dlg->addKeyword("name", name);
    dlg->addKeyword("wer", name);
    Response *gib =
        new Response(uppercase(
                         dlg->getPronoun()
                         + " sagt:\nIch brauche dein Gold nicht. "
                         "Behalt es!"
                     ));

    dlg->addKeyword("geben", gib);
    dlg->addKeyword("gib", gib);
    dlg->addKeyword(
        "begleiten",
        new Response(uppercase(
                         dlg->getPronoun()
                         + " sagt:\nIch kann dich nicht begleiten."
                     ))
    );
    Response *bye = new Response(
        uppercase(dlg->getPronoun() + " sagt:\nAde.")
    );
    bye->add(ResponsePart::STOPMUSIC);
    bye->add(ResponsePart::END);
    dlg->addKeyword("ade", bye);
    dlg->addKeyword("", bye);
    /*
     * This little easter egg appeared in the Amiga version of Ultima IV.
     * I've never figured out what the number means.
     * "Banjo" Bob Hardy was the programmer for the Amiga version.
     */
    dlg->addKeyword(
        "ojnab",
        new Response(uppercase(
                         dlg->getPronoun()
                         + " sagt:\nHallo Banjo Bob! Deine geheime Zahl ist "
                         "4F4A4E0A"
                     ))
    );
    return dlg;
} // U4TlkDialogueLoader::load
