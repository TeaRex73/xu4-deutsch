/*
 * $Id$
 */

#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <vector>
#include <SDL_mixer.h>

#include "debug.h"

#define musicMgr (Music::getInstance())

#define CAMP_FADE_OUT_TIME 1000
#define CAMP_FADE_IN_TIME 0
#define INN_FADE_OUT_TIME 1000
#define INN_FADE_IN_TIME 5000
#define NLOOPS 1

class Music {
public:
    enum Type {
        NONE,
        TOWNS,
        SHOPPING,
        DUNGEON,
        CASTLES,
        RULEBRIT,
        OUTSIDE,
        COMBAT,
        SHRINES,
        EXTRA,
        MAX
    };

    Music();
    Music(const Music &) = delete;
    Music(Music &&) = delete;
    Music &operator=(const Music &) = delete;
    Music &operator=(Music &&) = delete;
    ~Music();

    /** Returns an instance of the Music class */
    static Music *getInstance()
    {
        if (__builtin_expect(!instance, false)) {
            instance = new Music();
        }
        return instance;
    }

    /** Returns true if the mixer is playing any audio. */
    static bool isPlaying()
    {
        return getInstance()->isPlaying_sys();
    }

    static void callback(void *);

    static void init()
    {
    }

    void playCurrent();
    void play();

    static void freeze();
    static void thaw();

    void pause()
    {
        introMid = NONE;
        current = NONE;
        stopMid();
    }

    static void stop()
    {
        on = false;    /**< Stop playing music */
        stopMid();
    }

    static void fadeOut(int msecs);
    void fadeIn(int msecs, bool loadFromMap);

    void lordBritish()
    {
        playMid(RULEBRIT);    /**< Music when talk to L British */
    }

    void hawkwind()
    {
        playMid(SHOPPING);    /**< Music when talk to Hawkwind */
    }

    void shopping()
    {
        playMid(SHOPPING);    /**< Music when talki to vendor */
    }

    void gem()
    {
        playMid(SHRINES); /**< Music when peering */
    }

    void create_or_win()
    {
        playMid(EXTRA); /**< Music for starting game or game won */
    }

    void intro()
    {
        if (!introMid) {
            introMid = TOWNS;
        }
        playMid(introMid);
    }     /**< Play the introduction music on title loadup */

    void introSwitch(int n);
    bool toggle();
    static int decreaseMusicVolume();
    static int increaseMusicVolume();

    static void setMusicVolume(int volume)
    {
        setMusicVolume_sys(volume);
    }

    static int decreaseSoundVolume();
    static int increaseSoundVolume();

    static void setSoundVolume(int volume)
    {
        setSoundVolume_sys(volume);
    }

    static bool functional;
    std::vector<std::string> filenames;
    Type introMid;
    Type current;
    Mix_Music *playing;
    Debug *logger;

private:
    void create_sys();
    void destroy_sys();
    static void setMusicVolume_sys(int volume);
    static void setSoundVolume_sys(int volume);
    static void fadeOut_sys(int msecs);
    void fadeIn_sys(int msecs, bool loadFromMap);
    static bool isPlaying_sys();
    static Music *instance;
    static bool fading;
    static bool on;
    bool load_sys(const std::string &pathname);
    void playMid(Type music);
    static void stopMid();
    bool load(Type music);
};

#endif // ifndef MUSIC_H
