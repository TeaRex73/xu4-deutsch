/*
 * $Id$
 */

#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "utils.h"



class Debug;
class Person;
class Script;


/**
 * A response part can be text or a "command" that triggers an
 * action.
 */
class ResponsePart {
public:
    static const ResponsePart NONE;
    static const ResponsePart ASK;
    static const ResponsePart END;
    static const ResponsePart ATTACK;
    static const ResponsePart BRAGGED;
    static const ResponsePart HUMBLE;
    static const ResponsePart ADVANCELEVELS;
    static const ResponsePart HEALCONFIRM;
    static const ResponsePart STARTMUSIC_LB;
    static const ResponsePart STARTMUSIC_HW;
    static const ResponsePart STARTMUSIC_SILENCE;
    static const ResponsePart STOPMUSIC;
    static const ResponsePart HAWKWIND;
    ResponsePart(
        const std::string &value,
        const std::string &arg = "",
        bool command = false
    );
    operator std::string() const;
    bool operator==(const ResponsePart &rhs) const;
    bool isCommand() const;

private:
    std::string value, arg;
    bool command;
};


/**
 * A static response.  Each response can be made up of any number of
 * ResponseParts, which are either text fragments or commands.
 */
class Response {
public:
    Response(const std::string &response);

    virtual ~Response()
    {
    }

    void add(const ResponsePart &part);
    const virtual std::vector<ResponsePart> &getParts();
    operator std::string() const;
    Response *addref();
    void release();

private:
    int references;
    std::vector<ResponsePart> parts;
};


/**
 * A dynamically generated response.  This class allows the response
 * to be generated dynamically at the time of the conversation instead
 * of when the conversation data is loaded.
 */
class DynamicResponse:public Response {
public:
    DynamicResponse(
        Response *(*generator)(const DynamicResponse *),
        const std::string &param = ""
    );
    DynamicResponse(const DynamicResponse &) = delete;
    DynamicResponse(DynamicResponse &&) = delete;
    DynamicResponse &operator=(const DynamicResponse &) = delete;
    DynamicResponse &operator=(DynamicResponse &&) = delete;
    virtual ~DynamicResponse();
    const virtual std::vector<ResponsePart> &getParts();

    const std::string &getParam() const
    {
        return param;
    }

private:
    Response *(*generator)(const DynamicResponse *);
    Response *currentResponse;
    std::string param;
};


/**
 * The dialogue class, which holds conversation information for
 * townspeople and others who may talk to you.  It includes information
 * like pronouns, keywords, actual conversation text (of course),
 * questions, and what happens when you answer these questions.
 */
class Dialogue {
public:
    /**
     * A question-response to a keyword.
     */
    class Question {
    public:
        Question(const std::string &txt, Response *yes, Response *no);
        Question(const Question &) = delete;
        Question(Question &&) = delete;
        Question &operator=(const Question &) = delete;
        Question &operator=(Question &&) = delete;
        ~Question();
        std::string getText();
        Response *getResponse(bool yes);

    private:
        std::string text;
        Response *yesresp, *noresp;
    };


    /**
     * A dialogue keyword.
     * It contains all the keywords that the talker will respond to, as
     * well as the responses to those keywords.
     */
    class Keyword {
    public:
        Keyword(const std::string &kw, Response *resp);
        Keyword(const std::string &kw, const std::string &resp);
        Keyword(const Keyword &) = delete;
        Keyword(Keyword &&) = delete;
        Keyword &operator=(const Keyword &) = delete;
        Keyword &operator=(Keyword &&) = delete;
        ~Keyword();
        bool operator==(const std::string &kw) const;


        /*
         * Accessor methods
         */
        const std::string &getKeyword()
        {
            return keyword;
        }

        Response *getResponse()
        {
            return response;
        }

    private:
        std::string keyword;
        Response *response;
    };


    /**
     * A mapping of keywords to the Keyword object that represents them
     */
    typedef std::map<std::string, Keyword *> KeywordMap;


    /*
     * Constructors/Destructors
     */
    Dialogue();
    Dialogue(const Dialogue &) = delete;
    Dialogue(Dialogue &&) = delete;
    Dialogue &operator=(const Dialogue &) = delete;
    Dialogue &operator=(Dialogue &&) = delete;
    virtual ~Dialogue();


    /*
     * Accessor methods
     */
    const std::string &getName() const
    {
        return name;
    }

    const std::string &getPronoun() const
    {
        return pronoun;
    }

    const std::string &getPrompt() const
    {
        return prompt;
    }

    Response *getIntro(bool = false)
    {
        return intro;
    }

    Response *getLongIntro(bool = false)
    {
        return longIntro;
    }

    Response *getDefaultAnswer()
    {
        return defaultAnswer;
    }

    Dialogue::Question *getQuestion()
    {
        return question;
    }


    /*
     * Getters
     */
    void setName(const std::string &n)
    {
        name = n;
    }

    void setPronoun(const std::string &pn)
    {
        pronoun = pn;
    }

    void setPrompt(const std::string &prompt)
    {
        this->prompt = prompt;
    }

    void setIntro(Response *i)
    {
        intro = i;
    }

    void setLongIntro(Response *i)
    {
        longIntro = i;
    }

    void setDefaultAnswer(Response *a)
    {
        defaultAnswer = a;
    }

    void setTurnAwayProb(int prob)
    {
        turnAwayProb = prob;
        attackProb = prob - 0x40;
        if (attackProb < 0) attackProb = 0;
    }

    void setQuestion(Question *q)
    {
        question = q;
    }

    void addKeyword(const std::string &kw, Response *response);
    const ResponsePart &getAction() const;
    std::string dump(const std::string &arg);


    /*
     * Operators
     */
    Keyword *operator[](const std::string &kw);

private:
    std::string name;
    std::string pronoun;
    std::string prompt;
    Response *intro;
    Response *longIntro;
    Response *defaultAnswer;
    KeywordMap keywords;
    std::vector<Response *> responses;
    int turnAwayProb;
    int attackProb;
    Question *question;
};


/**
 * The conversation class, which handles the flow of text from the
 * player to the talker and vice-versa.  It is responsible for beginning
 * and termination conversations and handing state changes during.
 */
class Conversation {
public:
    /** Different states the conversation may be in */
    enum State {
        INTRO,  /**< The initial state of the conversation,
                   before anything is said */
        TALK, /**< The "default" state of the conversation */
        ASK, /**< The talker is asking the player a question */
        ASKYESNO, /**< The talker is asking the player
                     a yes/no question */
        VENDORQUESTION, /**< A vendor is asking the player
                           a question */
        BUY_ITEM, /**< Asked which item to buy */
        SELL_ITEM, /**< Asked which item to sell */
        BUY_QUANTITY, /**< Asked how many items to buy */
        SELL_QUANTITY, /**< Asked how many items to sell */
        BUY_PRICE, /**< Asked how much money to give someone */
        CONFIRMATION, /**< Asked by a vendor to confirm something */
        CONTINUEQUESTION, /**< Asked whether or not to continue */
        TOPIC, /**< Asked a topic to speak about */
        PLAYER, /**< Input for which player is required */
        FULLHEAL, /**< Heal the entire party before
                     continuing conversation */
        ADVANCELEVELS, /**< Check and advance the party's
                          levels before continuing */
        GIVEBEGGAR, /**< Asked how much to give a beggar */
        ATTACK, /**< The conversation ends with the talker
                   attacking you */
        DONE /**< The conversation is over */
    };

    /** Different types of conversation input required */
    enum InputType {
        INPUT_STRING,
        INPUT_CHARACTER,
        INPUT_NONE
    };

    /* Constructor/Destructors */
    Conversation();
    Conversation(const Conversation &) = delete;
    Conversation(Conversation &&) = delete;
    Conversation &operator=(const Conversation &) = delete;
    Conversation &operator=(Conversation &&) = delete;
    ~Conversation();
    /* Member functions */
    InputType getInputRequired(int *bufferLen);
    /* Static variables */
    static const unsigned int BUFFERLEN; /**< The default maxixum
                                            length of input */

public:
    State state; /**< The state of the conversation */
    std::string playerInput; /**< A std::string holding the text
                                the player inputs */
    std::list<std::string> reply; /**< What the talker says */
    class Script *script; /**< A script that this person
                             follows during the
                             conversation (may be nullptr) */
    Dialogue::Question *question; /**< The current question
                                     the player is being asked */
    int quant; /**< For vendor transactions */
    int player; /**< For vendor transactions */
    int price; /**< For vendor transactions */

private:
    Debug *logger;
};

#endif // ifndef CONVERSATION_H
