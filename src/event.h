/*
 * $Id$
 */

#ifndef EVENT_H
#define EVENT_H

#include <atomic>
#include <list>
#include <string>
#include <vector>

#include "controller.h"
#include "direction.h"
#include "screen.h"


#define eventHandler (EventHandler::getInstance())

#define U4_UP '@'
#define U4_DOWN '/'
#define U4_LEFT ';'
#define U4_RIGHT '\''
#define U4_BACKSPACE 8
#define U4_TAB 9
#define U4_SPACE ' '
#define U4_ESC 27
#define U4_ENTER 13
#define U4_ALT 128
#define U4_KEYPAD_ENTER 271
#define U4_META 323
#define U4_F_KEY 282
#define U4_RIGHT_SHIFT 303
#define U4_LEFT_SHIFT 304
#define U4_RIGHT_CTRL 305
#define U4_LEFT_CTRL 306
#define U4_RIGHT_ALT 307
#define U4_LEFT_ALT 308
#define U4_RIGHT_META 309
#define U4_LEFT_META 310

extern std::atomic_int eventTimerGranularity;
class EventHandler;
class TextView;


/**
 * A class for handling keystrokes.
 */
class KeyHandler {
public:
    KeyHandler(KeyHandler &&) = default;
    KeyHandler(const KeyHandler &) = default;
    KeyHandler &operator=(KeyHandler &&) = default;
    KeyHandler &operator=(const KeyHandler &) = default;

    virtual ~KeyHandler() = default;

    typedef bool (*Callback)(int, void *);

    typedef struct ReadBuffer {
        int (*handleBuffer)(std::string *);
        std::string *buffer;
        int bufferLen;
        int screenX, screenY;
    } ReadBuffer;

    typedef struct GetChoice {
        std::string choices;
        int (*handleChoice)(int);
    } GetChoice;

    // cppcheck-suppress noExplicitConstructor // implicit intended
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    KeyHandler(
        Callback func, void *d = nullptr, bool asynchronous = true
    );
    static int setKeyRepeat(int delay, int interval);
    static bool globalHandler(int key);
    static bool defaultHandler(int key, void *data);
    static bool ignoreKeys(int key, void *data);
    bool operator==(Callback cb) const;
    bool handle(int key) const;
    virtual bool isKeyIgnored(int key) const;

protected:
    Callback handler;
    bool async;
    void *data;
};


/**
 * A controller that wraps a key handler function.  Key handlers are
 * deprecated -- please use a controller instead.
 */
class KeyHandlerController:public Controller {
public:
    explicit KeyHandlerController(const KeyHandler *handler);
    KeyHandlerController(const KeyHandlerController &) = delete;
    KeyHandlerController(KeyHandlerController &&) = delete;
    KeyHandlerController &operator=(const KeyHandlerController &) = delete;
    KeyHandlerController &operator=(KeyHandlerController &&) = delete;
    ~KeyHandlerController() override;

    bool keyPressed(int key) override;
    const KeyHandler *getKeyHandler() const;

private:
    const KeyHandler *handler;
};


/**
 * A controller to read a string, terminated by the enter key.
 */
class ReadStringController:public WaitableController<std::string> {
public:
    ReadStringController(
        int max_len,
        int screenX, int
        screenY,
        const std::string &accepted_chars =
        "abcdefghijklmnopqrstuvwxyz{|}~"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]"
        "1234567890 \n\r\010"
    );
    ReadStringController(
        int max_len,
        TextView *view,
        const std::string &accepted_chars =
        "abcdefghijklmnopqrstuvwxyz{|}~"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]"
        "1234567890 \n\r\010"
    );
    ReadStringController(const ReadStringController &) = delete;
    ReadStringController(ReadStringController &&) = delete;
    ReadStringController &operator=(const ReadStringController &) = delete;
    ReadStringController &operator=(ReadStringController &&) = delete;

    bool keyPressed(int key) override;
    static std::string getString(
        int max_len, int screenX, int screenY, EventHandler *eh = nullptr
    );
    static std::string getString(
        int max_len, TextView *view, EventHandler *eh = nullptr
    );

protected:
    int max_len, screenX, screenY;
    TextView *view;
    std::string accepted;
};


/**
 * A controller to read a integer, terminated by the enter key.
 * Non-numeric keys are ignored.
 */
class ReadIntController:public ReadStringController {
public:
    ReadIntController(int max_len, int screenX, int screenY);
    static int getInt(
        int max_len, int screenX, int screenY, EventHandler *eh = nullptr
    );
    int stringToInt() const;
};


/**
 * A controller to read a single key from a provided list.
 */
class ReadChoiceController:public WaitableController<int> {
public:
    explicit ReadChoiceController(const std::string &choices);
    bool keyPressed(int key) override;
    static char getChar(
        const std::string &choices, EventHandler *eh = nullptr
    );

protected:
    std::string choices;
};


/**
 * A controller to read a direction enter with the arrow keys.
 */
class ReadDirController:public WaitableController<Direction> {
public:
    ReadDirController();
    bool keyPressed(int key) override;
};


/**
 * A controller to pause for a given length of time, ignoring all
 * keyboard input.
 */
class WaitController:public Controller {
public:
    explicit WaitController(unsigned int cyc);
    bool keyPressed(int key) override;
    void timerFired() override;
    static void wait();
    void setCycles(int cyc);

private:
    unsigned int cycles;
    unsigned int current;
};


/**
 * A class for handling timed events.
 */
class TimedEvent {
public:
    typedef std::list<TimedEvent *> List;
    typedef void (*Callback)(void *);

    TimedEvent(Callback cb, int i, void *d = nullptr);
    Callback getCallback() const;
    void *getData() const;
    void tick();

protected:
    Callback callback;
    void *data;
    int interval;
    int current;
};


/**
 * A class for managing timed events
 */
class TimedEventMgr {
public:
    typedef TimedEvent::List List;

    explicit TimedEventMgr(int i);
    TimedEventMgr(const TimedEventMgr &) = delete;
    TimedEventMgr(TimedEventMgr &&) = delete;
    TimedEventMgr &operator=(const TimedEventMgr &) = delete;
    TimedEventMgr &operator=(TimedEventMgr &&) = delete;
    ~TimedEventMgr();
    static unsigned int callback(unsigned int interval, void *param);
    bool isLocked() const; /**< Returns true if event list is in use */
    void add(
        TimedEvent::Callback callback, int interval, void *data = nullptr
    );
    List::iterator remove(List::iterator i);
    void remove(const TimedEvent *event);
    void remove(TimedEvent::Callback callback, const void *data = nullptr);
    void tick();
    void stop();
    void start();
    void reset(int interval); /**< sets new base interval */

protected:
    static unsigned int instances;
    void *id;
    int baseInterval;
    bool locked;
    List events;
    List deferredRemovals;

private:
    void lock(); /**< Locks the event list */
    void unlock(); /**< Unlocks the event list */
};

typedef void (*updateScreenCallback)();


/**
 * A class for handling game events.
 */
class EventHandler {
public:
    typedef std::list<MouseArea *> MouseAreaList;

    EventHandler();
    ~EventHandler();
    static EventHandler *getInstance();
    static void sleep(unsigned int msec);
    static void wait_msecs(unsigned int msecs);
    static void simulateDiskLoad(int duration, bool reEnableMusic = true);
    static void wait_cycles(unsigned int cycles);
    static void setControllerDone(bool done = true);
    static bool getControllerDone();
    static void end();
    static bool timerQueueEmpty();
    TimedEventMgr *getTimer();
    void run();
    void setScreenUpdate(void (*update_screen)());
    Controller *pushController(Controller *controller);
    Controller *popController();
    Controller *getController() const;
    void setController(Controller *controller);
    void pushKeyHandler(const KeyHandler &kh);
    void popKeyHandler();
    const KeyHandler *getKeyHandler() const;
    void setKeyHandler(KeyHandler &kh);
    void pushMouseAreaSet(MouseArea *mouseAreas);
    void popMouseAreaSet();
    MouseArea *getMouseAreaSet() const;
    MouseArea *mouseAreaForPoint(int x, int y) const;

protected:
    static bool controllerDone;
    static bool ended;
    TimedEventMgr timer;
    std::vector<Controller *> controllers;
    MouseAreaList mouseAreaSets;
    updateScreenCallback updateScreen;

private:
    static EventHandler *instance;
};

#endif // EVENT_H
