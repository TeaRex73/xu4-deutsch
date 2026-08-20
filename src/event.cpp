/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <list>
#include <string>

#include "event.h"

#include "context.h"
#include "controller.h"
#include "debug.h"
#include "direction.h"
#include "music.h"
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
        const Controller *controller = controllers.back();
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
void EventHandler::wait_msecs(const unsigned int msecs)
{
    const unsigned int msecs_per_cycle =
        1000 / settings.gameCyclesPerSecond;
    const unsigned int cycles = msecs / msecs_per_cycle;
    if (cycles > 0) {
        WaitController waitCtrl(cycles);
        getInstance()->pushController(&waitCtrl);
        WaitController::wait();
    }
    // Sleep the rest of the msecs we can't wait for
    sleep(msecs % msecs_per_cycle);
}

/**
 * Simulates the delay caused by loading stuff from floppy disk
 */
void EventHandler::simulateDiskLoad(
    const int duration, const bool reEnableMusic
)
{
    if (reEnableMusic) musicMgr->freeze(); else musicMgr->pause();
    soundStop();
    screenDisableCursor();
    screenHideCursor();
    screenMoving = false;
    wait_msecs(duration);
    screenMoving = true;
    screenEnableCursor();
    screenShowCursor();
    if (reEnableMusic) musicMgr->thaw();
}

/**
 * Waits a given number of game cycles before continuing
 */
void EventHandler::wait_cycles(const unsigned int cycles)
{
    WaitController waitCtrl(cycles);
    getInstance()->pushController(&waitCtrl);
    WaitController::wait();
}

void EventHandler::setControllerDone(const bool done)
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

Controller *EventHandler::pushController(Controller *controller)
{
    controllers.push_back(controller);
    getTimer()->add(
        &Controller::timerCallback, controller->getTimerInterval(), controller
    );
    return controller;
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

void EventHandler::setController(Controller *controller)
{
    while (!controllers.empty()) {
        const Controller *old_controller = controllers.back();
        popController();
        if (old_controller != controller) {
            delete old_controller;
        }
    }
    pushController(controller);
}

/* TimedEvent functions */
TimedEvent::TimedEvent(const Callback cb, const int i, void *d)
    :callback(cb), data(d), interval(i), current(0)
{
}

TimedEvent::Callback TimedEvent::getCallback() const
{
    return callback;
}

void *TimedEvent::getData() const
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
    const TimedEvent::Callback callback, const int interval, void *data
)
{
    events.push_back(new TimedEvent(callback, interval, data));
}


/**
 * Removes a timed event from the event queue.
 */
TimedEventMgr::List::iterator TimedEventMgr::remove(const List::iterator i)
{
    if (isLocked()) {
        deferredRemovals.push_back(*i);
        return i;
    }
    delete *i;
    return events.erase(i);
}

void TimedEventMgr::remove(const TimedEvent *event)
{
    const auto i = std::find_if(
        events.begin(),
        events.end(),
        [&](const TimedEvent *v) -> bool {
            return v == event;
        }
    );
    if (i != events.end()) {
        remove(i);
    }
}

void TimedEventMgr::remove(
    const TimedEvent::Callback callback, const void *data
)
{
    const auto i = std::find_if(
        events.begin(),
        events.end(),
        [&](const TimedEvent *v) -> bool {
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
    if (!mouseAreaSets.empty()) {
        mouseAreaSets.pop_front();
    }
}


/**
 * Get the currently active mouse area set off the top of the stack.
 */
MouseArea *EventHandler::getMouseAreaSet() const
{
    if (!mouseAreaSets.empty()) {
        return mouseAreaSets.front();
    }
    return nullptr;
}


/**
 * @param max_len the maximum length of the string
 * @param screenX the screen column where to begin input
 * @param screenY the screen row where to begin input
 * @param accepted_chars a string of characters to be accepted for input
 */
ReadStringController::ReadStringController(
    const int max_len,
    const int screenX,
    const int screenY,
    const std::string &accepted_chars
)

    :max_len(max_len),
     screenX(screenX),
     screenY(screenY),
     view(nullptr),
     accepted(accepted_chars)
{
}

ReadStringController::ReadStringController(
    const int max_len, TextView *view, const std::string &accepted_chars
)
    :max_len(max_len),
     screenX(view->getCursorX()),
     screenY(view->getCursorY()),
     view(view),
     accepted(accepted_chars)
{
}

bool ReadStringController::keyPressed(const int key)
{
    bool valid = true;
    const int len = static_cast<int>(value.length());
    std::size_t pos = std::string::npos;
    if (key < U4_ALT) {
        pos = accepted.find_first_of(static_cast<char>(key));
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
        } else if (key == '\n' || key == '\r') {
            doneWaiting();
        } else if (len < max_len) {
            /* add a character to the end */
            U4ASSERT(
                key <= std::numeric_limits<char>::max(),
                "Key too large to use as char: %d",
                key
            );
            value += static_cast<char>(key);
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
    const int max_len, const int screenX, const int screenY, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    ReadStringController ctrl(max_len, screenX, screenY);
    eh->pushController(&ctrl);
    return deumlaut(ctrl.waitFor());
}

std::string ReadStringController::getString(
    const int max_len, TextView *view, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    ReadStringController ctrl(max_len, view);
    eh->pushController(&ctrl);
    return deumlaut(ctrl.waitFor());
}

ReadIntController::ReadIntController(
    const int max_len, const int screenX, const int screenY
)
    :ReadStringController(max_len, screenX, screenY, "0123456789 \n\r\010")
{
}

int ReadIntController::getInt(
    const int max_len, const int screenX, const int screenY, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    ReadIntController ctrl(max_len, screenX, screenY);
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
    if (!choices.empty() && key > std::numeric_limits<char>::max()) {
        return false;
    }
    if (choices.empty()
        || choices.find_first_of(static_cast<char>(value))
            != std::string::npos) {
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
    return static_cast<char>(ctrl.waitFor());
}

ReadDirController::ReadDirController()
{
    value = DIR_NONE;
}

bool ReadDirController::keyPressed(const int key)
{
    const Direction d = keyToDirection(key);
    const bool valid = d != DIR_NONE;
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

WaitController::WaitController(const unsigned int cyc)
    : cycles(cyc), current(0)
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

void WaitController::setCycles(const int cyc)
{
    cycles = cyc;
}
