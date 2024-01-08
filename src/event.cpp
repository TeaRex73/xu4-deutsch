/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <cctype>
#include <list>

#include "event.h"

#include "context.h"
#include "debug.h"
#include "location.h"
#include "music.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "textview.h"
#include "utils.h"

std::atomic_int eventTimerGranularity(250);
extern int quit;
bool EventHandler::controllerDone = false;
bool EventHandler::ended = false;
unsigned int TimedEventMgr::instances = 0;
EventHandler *EventHandler::instance = nullptr;

EventHandler::~EventHandler()
{
    while (!controllers.empty()) {
        Controller *controller = controllers.back();
        popController();
        delete controller;
    }
}

EventHandler *EventHandler::getInstance()
{
    if (instance == nullptr) {
        instance = new EventHandler();
    }
    return instance;
}

/**
 * Waits a given number of milliseconds before continuing
 */
void EventHandler::wait_msecs(unsigned int msecs)
{
    int msecs_per_cycle = (1000 / settings.gameCyclesPerSecond);
    int cycles = msecs / msecs_per_cycle;
    if (cycles > 0) {
        WaitController waitCtrl(cycles);
        getInstance()->pushController(&waitCtrl);
        waitCtrl.wait();
    }
    // Sleep the rest of the msecs we can't wait for
    EventHandler::sleep(msecs % msecs_per_cycle);
}

/**
 * Simulates the delay caused by loading stuff from flopp disk
 */
void EventHandler::simulateDiskLoad(int duration, bool reenableMusic)
{
    if (reenableMusic) musicMgr->freeze(); else musicMgr->pause();
    soundStop();
    screenDisableCursor();
    screenHideCursor();
    screenMoving = false;
    wait_msecs(duration);
    screenMoving = true;
    screenEnableCursor();
    screenShowCursor();
    if (reenableMusic) musicMgr->thaw();
}

/**
 * Waits a given number of game cycles before continuing
 */
void EventHandler::wait_cycles(unsigned int cycles)
{
    WaitController waitCtrl(cycles);
    getInstance()->pushController(&waitCtrl);
    waitCtrl.wait();
}

void EventHandler::setControllerDone(bool done)
{
    controllerDone = done;
}   /**< Sets the controller exit flag for the event handler */

bool EventHandler::getControllerDone()
{
    return controllerDone; /**< Returns the current value of
                              the global exit flag */
}

void EventHandler::end()
{
    ended = true; /**< End all event processing */
}

TimedEventMgr *EventHandler::getTimer()
{
    return &timer;
}

Controller *EventHandler::pushController(Controller *c)
{
    controllers.push_back(c);
    getTimer()->add(&Controller::timerCallback, c->getTimerInterval(), c);
    return c;
}

Controller *EventHandler::popController()
{
    if (controllers.empty()) {
        return nullptr;
    }
    const Controller *controller = controllers.back();
    getTimer()->remove(&Controller::timerCallback, controller);
    controllers.pop_back();
    return getController();
}

Controller *EventHandler::getController() const
{
    if (controllers.empty()) {
        return nullptr;
    }
    return controllers.back();
}

void EventHandler::setController(Controller *c)
{
    while (!controllers.empty()) {
        Controller *oc = controllers.back();
        popController();
        if (oc != c) {
            delete oc;
        }
    }
    pushController(c);
}

/* TimedEvent functions */
TimedEvent::TimedEvent(TimedEvent::Callback cb, int i, void *d)
    :callback(cb), data(d), interval(i), current(0)
{
}

TimedEvent::Callback TimedEvent::getCallback() const
{
    return callback;
}

void *TimedEvent::getData()
{
    return data;
}


/**
 * Advances the timed event forward a tick.
 * When (current >= interval), then it executes its callback function.
 */
void TimedEvent::tick()
{
    if (++current >= interval) {
        (*callback)(data);
        current = 0;
    }
}


/**
 * Returns true if the event queue is locked
 */
bool TimedEventMgr::isLocked() const
{
    return locked;
}


/**
 * Adds a timed event to the event queue.
 */
void TimedEventMgr::add(
    TimedEvent::Callback callback, int interval, void *data
)
{
    events.push_back(new TimedEvent(callback, interval, data));
}


/**
 * Removes a timed event from the event queue.
 */
TimedEventMgr::List::iterator TimedEventMgr::remove(List::iterator i)
{
    if (isLocked()) {
        deferredRemovals.push_back(*i);
        return i;
    } else {
        delete *i;
        return events.erase(i);
    }
}

void TimedEventMgr::remove(const TimedEvent *event)
{
    List::iterator i = std::find_if(
        events.begin(),
        events.end(),
        [&](const TimedEvent *v) {
            return v == event;
        }
    );
    if (i != events.end()) {
        remove(i);
    }
}

void TimedEventMgr::remove(TimedEvent::Callback callback, const void *data)
{
    List::iterator i = std::find_if(
        events.begin(),
        events.end(),
        [&](TimedEvent *v) {
            return v->getCallback() == callback && v->getData() == data;
        }
    );
    if (i != events.end()) {
        remove(i);
    }
}


/**
 * Runs each of the callback functions of the TimedEvents associated
 * with this manager.
 */
void TimedEventMgr::tick()
{
    List::iterator i;
    lock();
    for (i = events.begin(); i != events.end(); ++i) {
        (*i)->tick();
    }
    unlock();
    // Remove events that have been deferred for removal
    for (i = deferredRemovals.begin(); i != deferredRemovals.end(); ++i) {
        events.remove(*i);
    }
}

void TimedEventMgr::lock()
{
    locked = true;
}

void TimedEventMgr::unlock()
{
    locked = false;
}

void EventHandler::pushMouseAreaSet(MouseArea *mouseAreas)
{
    mouseAreaSets.push_front(mouseAreas);
}

void EventHandler::popMouseAreaSet()
{
    if (mouseAreaSets.size()) {
        mouseAreaSets.pop_front();
    }
}


/**
 * Get the currently active mouse area set off the top of the stack.
 */
MouseArea *EventHandler::getMouseAreaSet() const
{
    if (mouseAreaSets.size()) {
        return mouseAreaSets.front();
    } else {
        return nullptr;
    }
}


/**
 * @param maxlen the maximum length of the string
 * @param screenX the screen column where to begin input
 * @param screenY the screen row where to begin input
 * @param accepted_chars a string of characters to be accepted for input
 */
ReadStringController::ReadStringController(
    int maxlen, int screenX, int screenY, const std::string &accepted_chars
)

    :maxlen(maxlen),
     screenX(screenX),
     screenY(screenY),
     view(nullptr),
     accepted(accepted_chars)
{
}

ReadStringController::ReadStringController(
    int maxlen, TextView *view, const std::string &accepted_chars
)
    :maxlen(maxlen),
     screenX(view->getCursorX()),
     screenY(view->getCursorY()),
     view(view),
     accepted(accepted_chars)
{
}

bool ReadStringController::keyPressed(int key)
{
    int valid = true, len = value.length();
    std::size_t pos = std::string::npos;
    if (key < U4_ALT) {
        pos = accepted.find_first_of(key);
    }
    if (pos != std::string::npos) {
        if (key == U4_BACKSPACE) {
            if (len > 0) {
                /* remove the last character */
                value.erase(len - 1, 1);
                if (view) {
                    view->textAt(screenX + len - 1, screenY, " ");
                    view->setCursorPos(screenX + len - 1, screenY, true);
                } else {
                    screenHideCursor();
                    screenTextAt(screenX + len - 1, screenY, " ");
                    screenSetCursorPos(screenX + len - 1, screenY);
                    screenShowCursor();
                }
            }
        } else if ((key == '\n') || (key == '\r')) {
            doneWaiting();
        } else if (len < maxlen) {
            /* add a character to the end */
            value += key;
            if (view) {
                view->textAt(screenX + len, screenY, "%c", key);
            } else {
                screenHideCursor();
                screenTextAt(screenX + len, screenY, "%c", key);
                screenSetCursorPos(screenX + len + 1, screenY);
                c->col = len + 1;
                screenShowCursor();
            }
        }
    } else {
        valid = false;
    }
    return valid || KeyHandler::defaultHandler(key, nullptr);
} // ReadStringController::keyPressed

std::string ReadStringController::getString(
    int maxlen, int screenX, int screenY, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    ReadStringController ctrl(maxlen, screenX, screenY);
    eh->pushController(&ctrl);
    return deumlaut(ctrl.waitFor());
}

std::string ReadStringController::getString(
    int maxlen, TextView *view, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    ReadStringController ctrl(maxlen, view);
    eh->pushController(&ctrl);
    return deumlaut(ctrl.waitFor());
}

ReadIntController::ReadIntController(int maxlen, int screenX, int screenY)
    :ReadStringController(maxlen, screenX, screenY, "0123456789 \n\r\010")
{
}

int ReadIntController::getInt(
    int maxlen, int screenX, int screenY, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    ReadIntController ctrl(maxlen, screenX, screenY);
    eh->pushController(&ctrl);
    ctrl.waitFor();
    return ctrl.stringToInt();
}

int ReadIntController::stringToInt() const
{
    return static_cast<int>(std::strtol(value.c_str(), nullptr, 10));
}

ReadChoiceController::ReadChoiceController(const std::string &choices)
    :choices(choices)
{
}

bool ReadChoiceController::keyPressed(int key)
{
    key = xu4_tolower(key);
    value = key;
    if (choices.empty() || (choices.find_first_of(value) < choices.length())) {
        // If the value is printable, display it
        if (!choices.empty() && std::isgraph(key)) {
            screenMessage("%c", xu4_toupper(key));
        } else {
            screenMessage("%c", ' ');
        }
        doneWaiting();
        return true;
    }
    return false;
}

char ReadChoiceController::getChar(
    const std::string &choices, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    ReadChoiceController ctrl(choices);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

ReadDirController::ReadDirController()
{
    value = DIR_NONE;
}

bool ReadDirController::keyPressed(int key)
{
    Direction d = keyToDirection(key);
    bool valid = (d != DIR_NONE);
    switch (key) {
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        value = DIR_NONE;
        doneWaiting();
        return true;
    default:
        if (valid) {
            value = d;
            doneWaiting();
            return true;
        }
        break;
    }
    return false;
}

WaitController::WaitController(unsigned int c)
    :Controller(), cycles(c), current(0)
{
}

void WaitController::timerFired()
{
    if (++current >= cycles) {
        current = 0;
        eventHandler->setControllerDone(true);
    }
}

bool WaitController::keyPressed(int)
{
    return true;
}

void WaitController::wait()
{
    Controller_startWait();
}

void WaitController::setCycles(int c)
{
    cycles = c;
}
