/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise
/* FIXME: should this file have all SDL-related stuff extracted and
   put in music_sdl.c? */
// Yes! :)

#include <memory>
#include <string>
#include <vector>

#include "music.h"
#include "sound.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "settings.h"
#include "u4.h"
#include "u4file.h"


/*
 * Static variables
 */
Music *Music::instance = nullptr;
bool Music::fading = false;
bool Music::on = false;
bool Music::functional = true;


/*
 * Constructors/Destructors
 */


/**
 * Initialize the music
 */
Music::Music()
    :filenames(),
     introMid(NONE),
     current(NONE),
     playing(nullptr),
     logger(new Debug("debug/music.txt", "Music"))
{
    filenames.reserve(MAX);
    filenames.push_back(""); // filename for MUSIC_NONE;
    TRACE(*logger, "Initializing music");
    /*
     * load music track filenames from xml config file
     */
    const Config *config = Config::getInstance();
    TRACE_LOCAL(*logger, "Loading music tracks");
    std::vector<ConfigElement> musicConfs =
        config->getElement("music").getChildren();
    std::vector<ConfigElement>::const_iterator i = musicConfs.cbegin();
    std::vector<ConfigElement>::const_iterator theEnd = musicConfs.cend();
    for (; i != theEnd; ++i) {
        if (i->getName() != "track") {
            continue;
        }
        filenames.push_back(i->getString("file"));
        TRACE_LOCAL(*logger, std::string("\tTrack file: ") + filenames.back());
    }
    create_sys(); // Call the Sound System specific creation file.
    // Set up the volume.
    on = (settings.musicVol > 0);
    setMusicVolume(settings.musicVol);
    setSoundVolume(settings.soundVol);
    eventHandler->getTimer()->add(
        &Music::callback, settings.gameCyclesPerSecond
    );
    TRACE(
        *logger,
        std::string("Music initialized: volume is ") + (on ? "on" : "off")
    );
}


/**
 * Stop playing the music and cleanup
 */
Music::~Music()
{
    TRACE(*logger, "Uninitializing music");
    eventHandler->getTimer()->remove(&Music::callback);
    destroy_sys(); // Call the Sound System specific destruction file.
    TRACE(*logger, "Music uninitialized");
    delete logger;
}


bool Music::load(Type music)
{
    U4ASSERT(
        music < MAX,
        "Attempted to load an invalid piece of music in Music::load()"
    );
    /* music already loaded */
    if (music == current) {
        /* tell calling function it didn't load correctly
           (because it's already playing) */
        if (isPlaying()) {
            return false;
        }
        /* it loaded correctly */
        else {
            return true;
        }
    }
    std::string pathname(u4find_music(filenames[music]));
    if (!pathname.empty()) {
        bool status = load_sys(pathname);
        if (status) {
            current = music;
        }
        return status;
    }
    return false;
} // Music::load


/**
 * Ensures that the music is playing if it is supposed to be, or off
 * if it is supposed to be turned off.
 */
void Music::callback(void *)
{
    eventHandler->getTimer()->remove(&Music::callback);
    if (musicMgr->on && !isPlaying()) {
        musicMgr->playCurrent();
    } else if (!musicMgr->on && isPlaying()) {
        musicMgr->stop();
    }
    eventHandler->getTimer()->add(&Music::callback,
                                  settings.gameCyclesPerSecond);
}

void Music::playCurrent()
{
    if (introMid) {
        int next = static_cast<int>(introMid + 1);
        if (next >= static_cast<int>(EXTRA)) next = 1;
        introSwitch(next);
        return;
    }
    if (current) {
        playMid(current);
    }
}


/**
 * Main music loop
 */
void Music::play()
{
    playMid(c->location->map->music);
}


/**
 * Cycle through the introduction music
 */
void Music::introSwitch(int n)
{
    if ((n > NONE) && (n < MAX)) {
        introMid = static_cast<Type>(n);
        intro();
    }
}


/**
 * Toggle the music on/off (usually by pressing 'v')
 */
bool Music::toggle()
{
    eventHandler->getTimer()->remove(&Music::callback);
    on = !on;
    if (!on) {
        fadeOut(1000);
    } else {
        fadeIn(1000, true);
    }
    eventHandler->getTimer()->add(
        &Music::callback, settings.gameCyclesPerSecond
    );
    play();
    return on;
}


/**
 * Fade out the music
 */
void Music::fadeOut(int msecs)
{
    // fade the music out even if 'on' is false
    if (!functional) {
        return;
    }
    if (isPlaying()) {
        if (!settings.volumeFades) {
            stop();
        } else {
            fadeOut_sys(msecs);
        }
    }
}


/**
 * Fade in the music
 */
void Music::fadeIn(int msecs, bool loadFromMap)
{
    if (!functional || on) {
        return;
    }
    if (!isPlaying()) {
        /* make sure we've got something loaded to play */
        if (loadFromMap || !playing) {
            load(c->location->map->music);
        }
        if (!settings.volumeFades) {
            play();
        } else {
            fadeIn_sys(msecs, loadFromMap);
        }
    }
}

int Music::increaseMusicVolume()
{
    if (++settings.musicVol > MAX_VOLUME) {
        settings.musicVol = MAX_VOLUME;
    } else {
        setMusicVolume(settings.musicVol);
    }
    return settings.musicVol * 100 / MAX_VOLUME; // percentage
}

int Music::decreaseMusicVolume()
{
    if (--settings.musicVol < 0) {
        settings.musicVol = 0;
    } else {
        setMusicVolume(settings.musicVol);
    }
    return settings.musicVol * 100 / MAX_VOLUME; // percentage
}

int Music::increaseSoundVolume()
{
    if (++settings.soundVol > MAX_VOLUME) {
        settings.soundVol = MAX_VOLUME;
    } else {
        setSoundVolume(settings.soundVol);
    }
    return settings.soundVol * 100 / MAX_VOLUME; // percentage
}

int Music::decreaseSoundVolume()
{
    if (--settings.soundVol < 0) {
        settings.soundVol = 0;
    } else {
        setSoundVolume(settings.soundVol);
    }
    return settings.soundVol * 100 / MAX_VOLUME; // percentage
}
