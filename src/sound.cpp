/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise
#include "sound.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"

#include "sound_p.h"


int soundInit(void)
{
    return SoundManager::getInstance()->init();
}

void soundDelete(void)
{
    delete SoundManager::getInstance();
}

void soundLoad(Sound sound)
{
    SoundManager::getInstance()->load(sound);
}

void soundPlay(
    Sound sound, bool onlyOnce, int specificDurationInTicks, bool wait
)
{
    SoundManager::getInstance()->play(sound,
                                      onlyOnce,
                                      specificDurationInTicks,
                                      wait);
}

void soundStop(int channel)
{
    SoundManager::getInstance()->stop(channel);
}

SoundManager *SoundManager::instance = 0;

SoundManager::SoundManager()
    :soundFilenames(), soundChunk()
{
}

SoundManager::~SoundManager()
{
    del();
    instance = 0;
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
    std::vector<ConfigElement> soundConfs =
        config->getElement("sound").getChildren();
    std::vector<ConfigElement>::const_iterator i = soundConfs.cbegin();
    std::vector<ConfigElement>::const_iterator theEnd = soundConfs.cend();
    for (; i != theEnd; ++i) {
        if (i->getName() != "track") {
            continue;
        }
        soundFilenames.push_back(i->getString("file"));
    }
    return init_sys();
}

bool SoundManager::load(Sound sound)
{
    U4ASSERT(
        sound < SOUND_MAX, "Attempted to load an invalid sound in soundLoad()"
    );
    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol) {
        return false;
    }
    if (soundChunk[sound] == nullptr) {
        std::string pathname(u4find_sound(soundFilenames[sound]));
        std::string basename =
            pathname.substr(pathname.find_last_of("/") + 1);
        if (!basename.empty()) {
            return load_sys(sound, pathname);
        }
    }
    return true;
}

void SoundManager::play(
    Sound sound, bool onlyOnce, int specificDurationInTicks, bool wait
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

void SoundManager::stop(int channel)
{
    stop_sys(channel);
}
