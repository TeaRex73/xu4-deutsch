/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <SDL.h>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include "u4.h"

#include "event.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"
#include "u4_sdl.h"
#include "utils.h"

extern bool verbose;
extern int quit;

KeyHandler::KeyHandler(Callback func, void *d, bool asyncronous)
    :handler(func), async(asyncronous), data(d)
{
}


/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int KeyHandler::setKeyRepeat(int delay, int interval)
{
    return SDL_EnableKeyRepeat(delay, interval);
}


/**
 * Handles any and all keystrokes.
 * Generally used to exit the application, switch applications,
 * minimize, maximize, etc.
 */
bool KeyHandler::globalHandler(int key)
{
    switch (key) {
    case U4_META + 'q':
    case U4_META + 'x':
    case U4_ALT + 'x':
    case U4_ALT + U4_FKEY + 3:
        quit = 2;
        EventHandler::end();
        return true;
    default:
        return false;
    }
}


/**
 * A default key handler that should be valid everywhere
 */
bool KeyHandler::defaultHandler(int key, void *)
{
    bool valid = true;
    switch (key) {
    case '`':
        if (c && c->location) {
            std::printf(
                "x = %d, y = %d, level = %d, tile = %d (%s)\n",
                c->location->coords.x,
                c->location->coords.y,
                c->location->coords.z,
                c->location->map->ttrti(
                    *c->location->map->tileAt(
                        c->location->coords, WITH_OBJECTS
                    )
                ),
                c->location->map->tileTypeAt(
                    c->location->coords, WITH_OBJECTS
                )->getName().c_str()
            );
        }
        break;
    default:
        valid = false;
        break;
    }
    return valid;
}


/**
 * A key handler that ignores keypresses
 */
bool KeyHandler::ignoreKeys(int, void *)
{
    return true;
}


/**
 * Handles a keypress.
 * First it makes sure the key combination is not ignored
 * by the current key handler. Then, it passes the keypress
 * through the global key handler. If the global handler
 * does not process the keystroke, then the key handler
 * handles it itself by calling its handler callback function.
 */
bool KeyHandler::handle(int key)
{
    bool processed = false;
    if (!isKeyIgnored(key)) {
        processed = globalHandler(key);
        if (!processed) {
            processed = handler(key, data);
        }
    }
    return processed;
}


/**
 * Returns true if the key or key combination is always ignored by xu4
 */
bool KeyHandler::isKeyIgnored(int key)
{
    switch (key) {
    case U4_RIGHT_SHIFT:
    case U4_LEFT_SHIFT:
    case U4_RIGHT_CTRL:
    case U4_LEFT_CTRL:
    case U4_RIGHT_ALT:
    case U4_LEFT_ALT:
    case U4_RIGHT_META:
    case U4_LEFT_META:
    case U4_TAB:
        return true;
    default:
        return false;
    }
}

bool KeyHandler::operator==(Callback cb) const
{
    return (handler == cb) ? true : false;
}

KeyHandlerController::KeyHandlerController(KeyHandler *handler)
    :handler(handler)
{
}

KeyHandlerController::~KeyHandlerController()
{
    delete handler;
}

bool KeyHandlerController::keyPressed(int key)
{
    ASSERT(handler != nullptr, "key handler must be initialized");
    return handler->handle(key);
}

KeyHandler *KeyHandlerController::getKeyHandler()
{
    return handler;
}


/**
 * Constructs a timed event manager object.
 * Adds a timer callback to the SDL subsystem, which
 * will drive all of the timed events that this object
 * controls.
 */
TimedEventMgr::TimedEventMgr(int i)
    :id(nullptr),
     baseInterval(i),
     locked(false),
     events(),
     deferredRemovals()
{
    /* start the SDL timer */
    if (instances == 0) {
        if (u4_SDL_InitSubSystem(SDL_INIT_TIMER) < 0) {
            errorFatal("unable to init SDL: %s", SDL_GetError());
        }
    }
    id = static_cast<void *>(SDL_AddTimer(i, &TimedEventMgr::callback, this));
    instances++;
}


/**
 * Destructs a timed event manager object.
 * It removes the callback timer and un-initializes the
 * SDL subsystem if there are no other active TimedEventMgr
 * objects.
 */
TimedEventMgr::~TimedEventMgr()
{
    SDL_RemoveTimer(static_cast<SDL_TimerID>(id));
    id = nullptr;
    if (instances == 1) {
        u4_SDL_QuitSubSystem(SDL_INIT_TIMER);
    }
    if (instances > 0) {
        instances--;
    }
    for (List::iterator i = deferredRemovals.begin();
         i != deferredRemovals.end();
         /* nothing */) {
        List::iterator tmp;
        tmp = i;
        ++tmp;
        delete (*i);
        i = tmp;
    }
    deferredRemovals.clear();
}

/**
 * Adds an SDL timer event to the message queue.
 */
unsigned int TimedEventMgr::callback(unsigned int interval, void *param)
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = param;
    event.user.data2 = nullptr;
    SDL_PushEvent(&event);
    return interval;
}


/**
 * Re-initializes the timer manager to a new timer granularity
 */
void TimedEventMgr::reset(unsigned int interval)
{
    baseInterval = interval;
    stop();
    start();
}

void TimedEventMgr::stop()
{
    if (id) {
        SDL_RemoveTimer(static_cast<SDL_TimerID>(id));
        id = nullptr;
    }
}

void TimedEventMgr::start()
{
    if (!id) {
        id = static_cast<void *>(
            SDL_AddTimer(baseInterval, &TimedEventMgr::callback, this)
        );
    }
}


/**
 * Constructs an event handler object.
 */
EventHandler::EventHandler()
    :timer(eventTimerGranularity),
     controllers(),
     mouseAreaSets(),
     updateScreen(nullptr)
{
}

static void handleMouseMotionEvent(const SDL_Event &event)
{
    if (!settings.mouseOptions.enabled) {
        return;
    }
    MouseArea *area;
    area = eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
    if (area) {
        screenSetMouseCursor(area->cursor);
    } else {
        screenSetMouseCursor(MC_DEFAULT);
    }
}

static void handleActiveEvent(
    const SDL_Event &event, updateScreenCallback updateScreen
)
{
    if (event.active.state & SDL_APPACTIVE) {
        // application was previously iconified and
        // is now being restored
        if (event.active.gain) {
            if (updateScreen) {
                (*updateScreen)();
            }
            screenRedrawScreen();
        }
    }
}

static void handleMouseButtonDownEvent(
    const SDL_Event &event,
    Controller *controller,
    updateScreenCallback updateScreen
)
{
    int button = event.button.button - 1;
    if (!settings.mouseOptions.enabled) {
        return;
    }
    if (button > 2) {
        button = 0;
    }
    MouseArea *area =
        eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
    if (!area || (area->command[button] == 0)) {
        return;
    }
    controller->keyPressed(area->command[button]);
    if (updateScreen) {
        (*updateScreen)();
    }
    screenRedrawScreen();
}

static void handleKeyDownEvent(
    const SDL_Event &event,
    Controller *controller,
    updateScreenCallback updateScreen
)
{
    int processed;
    int key;
    if (event.key.keysym.unicode <= 0xff) {
        key = event.key.keysym.unicode;
        if (key > 0x7f) {
            switch (key) {
            case 0xe4:
                key = '{';
                break;
            case 0xc4:
                key = '[';
                break;
            case 0xf6:
                key = '|';
                break;
            case 0xd6:
                key = '\\';
                break;
            case 0xfc:
                key = '}';
                break;
            case 0xdc:
                key = ']';
                break;
            case 0xdf:
                key = '~';
                break;
            case 0xa7:
                key = '@';
                break;
            default:
                key = 0;
            }
        }
    } else {
        key = event.key.keysym.sym;
    }
    if (event.key.keysym.mod & KMOD_ALT) {
        key = U4_ALT + event.key.keysym.sym;
        if (event.key.keysym.mod & KMOD_META) {
            key += U4_META;
        }
    }
    if (event.key.keysym.sym == SDLK_UP) {
        key = U4_UP;
    } else if (event.key.keysym.sym == SDLK_DOWN) {
        key = U4_DOWN;
    } else if (event.key.keysym.sym == SDLK_LEFT) {
        key = U4_LEFT;
    } else if (event.key.keysym.sym == SDLK_RIGHT) {
        key = U4_RIGHT;
    } else if ((event.key.keysym.sym == SDLK_BACKSPACE)
               || (event.key.keysym.sym == SDLK_DELETE)) {
        key = U4_BACKSPACE;
    }
    if ((event.key.keysym.sym >= SDLK_F1)
        && (event.key.keysym.sym <= SDLK_F15)) {
        key = U4_FKEY + (event.key.keysym.sym - SDLK_F1);
    }
    if (verbose) {
        std::printf(
            "key event: unicode = %d, sym = %d, mod = %d; translated = %d\n",
            event.key.keysym.unicode,
            event.key.keysym.sym,
            event.key.keysym.mod,
            key
        );
    }
    /* handle the keypress */
    if ((key >= 'a') && (key <= '}')) {
        key = xu4_toupper(key);
    }
    processed = controller->notifyKeyPressed(key);
    if (processed) {
        if (updateScreen) {
            (*updateScreen)();
        }
        screenRedrawScreen();
    }
} // handleKeyDownEvent

static Uint32 sleepTimerCallback(Uint32, void *)
{
    SDL_Event stopEvent;
    stopEvent.type = SDL_USEREVENT;
    stopEvent.user.code = 1;
    stopEvent.user.data1 = 0;
    stopEvent.user.data2 = 0;
    SDL_PushEvent(&stopEvent);
    return 0;
}

/**
 * Delays program execution for the specified number of milliseconds.
 * This doesn't actually stop events, but it stops the user from interacting
 * while some important event happens (e.g., getting hit by a cannon ball
 * or a spell effect).
 */
void EventHandler::sleep(unsigned int usec)
{
    // Start a timer for the amount of time we want
    // to sleep from user input.
    // Make this static so that all instance stop.
    // (e.g., sleep calling sleep).
    static std::atomic_bool stopUserInput(true);
    SDL_TimerID sleepingTimer = SDL_AddTimer(usec, sleepTimerCallback, 0);
    stopUserInput = true;
    while (stopUserInput) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            // Discard the event.
            break;
        case SDL_MOUSEMOTION:
            handleMouseMotionEvent(event);
            break;
        case SDL_ACTIVEEVENT:
            handleActiveEvent(event, eventHandler->updateScreen);
            break;
        case SDL_USEREVENT:
            if (event.user.code == 0) {
                eventHandler->getTimer()->tick();
            } else if (event.user.code == 1) {
                SDL_RemoveTimer(sleepingTimer);
                stopUserInput = false;
            }
            break;
        case SDL_QUIT:
            std::exit(EXIT_FAILURE);
            break;
        default:
            break;
        }
    }
} // EventHandler::sleep

void EventHandler::run()
{
    if (updateScreen) {
        (*updateScreen)();
    }
    screenRedrawScreen();
    while (!ended && !controllerDone) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_KEYDOWN:
            {
#ifdef DEBUG
                static std::atomic<std::clock_t> clocksum(0);
                                static std::atomic_int keycount(0);
                std::clock_t oldc, newc, diff;
                oldc = std::clock();
#endif
                handleKeyDownEvent(event, getController(), updateScreen);
#ifdef DEBUG
                newc = std::clock();
                keycount++;
                diff = newc - oldc;
                clocksum += diff;
                std::fprintf(
                    stderr,
                    "diff = %ld, sum = %ld, avg = %f\n",
                    static_cast<long int>(diff),
                    static_cast<long int>(static_cast<std::clock_t>(clocksum)),
                    (static_cast<double>(static_cast<std::clock_t>(clocksum))
                     / static_cast<double>(static_cast<int>(keycount)))
                );
#endif
                break;
            }
        case SDL_MOUSEBUTTONDOWN:
            handleMouseButtonDownEvent(event, getController(), updateScreen);
            break;
        case SDL_MOUSEMOTION:
            handleMouseMotionEvent(event);
            break;
        case SDL_USEREVENT:
            eventHandler->getTimer()->tick();
            break;
        case SDL_ACTIVEEVENT:
            handleActiveEvent(event, updateScreen);
            break;
        case SDL_QUIT:
            std::exit(EXIT_FAILURE);
            break;
        default:
            break;
        }
    }
} // EventHandler::run

void EventHandler::setScreenUpdate(void (*updateScreen)(void))
{
    this->updateScreen = updateScreen;
}


/**
 * Returns true if the queue is empty of events that match 'mask'.
 */
bool EventHandler::timerQueueEmpty()
{
    SDL_Event event;
    if (SDL_PeepEvents(
            &event, 1, SDL_PEEKEVENT, SDL_EVENTMASK(SDL_USEREVENT)
        )) {
        return false;
    } else {
        return true;
    }
}

/**
 * Adds a key handler to the stack.
 */
void EventHandler::pushKeyHandler(KeyHandler kh)
{
    KeyHandler *new_kh = new KeyHandler(std::move(kh));
    KeyHandlerController *khc = new KeyHandlerController(new_kh);
    pushController(khc);
}

/**
 * Pops a key handler off the stack.
 * Returns a pointer to the resulting key handler after
 * the current handler is popped.
 */
void EventHandler::popKeyHandler()
{
    if (controllers.empty()) {
        return;
    }
    KeyHandlerController *khc =
        dynamic_cast<KeyHandlerController *>(controllers.back());
    if (khc == nullptr) {
        return;
    }
    Controller *oc = controllers.back();
    popController();
    delete oc;
}


/**
 * Returns a pointer to the current key handler.
 * Returns nullptr if there is no key handler.
 */
KeyHandler *EventHandler::getKeyHandler() const
{
    if (controllers.empty()) {
        return nullptr;
    }
    KeyHandlerController *khc =
        dynamic_cast<KeyHandlerController *>(controllers.back());
    ASSERT(
        khc != nullptr,
        "EventHandler::getKeyHandler called when controller wasn't a "
        "keyhandler"
    );
    if (khc == nullptr) {
        return nullptr;
    }
    return khc->getKeyHandler();
}


/**
 * Eliminates all key handlers and begins stack with new handler.
 * This pops all key handlers off the stack and adds
 * the key handler provided to the stack, making it the
 * only key handler left. Use this function only if you
 * are sure the key handlers in the stack are disposable.
 */
void EventHandler::setKeyHandler(KeyHandler kh)
{
    while (popController() != nullptr) {}
    pushKeyHandler(std::move(kh));
}

MouseArea *EventHandler::mouseAreaForPoint(int x, int y)
{
    int i;
    MouseArea *areas = getMouseAreaSet();
    if (!areas) {
        return nullptr;
    }
    for (i = 0; areas[i].npoints != 0; i++) {
        if (screenPointInMouseArea(x, y, &(areas[i]))) {
            return &(areas[i]);
        }
    }
    return nullptr;
}
