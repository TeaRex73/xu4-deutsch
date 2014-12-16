/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
#include <vector>

#include "context.h"
#include "conversation.h"
#include "dialogueloader_lb.h"
#include "player.h"
#include "savegame.h"
#include "u4file.h"

using std::string;
using std::vector;

Response *lordBritishGetHelp(const DynamicResponse *resp);
Response *lordBritishGetIntro(const DynamicResponse *resp);

DialogueLoader* U4LBDialogueLoader::instance = DialogueLoader::registerLoader(new U4LBDialogueLoader, "application/x-u4lbtlk");

/**
 * A special case dialogue loader for Lord British.  Loads most of the
 * keyword/responses from a hardcoded location in avatar.exe.  The
 * "help" response is a special case that changes based on the
 * current party status.
 */
Dialogue* U4LBDialogueLoader::load(void *source) {
    U4FILE *britkey = u4fopen("britkey.ger");
    U4FILE *britansw = u4fopen("britansw.ger");
    if (!britkey || !britansw)
        return NULL;

    vector<string> lbKeywords = u4read_stringtable(britkey, 0, 27);
    vector<string> lbText = u4read_stringtable(britansw, 0, 27);
    /* There's a \0 in the 19th string so we get a
       spurious 20th entry. No we don't.
    for (int i = 20; i < 24; i++)
        lbText[i] = lbText[i+1];
    lbText.pop_back();
    */

    Dialogue *dlg = new Dialogue();
    dlg->setTurnAwayProb(0);

    dlg->setName("Lord British");
    dlg->setPronoun("Er");
    dlg->setPrompt("Dein Begehr:\n?");
    Response *intro = new DynamicResponse(&lordBritishGetIntro);
    dlg->setIntro(intro);
    dlg->setLongIntro(intro);
    dlg->setDefaultAnswer(new Response("Er sagt:\nDamit kann ich dir nicht helfen.\n"));

    for (unsigned i = 0; i < lbKeywords.size(); i++) {
        dlg->addKeyword(lbKeywords[i], new Response(lbText[i] + "\n"));
    }

    /* since the original game files are a bit sketchy on the 'abyss' keyword,
       let's handle it here just to be safe :). Or not.
    dlg->addKeyword("abgrund",
                    new Response("\n\nEr sagt:\nDer Gro~e Stygische Abgrund ist die dunkelste Nische "
                                 "des B|sen, die in Britannia }brig ist.\n\nMan sagt, da~ in den tiefsten "
                                 "H|hlungen des Abgrunds die Kammer des Codex liegt!\n\nMan sagt auch, da~ nur "
                                 "eine Person h|chster Tugend diese Kammer betreten darf, so jemand wie ein "
                                 "Avatar!!!\n"));
    */
    Response *heal = new Response("Er sagt:\nMir geht es gut, danke.");
    heal->add(ResponsePart::HEALCONFIRM);
    dlg->addKeyword("gesu", heal);

    Response *bye = NULL;
    if (c->party->size() > 1)
        bye = new Response("Er sagt:\nLebt wohl, meine Freunde!");
    else
	switch (c->party->member(0)->getSex()) {
	case SEX_MALE:
            bye = new Response("Er sagt:\nLebe wohl, mein Freund!");
	    break;
	case SEX_FEMALE:
	     bye = new Response("Er sagt:\nLebe wohl, meine Freundin!");
	     break;
	default:
	     ASSERT(0, "Invalid Sex %d", c->party->member(0)->getSex());
	}
    bye->add(ResponsePart::STOPMUSIC);
    bye->add(ResponsePart::END);
    dlg->addKeyword("ade", bye);
    dlg->addKeyword("", bye);

    dlg->addKeyword("helf", new DynamicResponse(&lordBritishGetHelp));
    dlg->addKeyword("hilf", new DynamicResponse(&lordBritishGetHelp));
    dlg->addKeyword("beis", new DynamicResponse(&lordBritishGetHelp));

    return dlg;
}

/**
 * Generate the appropriate response when the player asks Lord British
 * for help.  The help text depends on the current party status; when
 * one quest item is complete, Lord British provides some direction to
 * the next one.
 */
Response *lordBritishGetHelp(const DynamicResponse *resp) {
    int v;
    bool fullAvatar, partialAvatar;
    string text;

    /*
     * check whether player is full avatar (in all virtues) or partial
     * avatar (in at least one virtue)
     */
    fullAvatar = true;
    partialAvatar = false;
    for (v = 0; v < VIRT_MAX; v++) {
        fullAvatar &= (c->saveGame->karma[v] == 0);
        partialAvatar |= (c->saveGame->karma[v] == 0);
    }

    if (c->saveGame->moves <= 1000) {
        text = string("Um in diesem feindlichen Lande zu }berleben, mu~t Du dich zuerst selbst kennen! "
               "Versuche, deine Waffen und deine Magie zu meistern!\n\nSei sehr vorsichtig auf diesen "
               "deinen ersten Reisen durch Britannia.\n\nBis du dich selbst gut kennst, entferne dich "
	           "nicht weit von der Sicherheit der St{dte!\n");
    }

    else if (c->saveGame->members == 1) {
	text = string("Reise nicht allein durchs offene Land.\n\nEs gibt viele w}rdige Leute in den "
               "verschiedenen St{dten, bei denen es weise w{re, sie zu bitten, dich zu begleiten.\n\n"
               "Baue deine Gruppe auf acht Reisende aus, denn nur ") +
			   string((c->party->member(0)->getSex() == SEX_MALE) ? "ein wahrer Anf}hrer" : "eine wahre Anf}hrerin") +
			   string(" kann die Queste gewinnen!\n");
    }

    else if (c->saveGame->runes == 0) {
	text = string("Lerne die Pfade der Tugend. Versuche, Zutritt zu den acht Schreinen zu erlangen!\n\n"
	       "Finde die Runen, die man zum Eintritte in jeden Schrein braucht, und lerne jeden "
           "Gesang, oder \"Mantra\", um Meditationen zu fokussieren.\n\nIn den Schreinen wirst du von den "
           "Taten lernen, die Tugend oder Laster in deinem Inneren zeigen!\n\nW{hle deinen Pfad weise, denn alle guten "
	       "und b|sen Taten werden erinnert und k|nnen zur}ckkehren, dich aufzuhalten!\n");
    }

    else if (!partialAvatar) {
	text = string("Besuche h{ufig den Seher Hawkwind und verwende seine Weisheit, um dir zu "
           "helfen, deine Tugend zu erweisen.\n\nWenn du bereit bist, wird Hawkwind dir raten, "
           "die Erh|hung zum teilweisen Avatartum in einer Tugend zu erlangen.\n\n"
           "Versuche, ein Teil-Avatar in allen acht Tugenden zu werden, denn erst dann wirst du "
	       "bereit sein, den Kodex zu suchen!\n");
    }

    else if (c->saveGame->stones == 0) {
	text = string("Gehe nun in die Tiefen der H|hlen. Nimm dort die 8 farbigen Steine "
           "von den Altar-Podesten in den G{ngen der H|hlen an dich.\n\nFinde die "
           "Verwendungen dieser Steine heraus, denn sie k|nnen dir im Abgrunde helfen!\n");
    }

    else if (!fullAvatar) {
        text = string("Du kommst wirklich sehr gut voran auf dem Pfad zum Avatartume! "
			   "Versuche, die Erh|hung in allen acht Tugenden zu erlangen!\n");
    }

    else if ((c->saveGame->items & ITEM_BELL) == 0 ||
             (c->saveGame->items & ITEM_BOOK) == 0 ||
             (c->saveGame->items & ITEM_CANDLE) == 0) {
        text = string("Finde die Glocke, das Buch und die Kerze! "
			   "Mit diesen drei Dingen darf man den Gro~en Stygischen Abgrund betreten!\n");
    }

    else if ((c->saveGame->items & ITEM_KEY_C) == 0 ||
             (c->saveGame->items & ITEM_KEY_L) == 0 ||
             (c->saveGame->items & ITEM_KEY_T) == 0) {
	text = string("Bevor du den Abgrund betrittst, ben|tigst du den Dreiteiligen Schl}ssel, und das Wort des Durchlasses.\n\n"
	       "Dann darfst du die Kammer des Kodexes der Ultimativen Weisheit betreten!\n");
    }

    else {
	text = string("Du erscheinst nun bereit, die letzte Reise in den dunklen Abgrund anzutreten! "
           "Gehe nur mit eine Gruppe von acht!\n\nViel Gl}ck, und m|gen die M{chte des Guten "
           "}ber dich wachen auf dieser deiner gefahrvollsten Unternehmung!\n\nDie Herzen "
           "und Seelen von ganz Britannia gehen jetzt mit dir. Sei vorsichtig, ") +
           ((c->party->member(0)->getSex() == SEX_MALE) ? "mein Freund.\n" : "meine Freundin.\n");
    }

    return new Response(string("Er sagt:\n") + text);
}

Response *lordBritishGetIntro(const DynamicResponse *resp) {
    Response *intro = new Response("");
    intro->add(ResponsePart::STARTMUSIC_LB);

    if (c->saveGame->lbintro) {
        if (c->saveGame->members == 1) {
            intro->add(string("\n\nDu triffst Lord British.\n\nEr sagt:\nWilkommen\n") +
                       c->party->member(0)->getName() + "!\n");
        }
        else if (c->saveGame->members == 2) {
            intro->add(string("\n\nDu triffst Lord British.\n\nEr sagt:\nWilkommen\n") +
                       c->party->member(0)->getName() +
                       ", und auch du " +
                       c->party->member(1)->getName() +
                       "!\n");
        }
        else {
            intro->add(string("\n\nDu triffst Lord British.\n\nEr sagt:\nWilkommen\n") +
                       c->party->member(0)->getName() +
                       " und deine gesch{tzten Abenteurer!\n");
        }

        // Lord British automatically adds "What would thou ask of me?"

        // Check levels here, just like the original!
        intro->add(ResponsePart::ADVANCELEVELS);
    }

    else {
        intro->add(string("\n\nDu triffst Lord British.\n\nEr erhebt sich und sagt: Endlich! ") +
                   c->party->member(0)->getName() +
                   string(", du bist gekommen! Wir haben so eine lange, lange Zeit gewartet...\n\n"
                   "Er setzt sich und sagt: Ein neues Zeitalter liegt auf "
                   "Britannia.\n\nDie Gro~en ]blen Herrscher sind fort, aber unserem Volke "
                   "mangelt es an Richtung und Zweck in ihren Leben...\n\n") +
				   string((c->party->member(0)->getSex() == SEX_MALE) ? "Ein Vork{mpfer" : "Eine Vork{mpferin") +
                   string(" der Tugend wird gebraucht. Du k|nntest ") +
				   string((c->party->member(0)->getSex() == SEX_MALE) ? "dieser Vork{mpfer" : "diese Vork{mpferin") +
				   string(" sein, aber nur "
                   "die Zukunft wird es zeigen.\n\nIch werde dir in jeder Weise beistehen, in der "
                   "ich kann!\n\nDein Begehr:\n?"));
        c->saveGame->lbintro = 1;
    }

    return intro;
}
