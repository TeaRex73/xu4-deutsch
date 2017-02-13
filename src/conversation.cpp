/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstring>
#include "conversation.h"
#include "debug.h"
#include "person.h"
#include "script.h"
#include "utils.h"

/* Static variable initialization */
const ResponsePart ResponsePart::NONE("<NONE>", "", true);
const ResponsePart ResponsePart::ASK("<ASK>", "", true);
const ResponsePart ResponsePart::END("<END>", "", true);
const ResponsePart ResponsePart::ATTACK("<ATTACK>", "", true);
const ResponsePart ResponsePart::BRAGGED("<BRAGGED>", "", true);
const ResponsePart ResponsePart::HUMBLE("<HUMBLE>", "", true);
const ResponsePart ResponsePart::ADVANCELEVELS("<ADVANCELEVELS>", "", true);
const ResponsePart ResponsePart::HEALCONFIRM("<HEALCONFIRM>", "", true);
const ResponsePart ResponsePart::STARTMUSIC_LB("<STARTMUSIC_LB>", "", true);
const ResponsePart ResponsePart::STARTMUSIC_HW("<STARTMUSIC_HW>", "", true);
const ResponsePart ResponsePart::STARTMUSIC_SILENCE(
    "<STARTMUSIC_SILENCE>", "", true
);
const ResponsePart ResponsePart::STOPMUSIC("<STOPMUSIC>", "", true);
const ResponsePart ResponsePart::HAWKWIND("<HAWKWIND>", "", true);
const unsigned int Conversation::BUFFERLEN = 16;


Response::Response(const std::string &response)
    :references(0)
{
    add(response);
}

void Response::add(const ResponsePart &part)
{
    parts.push_back(part);
}

const std::vector<ResponsePart> &Response::getParts() const
{
    return parts;
}

Response::operator std::string() const
{
    std::string result;
    for (std::vector<ResponsePart>::const_iterator i = parts.begin();
         i != parts.end();
         i++) {
        result += *i;
    }
    return result;
}

Response *Response::addref()
{
    references++;
    return this;
}

void Response::release()
{
    references--;
    if (references <= 0) {
        delete this;
    }
}

ResponsePart::ResponsePart(
    const std::string &value, const std::string &arg, bool command
)
{
    this->value = value;
    this->arg = arg;
    this->command = command;
}

ResponsePart::operator std::string() const
{
    return value;
}

bool ResponsePart::operator==(const ResponsePart &rhs) const
{
    return value == rhs.value;
}

bool ResponsePart::isCommand() const
{
    return command;
}

DynamicResponse::DynamicResponse(
    Response *(*generator)(const DynamicResponse *), const std::string &param
)
    :Response(""), param(param)
{
    this->generator = generator;
    currentResponse = nullptr;
}

DynamicResponse::~DynamicResponse()
{
    delete currentResponse;
}

const std::vector<ResponsePart> &DynamicResponse::getParts() const
{
    // blah, must cast away constness
    const_cast<DynamicResponse *>(this)->currentResponse = (*generator)(this);
    return currentResponse->getParts();
}


/*
 * Dialogue::Question class
 */
Dialogue::Question::Question(
    const std::string &txt, Response *yes, Response *no
)
    :text(txt), yesresp(yes->addref()), noresp(no->addref())
{
}

Dialogue::Question::~Question()
{
    yesresp->release();
    noresp->release();
}

std::string Dialogue::Question::getText()
{
    return text;
}

Response *Dialogue::Question::getResponse(bool yes)
{
    if (yes) {
        return yesresp;
    }
    return noresp;
}


/*
 * Dialogue::Keyword class
 */
Dialogue::Keyword::Keyword(const std::string &kw, Response *resp)
    :keyword(kw), response(resp->addref())
{
    trim(keyword);
    lowercase(keyword);
}

Dialogue::Keyword::Keyword(const std::string &kw, const std::string &resp)
    :keyword(kw), response((new Response(resp))->addref())
{
    trim(keyword);
    lowercase(keyword);
}

Dialogue::Keyword::~Keyword()
{
    response->release();
}

bool Dialogue::Keyword::operator==(const std::string &kw) const
{
    // minimum 4-character "guessing"
    int testLen = (keyword.size() < 4) ? keyword.size() : 4;
    // exception: empty keyword only matches
    // empty std::string (alias for 'bye')
    if ((testLen == 0) && (kw.size() > 0)) {
        return false;
    }
    if (xu4_strncasecmp(kw.c_str(), keyword.c_str(), testLen) == 0) {
        return true;
    }
    return false;
}


/*
 * Dialogue class
 */
Dialogue::Dialogue()
    :intro(nullptr),
     longIntro(nullptr),
     defaultAnswer(nullptr),
     question(nullptr)
{
}

Dialogue::~Dialogue()
{
    delete intro;
    if (longIntro != intro) {
        delete longIntro;
    }
    delete defaultAnswer;
    for (KeywordMap::iterator i = keywords.begin();
         i != keywords.end();
         i++) {
        delete i->second;
    }
    delete question;
}

void Dialogue::addKeyword(const std::string &kw, Response *response)
{
    if (keywords.find(kw) != keywords.end()) {
        delete keywords[kw];
    }
    keywords[kw] = new Keyword(kw, response);
}

Dialogue::Keyword *Dialogue::operator[](const std::string &kw)
{
    KeywordMap::iterator i = keywords.find(kw);
    // If they entered the keyword verbatim, return it!
    if (i != keywords.end()) {
        return i->second;
    }
    // Otherwise, go find one that fits the description.
    else {
        for (i = keywords.begin(); i != keywords.end(); i++) {
            if ((*i->second) == kw) {
                return i->second;
            }
        }
    }
    return nullptr;
}

const ResponsePart &Dialogue::getAction() const
{
    int prob = xu4_random(0x100);
    /* Does the person turn away from/attack you? */
    if (prob >= turnAwayProb) {
        return ResponsePart::NONE;
    } else {
        musicMgr->play();
        if (attackProb - prob < 0x40) {
            return ResponsePart::END;
        } else {
            return ResponsePart::ATTACK;
        }
    }
}

std::string Dialogue::dump(const std::string &arg)
{
    std::string result;

    if (arg == "") {
        result = "keywords:\n";
        for (KeywordMap::iterator i = keywords.begin();
             i != keywords.end();
             i++) {
            result += i->first + "\n";
        }
    } else if (keywords.find(arg) != keywords.end()) {
        result = static_cast<std::string>(*keywords[arg]->getResponse());
    }
    return result;
}


/*
 * Conversation class
 */
Conversation::Conversation()
    :state(INTRO), script(new Script()), logger(0)
{
    logger = new Debug("debug/conversation.txt", "Conversation");
}

Conversation::~Conversation()
{
    delete logger;
    delete script;
}

Conversation::InputType Conversation::getInputRequired(int *bufferlen)
{
    switch (state) {
    case BUY_QUANTITY:
    case SELL_QUANTITY:
        *bufferlen = 2;
        return INPUT_STRING;
    case TALK:
    case BUY_PRICE:
    case TOPIC:
        *bufferlen = BUFFERLEN;
        return INPUT_STRING;
    case GIVEBEGGAR:
        *bufferlen = 2;
        return INPUT_STRING;
    case ASK:
    case ASKYESNO:
    case CONFIRMATION:
        *bufferlen = 4;
        return INPUT_STRING;
    case VENDORQUESTION:
    case BUY_ITEM:
    case SELL_ITEM:
    case CONTINUEQUESTION:
    case PLAYER:
        return INPUT_CHARACTER;
    case ATTACK:
    case DONE:
    case INTRO:
    case FULLHEAL:
    case ADVANCELEVELS:
        return INPUT_NONE;
    default:
        ASSERT(0, "invalid state: %d", state);
    } // switch
    return INPUT_NONE;
}
