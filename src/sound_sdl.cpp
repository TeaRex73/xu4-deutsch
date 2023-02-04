 /*
 * SoundMgr_SDL.cpp
 *
 *  Created on: 2011-01-27
 *      Author: Darren Janeczek
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "sound_p.h"

#include <atomic>

#include <SDL.h>
#include <SDL_mixer.h>

#include "sound.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"
#include "event.h"

bool SoundManager::load_sys(Sound sound, const std::string &pathname)
{
    soundChunk[sound] = Mix_LoadWAV(pathname.c_str());
    if (!soundChunk[sound]) {
        errorWarning(
            "Unable to load sound effect file %s: %s",
            soundFilenames[sound].c_str(),
            Mix_GetError()
        );
        return false;
    }
    return true;
}

static std::atomic_bool finished;

static void channel_finished(int channel)
{
    if (channel == 1) finished = true;
}

void SoundManager::play_sys(
    Sound sound, bool onlyOnce, int specificDurationInTicks, bool wait
)
{
    /**
     * Use Channel 1 for sound effects
     */
    // make sure finished is in the right state. prevent race conditions.
    if (Mix_Playing(1)) {
        finished = false;
    }
    if (!Mix_Playing(1)) {
        finished = true;
    }
    // spin lock - EventHandler::sleep() might block far too long
    while (!onlyOnce && !finished) /* do nothing */ ;
    if (finished || !onlyOnce) {
        finished = false;
        if (
            Mix_PlayChannelTimed(
                1,
                soundChunk[sound],
                (specificDurationInTicks == -1) ? 0 : -1,
                specificDurationInTicks
            ) < 0
        ) {
            errorFatal(
                "Error playing sound %d: %s\n", sound, Mix_GetError()
            );
        }
        // spin lock, see above
        while (wait && !finished) /* do nothing */ ;
    }
}

void SoundManager::stop_sys(int channel)
{
    // If music didn't initialize correctly, then we shouldn't try to stop it
    if (!musicMgr->functional || !settings.soundVol) {
        return;
    }
    if (!finished) {
        Mix_HaltChannel(channel);
    }
}

bool SoundManager::init_sys()
{
    finished = true;
    Mix_ChannelFinished(&channel_finished);
    return true;
}

void SoundManager::del_sys()
{
    stop_sys(1);
    Mix_ChannelFinished(nullptr);
    for (int i = 0; i < SOUND_MAX; i++) {
        if (soundChunk[i] != nullptr) {
            Mix_FreeChunk(soundChunk[i]);
            soundChunk[i] = nullptr;
        }
    }
}
