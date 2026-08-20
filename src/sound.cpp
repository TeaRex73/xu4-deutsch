/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <string>
#include <vector>

#include "sound.h"

#include "config.h"
#include "debug.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"

#include "sound_p.h"


int soundInit()
{
    return SoundManager::getInstance()->init();
}

void soundDelete()
{
    delete SoundManager::getInstance();
}

void soundLoad(const Sound sound)
{
    SoundManager::getInstance()->load(sound);
}

void soundPlay(
    const Sound sound,
    const bool onlyOnce,
    const int specificDurationInTicks,
    const bool wait
)
{
    SoundManager::getInstance()->play(sound,
                                      onlyOnce,
                                      specificDurationInTicks,
                                      wait);
}

void soundStop(const int channel)
{
    SoundManager::stop(channel);
}

SoundManager *SoundManager::instance = nullptr;

SoundManager::SoundManager() = default;

SoundManager::~SoundManager()
{
    del();
    instance = nullptr;
}

SoundManager *SoundManager::getInstance()
{
    if (__builtin_expect(!instance, 0)) {
        instance = new SoundManager();
    }
    return instance;
}

bool SoundManager::init()
{
    /*
     * load sound track filenames from xml config file
     */
    const Config *config = Config::getInstance();
    soundFilenames.reserve(SOUND_MAX);
    soundChunk.resize(SOUND_MAX, nullptr);
    const std::vector<ConfigElement> soundConfs =
        config->getElement("sound").getChildren();
    for (const auto &soundConf: soundConfs) {
        if (soundConf.getName() != "track") {
            continue;
        }
        soundFilenames.push_back(soundConf.getString("file"));
    }
    return init_sys();
}

bool SoundManager::load(const Sound sound)
{
    U4ASSERT(
        sound < SOUND_MAX, "Attempted to load an invalid sound in soundLoad()"
    );
    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol) {
        return false;
    }
    if (soundChunk[sound] == nullptr) {
        const std::string pathname(u4find_sound(soundFilenames[sound]));
        const std::string basename =
            pathname.substr(pathname.find_last_of('/') + 1);
        if (!basename.empty()) {
            return load_sys(sound, pathname);
        }
    }
    return true;
}

void SoundManager::play(
    const Sound sound,
    const bool onlyOnce,
    const int specificDurationInTicks,
    const bool wait
)
{
    U4ASSERT(
        sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()"
    );
    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol) {
        return;
    }
    if (soundChunk[sound] == nullptr) {
        if (!load(sound)) {
            return;
        }
    }
    play_sys(sound, onlyOnce, specificDurationInTicks, wait);
}

void SoundManager::stop(const int channel)
{
    stop_sys(channel);
}
