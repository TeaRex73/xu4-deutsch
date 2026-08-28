#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>

#include <SDL.h> // IWYU pragma: keep
#include <SDL_mixer.h>

#include "music.h"

#include "debug.h"
#include "error.h"
#include "settings.h"
#include "u4_sdl.h"

void Music::create_sys() const
{
    /*
     * initialize sound subsystem
     */
    TRACE_LOCAL(*logger, "Initializing SDL sound subsystem");
    constexpr int audio_rate = 48000;
    constexpr Uint16 audio_format = AUDIO_S16LSB; /* 16-bit stereo */
    constexpr int audio_channels = 2;
    constexpr int audio_buffers = 1024;
    if (u4_SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
        errorWarning(
            "unable to init SDL audio subsystem: %s", SDL_GetError()
        );
        functional = false;
        return;
    }
    TRACE_LOCAL(*logger, "Initializing SDL_mixer");
    if (!(Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG)) {
        errorWarning(
            "unable to init SDL_mixer: %s", Mix_GetError()
        );
        functional = false;
        return;
    }
    TRACE_LOCAL(*logger, "Opening audio");
    if (Mix_OpenAudio(
            audio_rate, audio_format, audio_channels, audio_buffers
        )) {
        errorWarning("Unable to open audio!");
        functional = false;
        return;
    }
    functional = true;
    TRACE_LOCAL(*logger, "Allocating channels");
    Mix_AllocateChannels(16);
} // Music::create_sys

void Music::destroy_sys()
{
    if (playing) {
        TRACE_LOCAL(*logger, "Stopping currently playing music");
        Mix_FreeMusic(playing);
        playing = nullptr;
    }
    TRACE_LOCAL(*logger, "Closing audio");
    Mix_CloseAudio();
    TRACE_LOCAL(*logger, "Quitting SDL_mixer");
    Mix_Quit();
    TRACE_LOCAL(*logger, "Quitting SDL audio subsystem");
    u4_SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool Music::load_sys(const std::string &pathname)
{
    if (playing) {
        Mix_FreeMusic(playing);
        playing = nullptr;
    }
    playing = Mix_LoadMUS(pathname.c_str());
    if (!playing) {
        errorWarning(
            "unable to load music file %s: %s",
            pathname.c_str(),
            Mix_GetError()
        );
        return false;
    }
    return true;
}


/**
 * Play a midi file
 */
void Music::playMid(const Type music)
{
    if (!functional || !on || music == NONE) {
        return;
    }
    /* loaded a new piece of music */
    if (load(music)) {
        Mix_PlayMusic(playing, N_LOOPS);
    }
    setMusicVolume_sys(settings.musicVol);
    setSoundVolume_sys(settings.soundVol);
    // Mix_SetMusicPosition(0.0);
    //Could be useful if music was stored on different 'it/mod' patterns
}


/**
 * Stop playing a MIDI file.
 */
void Music::stopMid()
{
    Mix_HaltMusic();
}

void Music::freeze()
{
    Mix_PauseMusic();
}

void Music::thaw()
{
    Mix_ResumeMusic();
}

/**
 * Set, increase, and decrease sound volume
 */
void Music::setSoundVolume_sys(const int volume)
{
    /**
     * Use Channel 1 for sound effects
     */
    Mix_Volume(
        1,
        static_cast<int>(
            static_cast<double>(MIX_MAX_VOLUME) / MAX_VOLUME * volume
        )
    );
}


/**
 * System specific version to check if the version is still playing.
 */
bool Music::isPlaying_sys()
{
    return Mix_PlayingMusic();
} /**< Returns true if the mixer is playing any audio */


/**
 * Set, increase, and decrease music volume
 */
void Music::setMusicVolume_sys(const int volume)
{
    Mix_VolumeMusic(
        static_cast<int>(
            static_cast<double>(MIX_MAX_VOLUME) / MAX_VOLUME * volume
        )
    );
}

void Music::fadeIn_sys(const int msecs, bool) const
{
    if (Mix_FadeInMusic(playing, N_LOOPS, msecs) == -1) {
        errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());
    }
}

void Music::fadeOut_sys(const int msecs)
{
    if (Mix_FadeOutMusic(msecs) == -1) {
        errorWarning("Mix_FadeOutMusic: %s\n", Mix_GetError());
    }
}
