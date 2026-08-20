/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <vector>

#include "conversation.h"

#include "debug.h"
#include "music.h"
#include "script.h"
#include "utils.h"

/* Static variable initialization */
const ResponsePart ResponsePart::NONE("<NONE>", "", true);
const ResponsePart ResponsePart::ASK("<ASK>", "", true);
const ResponsePart ResponsePart::END("<END>", "", true);
const ResponsePart ResponsePart::ATTACK("<ATTACK>", "", true);
const ResponsePart ResponsePart::BRAGGED("<BRAGGED>", "", true);
const ResponsePart ResponsePart::HUMBLE("<HUMBLE>", "", true);
const ResponsePart ResponsePart::ADVANCE_LEVELS("<ADVANCE_LEVELS>", "", true);
const ResponsePart ResponsePart::HEAL_CONFIRM("<HEAL_CONFIRM>", "", true);
const ResponsePart ResponsePart::START_MUSIC_LB("<START_MUSIC_LB>", "", true);
const ResponsePart ResponsePart::START_MUSIC_HW("<START_MUSIC_HW>", "", true);
const ResponsePart ResponsePart::START_MUSIC_SILENCE(
    "<START_MUSIC_SILENCE>", "", true
);
const ResponsePart ResponsePart::STOP_MUSIC("<STOP_MUSIC>", "", true);
const ResponsePart ResponsePart::HAWKWIND("<HAWKWIND>", "", true);
const ResponsePart ResponsePart::DISK_LOAD("<DISK_LOAD>", "", true);
const unsigned int Conversation::BUFFER_LEN = 16;


Response::Response(const std::string &response)
    :references(0)
{
    add(response);
}

void Response::add(const ResponsePart &part)
{
    parts.push_back(part);
}

const std::vector<ResponsePart> &Response::getParts()
{
    return parts;
}

Response::operator std::string() const
{
    std::string result;
    for (const auto &part: parts) {
        result += static_cast<std::string>(part);
    }
    return result;
}

Response *Response::add_ref()
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
    const std::string &value, const std::string &arg, const bool command
)
    :value(value), arg(arg), command(command)
{
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
    :Response(""), generator(generator), currentResponse(nullptr), param(param)
{
}

DynamicResponse::~DynamicResponse()
{
    delete currentResponse;
}

const std::vector<ResponsePart> &DynamicResponse::getParts()
{
    delete currentResponse;
    currentResponse = (*generator)(this);
    return currentResponse->getParts();
}


/*
 * Dialogue::Question class
 */
Dialogue::Question::Question(
    const std::string &txt, Response *yes, Response *no
)
    :text(txt), yes_resp(yes->add_ref()), no_resp(no->add_ref())
{
}

Dialogue::Question::~Question()
{
    yes_resp->release();
    no_resp->release();
}

std::string Dialogue::Question::getText() const
{
    return text;
}

Response *Dialogue::Question::getResponse(const bool yes) const
{
    if (yes) {
        return yes_resp;
    }
    return no_resp;
}


/*
 * Dialogue::Keyword class
 */
Dialogue::Keyword::Keyword(const std::string &kw, Response *resp)
    :keyword(kw), response(resp->add_ref())
{
    trim(keyword);
    lowercase(keyword);
}

Dialogue::Keyword::Keyword(const std::string &kw, const std::string &resp)
    :keyword(kw), response((new Response(resp))->add_ref())
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
    const int testLen =
        keyword.size() < 4 ? static_cast<int>(keyword.size()) : 4;
    // exception: empty keyword only matches
    // empty std::string (alias for 'bye')
    if (testLen == 0 && !kw.empty()) {
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
     turnAwayProb(0),
     attackProb(0),
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
    for (const auto &keyword: keywords) {
        delete keyword.second;
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
    KeywordMap::const_iterator i = keywords.find(kw);
    // If they entered the keyword verbatim, return it!
    if (i != keywords.cend()) {
        return i->second;
    }
    // Otherwise, go find one that fits the description.
    for (i = keywords.cbegin(); i != keywords.cend(); ++i) {
        if (*i->second == kw) {
            return i->second;
        }
    }
    return nullptr;
}

const ResponsePart &Dialogue::getAction() const
{
    const int prob = xu4_random(0x100);
    /* Does the person turn away from/attack you? */
    if (prob >= turnAwayProb) {
        return ResponsePart::NONE;
    }
    musicMgr->play();
    if (prob >= attackProb) {
        return ResponsePart::END;
    }
    return ResponsePart::ATTACK;
}

std::string Dialogue::dump(const std::string &arg)
{
    std::string result;

    if (arg.empty()) {
        result = "keywords:\n";
        for (const auto &keyword: keywords) {
            result += keyword.first + "\n";
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
    :state(INTRO),
     script(new Script()),
     question(nullptr),
     quant(0),
     player(0),
     price(0),
     logger(new Debug("debug/conversation.txt", "Conversation"))
{
}

Conversation::~Conversation()
{
    delete logger;
    delete script;
}

Conversation::InputType Conversation::getInputRequired(int *bufferLen) const
{
    switch (state) {
    case BUY_QUANTITY:
    case SELL_QUANTITY:
        *bufferLen = 2;
        return INPUT_STRING;
    case TALK:
    case BUY_PRICE:
    case TOPIC:
        *bufferLen = BUFFER_LEN;
        return INPUT_STRING;
    case GIVE_BEGGAR:
        *bufferLen = 2;
        return INPUT_STRING;
    case ASK:
    case ASK_YES_NO:
    case CONFIRMATION:
        *bufferLen = 4;
        return INPUT_STRING;
    case VENDOR_QUESTION:
    case BUY_ITEM:
    case SELL_ITEM:
    case CONTINUE_QUESTION:
    case PLAYER:
        return INPUT_CHARACTER;
    case ATTACK:
    case DONE:
    case INTRO:
    case FULL_HEAL:
    case ADVANCE_LEVELS:
        return INPUT_NONE;
    default:
        U4ASSERT(0, "invalid state: %d", state);
    } // switch
    return INPUT_NONE;
}
