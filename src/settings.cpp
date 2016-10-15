/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "settings.h"

#include "debug.h"
#include "error.h"
#include "event.h"
#include "filesystem.h"
#include "utils.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#include <shlobj.h>
#elif defined(MACOSX)
#include <CoreServices/CoreServices.h>
#endif




/*
 * Initialize static members
 */
Settings *Settings::instance = NULL;

#if defined(_WIN32) || defined(__CYGWIN__)
#define SETTINGS_BASE_FILENAME "xu4.cfg"
#else
#define SETTINGS_BASE_FILENAME "xu4rc"
#endif

bool SettingsEnhancementOptions::operator==(
    const SettingsEnhancementOptions &s
) const
{
    if (activePlayer != s.activePlayer) {
        return false;
    }
    if (u5spellMixing != s.u5spellMixing) {
        return false;
    }
    if (u5shrines != s.u5shrines) {
        return false;
    }
    if (u5combat != s.u5combat) {
        return false;
    }
    if (slimeDivides != s.slimeDivides) {
        return false;
    }
    if (gazerSpawnsInsects != s.gazerSpawnsInsects) {
        return false;
    }
    if (textColorization != s.textColorization) {
        return false;
    }
    if (c64chestTraps != s.c64chestTraps) {
        return false;
    }
    if (smartEnterKey != s.smartEnterKey) {
        return false;
    }
    if (peerShowsObjects != s.peerShowsObjects) {
        return false;
    }
    if (u4TileTransparencyHack != s.u4TileTransparencyHack) {
        return false;
    }
    if (u4TileTransparencyHack != s.u4TileTransparencyHack) {
        return false;
    }
    if (u4TileTransparencyHackPixelShadowOpacity
        != s.u4TileTransparencyHackPixelShadowOpacity) {
        return false;
    }
    if (u4TileTransparencyHackShadowBreadth
        != s.u4TileTransparencyHackShadowBreadth) {
        return false;
    }
    return true;
}

bool SettingsEnhancementOptions::operator!=(
    const SettingsEnhancementOptions &s
) const
{
    return !operator==(s);
}


bool MouseOptions::operator==(const struct MouseOptions &s) const
{
    if (enabled != s.enabled) {
        return false;
    }
    return true;
}

bool MouseOptions::operator!=(const struct MouseOptions &s) const
{
    return !operator==(s);
}

bool SettingsData::operator==(const SettingsData &s) const
{
    if (battleSpeed != s.battleSpeed) {
        return false;
    }
    if (campTime != s.campTime) {
        return false;
    }
    if (debug != s.debug) {
        return false;
    }
    if (enhancements != s.enhancements) {
        return false;
    }
    if (enhancementsOptions != s.enhancementsOptions) {
        return false;
    }
    if (filterMoveMessages != s.filterMoveMessages) {
        return false;
    }
    if (fullscreen != s.fullscreen) {
        return false;
    }
    if (gameCyclesPerSecond != s.gameCyclesPerSecond) {
        return false;
    }
    if (screenAnimationFramesPerSecond
        != s.screenAnimationFramesPerSecond) {
        return false;
    }
    if (innAlwaysCombat != s.innAlwaysCombat) {
        return false;
    }
    if (innTime != s.innTime) {
        return false;
    }
    if (keydelay != s.keydelay) {
        return false;
    }
    if (keyinterval != s.keyinterval) {
        return false;
    }
    if (mouseOptions != s.mouseOptions) {
        return false;
    }
    if (musicVol != s.musicVol) {
        return false;
    }
    if (scale != s.scale) {
        return false;
    }
    if (screenShakes != s.screenShakes) {
        return false;
    }
    if (gamma != s.gamma) {
        return false;
    }
    if (shakeInterval != s.shakeInterval) {
        return false;
    }
    if (shortcutCommands != s.shortcutCommands) {
        return false;
    }
    if (shrineTime != s.shrineTime) {
        return false;
    }
    if (soundVol != s.soundVol) {
        return false;
    }
    if (spellEffectSpeed != s.spellEffectSpeed) {
        return false;
    }
    if (validateXml != s.validateXml) {
        return false;
    }
    if (volumeFades != s.volumeFades) {
        return false;
    }
    if (titleSpeedRandom != s.titleSpeedRandom) {
        return false;
    }
    if (titleSpeedOther != s.titleSpeedOther) {
        return false;
    }
    if (soundVol != s.soundVol) {
        return false;
    }
    if (pauseForEachTurn != s.pauseForEachTurn) {
        return false;
    }
    if (pauseForEachMovement != s.pauseForEachMovement) {
        return false;
    }
    if (filter != s.filter) {
        return false;
    }
    if (gemLayout != s.gemLayout) {
        return false;
    }
    if (lineOfSight != s.lineOfSight) {
        return false;
    }
    if (videoType != s.videoType) {
        return false;
    }
    if (battleDiff != s.battleDiff) {
        return false;
    }
    if (logging != s.logging) {
        return false;
    }
    if (game != s.game) {
        return false;
    }
    return true;
} // operator==


bool SettingsData::operator!=(const SettingsData &s) const
{
    return !operator==(s);
}


/**
 * Default contructor.  Settings is a singleton so this is private.
 */
Settings::Settings()
{
    battleDiffs.push_back("Normal");
    battleDiffs.push_back("Hard");
    battleDiffs.push_back("Expert");
}


/**
 * Initialize the settings.
 */
void Settings::init(const bool useProfile, const std::string profileName)
{
    if (useProfile) {
        userPath = "./profiles/";
        userPath += profileName.c_str();
        userPath += "/";
    } else {
#if defined(MACOSX)
        FSRef folder;
        OSErr err = FSFindFolder(
            kUserDomain, kApplicationSupportFolderType, kCreateFolder, &folder
        );
        if (err == noErr) {
            UInt8 path[2048];
            if (FSRefMakePath(&folder, path, 2048) == noErr) {
                userPath.append(reinterpret_cast<const char *>(path));
                userPath.append("/xu4/");
            }
        }
        if (userPath.empty()) {
            char *home = std::getenv("HOME");
            if (home && home[0]) {
                if (userPath.size() == 0) {
                    userPath += home;
                    userPath += "/.xu4";
                    userPath += "/";
                }
            } else {
                userPath = "./";
            }
        }

#elif defined(__unix__)
        char *home = std::getenv("HOME");
        if (home && home[0]) {
            userPath += home;
            userPath += "/.xu4";
            userPath += "/";
        } else {
            userPath = "./";
        }

#elif defined(_WIN32) || defined(__CYGWIN__)
        userPath = "./";
        LPMALLOC pMalloc = NULL;
        if (SHGetMalloc(&pMalloc) == S_OK) {
            LPITEMIDLIST pItemIDList = NULL;
            if ((SHGetSpecialFolderLocation(
                     NULL, CSIDL_APPDATA, &pItemIDList
                 ) == S_OK) && (pItemIDList != NULL)) {
                LPSTR pBuffer = NULL;
                if ((pBuffer = (LPSTR)pMalloc->Alloc(MAX_PATH + 2)) != NULL) {
                    if (SHGetPathFromIDList(pItemIDList, pBuffer) == TRUE) {
                        userPath = pBuffer;
                        userPath += "/xu4/";
                    }
                    pMalloc->Free(pBuffer);
                }
                pMalloc->Free(pItemIDList);
            }
            pMalloc->Release();
        }
#else // if defined(MACOSX)
        userPath = "./";
#endif // if defined(MACOSX)
    }
    FileSystem::createDirectory(userPath);
    filename = userPath + SETTINGS_BASE_FILENAME;
    read();
} // Settings::init


/**
 * Return the global instance of settings.
 */
#if 0
Settings &Settings::getInstance()
{
    if (instance == NULL) {
        instance = new Settings();
    }
    return *instance;
}
#endif

void Settings::setData(const SettingsData &data)
{
    // bitwise copy is safe
    *(SettingsData *)this = data;
}


/**
 * Read settings in from the settings file.
 */
bool Settings::read()
{
    char buffer[256];
    std::FILE *settingsFile;
    extern int eventTimerGranularity;
    /* default settings */
#if 0
    scale = DEFAULT_SCALE;
#endif
    fullscreen = DEFAULT_FULLSCREEN;
    filter = DEFAULT_FILTER;
    videoType = DEFAULT_VIDEO_TYPE;
    gemLayout = DEFAULT_GEM_LAYOUT;
    lineOfSight = DEFAULT_LINEOFSIGHT;
    screenShakes = DEFAULT_SCREEN_SHAKES;
    gamma = DEFAULT_GAMMA;
    musicVol = DEFAULT_MUSIC_VOLUME;
    soundVol = DEFAULT_SOUND_VOLUME;
    volumeFades = DEFAULT_VOLUME_FADES;
    shortcutCommands = DEFAULT_SHORTCUT_COMMANDS;
    keydelay = DEFAULT_KEY_DELAY;
    keyinterval = DEFAULT_KEY_INTERVAL;
    filterMoveMessages = DEFAULT_FILTER_MOVE_MESSAGES;
    battleSpeed = DEFAULT_BATTLE_SPEED;
    enhancements = DEFAULT_ENHANCEMENTS;
    gameCyclesPerSecond = DEFAULT_CYCLES_PER_SECOND;
    screenAnimationFramesPerSecond = DEFAULT_ANIMATION_FRAMES_PER_SECOND;
    debug = DEFAULT_DEBUG;
    battleDiff = DEFAULT_BATTLE_DIFFICULTY;
    validateXml = DEFAULT_VALIDATE_XML;
    spellEffectSpeed = DEFAULT_SPELL_EFFECT_SPEED;
    campTime = DEFAULT_CAMP_TIME;
    innTime = DEFAULT_INN_TIME;
    shrineTime = DEFAULT_SHRINE_TIME;
    shakeInterval = DEFAULT_SHAKE_INTERVAL;
    titleSpeedRandom = DEFAULT_TITLE_SPEED_RANDOM;
    titleSpeedOther = DEFAULT_TITLE_SPEED_OTHER;
    pauseForEachMovement = DEFAULT_PAUSE_FOR_EACH_MOVEMENT;
    pauseForEachTurn = DEFAULT_PAUSE_FOR_EACH_TURN;
    /* all specific minor enhancements default to "on",
       any major enhancements default to "off" */
    enhancementsOptions.activePlayer = false;
    enhancementsOptions.u5spellMixing = false;
    enhancementsOptions.u5shrines = false;
    enhancementsOptions.slimeDivides = false;
    enhancementsOptions.gazerSpawnsInsects = false;
    enhancementsOptions.textColorization = false;
    enhancementsOptions.c64chestTraps = true;
    enhancementsOptions.smartEnterKey = false;
    enhancementsOptions.peerShowsObjects = false;
    enhancementsOptions.u5combat = false;
    enhancementsOptions.u4TileTransparencyHack = false;
    enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity =
        DEFAULT_SHADOW_PIXEL_OPACITY;
    enhancementsOptions.u4TileTransparencyHackShadowBreadth =
        DEFAULT_SHADOW_PIXEL_SIZE;
    innAlwaysCombat = 0;
    campingAlwaysCombat = 0;
    /* mouse defaults to on */
    mouseOptions.enabled = 0;
    logging = DEFAULT_LOGGING;
    game = "Ultima IV";
    settingsFile = std::fopen(filename.c_str(), "rt");
    if (!settingsFile) {
        return false;
    }
    while (std::fgets(buffer, sizeof(buffer), settingsFile) != NULL) {
        while (std::isspace(buffer[std::strlen(buffer) - 1])) {
            buffer[std::strlen(buffer) - 1] = '\0';
        }
        if (std::strstr(buffer, "scale=") == buffer) {
            /* do nothing */
            /* scale =
                (unsigned int) std::strtoul(
                    buffer + std::strlen("scale="), NULL, 0
               ); */
        }
        else if (std::strstr(buffer, "fullscreen=") == buffer) {
            fullscreen = (int)std::strtoul(
                buffer + std::strlen("fullscreen="), NULL, 0
            );
        } else if (std::strstr(buffer, "filter=") == buffer) {
            filter = buffer + std::strlen("filter=");
        } else if (std::strstr(buffer, "video=") == buffer) {
            videoType = buffer + std::strlen("video=");
        } else if (std::strstr(buffer, "gemLayout=") == buffer) {
            gemLayout = buffer + std::strlen("gemLayout=");
        } else if (std::strstr(buffer, "lineOfSight=") == buffer) {
            lineOfSight = buffer + std::strlen("lineOfSight=");
        } else if (std::strstr(buffer, "screenShakes=") == buffer) {
            screenShakes = (int)std::strtoul(
                buffer + std::strlen("screenShakes="), NULL, 0
            );
        } else if (std::strstr(buffer, "gamma=") == buffer) {
            gamma = (int)std::strtoul(
                buffer + std::strlen("gamma="), NULL, 0
            );
        } else if (std::strstr(buffer, "musicVol=") == buffer) {
            musicVol = (int)std::strtoul(
                buffer + std::strlen("musicVol="), NULL, 0
            );
        } else if (std::strstr(buffer, "soundVol=") == buffer) {
            soundVol = (int)std::strtoul(
                buffer + std::strlen("soundVol="), NULL, 0
            );
        } else if (std::strstr(buffer, "volumeFades=") == buffer) {
            volumeFades = (int)std::strtoul(
                buffer + std::strlen("volumeFades="), NULL, 0
            );
        } else if (std::strstr(buffer, "shortcutCommands=") == buffer) {
            shortcutCommands = (int)std::strtoul(
                buffer + std::strlen("shortcutCommands="), NULL, 0
            );
        } else if (std::strstr(buffer, "keydelay=") == buffer) {
            keydelay = (int)std::strtoul(
                buffer + std::strlen("keydelay="), NULL, 0
            );
        } else if (std::strstr(buffer, "keyinterval=") == buffer) {
            keyinterval = (int)std::strtoul(
                buffer + std::strlen("keyinterval="), NULL, 0
            );
        } else if (std::strstr(buffer, "filterMoveMessages=") == buffer) {
            filterMoveMessages = (int)std::strtoul(
                buffer + std::strlen("filterMoveMessages="),
                NULL,
                0
            );
        } else if (std::strstr(buffer, "battlespeed=") == buffer) {
            battleSpeed = (int)std::strtoul(
                buffer + std::strlen("battlespeed="), NULL, 0
            );
        } else if (std::strstr(buffer, "enhancements=") == buffer) {
            enhancements = (int)std::strtoul(
                buffer + std::strlen("enhancements="), NULL, 0
            );
        } else if (std::strstr(buffer, "gameCyclesPerSecond=") == buffer) {
            gameCyclesPerSecond = (int)std::strtoul(
                buffer + std::strlen("gameCyclesPerSecond="), NULL, 0
            );
        } else if (std::strstr(buffer, "debug=") == buffer) {
            debug = (int)std::strtoul(
                buffer + std::strlen("debug="), NULL, 0
            );
        } else if (std::strstr(buffer, "battleDiff=") == buffer) {
            battleDiff = buffer + std::strlen("battleDiff=");
        } else if (std::strstr(buffer, "validateXml=") == buffer) {
            validateXml = (int)std::strtoul(
                buffer + std::strlen("validateXml="), NULL, 0
            );
        } else if (std::strstr(buffer, "spellEffectSpeed=") == buffer) {
            spellEffectSpeed = (int)std::strtoul(
                buffer + std::strlen("spellEffectSpeed="), NULL, 0
            );
        } else if (std::strstr(buffer, "campTime=") == buffer) {
            campTime = (int)std::strtoul(
                buffer + std::strlen("campTime="), NULL, 0
            );
        } else if (std::strstr(buffer, "innTime=") == buffer) {
            innTime = (int)std::strtoul(
                buffer + std::strlen("innTime="), NULL, 0
            );
        } else if (std::strstr(buffer, "shrineTime=") == buffer) {
            shrineTime = (int)std::strtoul(
                buffer + std::strlen("shrineTime="), NULL, 0
            );
        } else if (std::strstr(buffer, "shakeInterval=") == buffer) {
            shakeInterval = (int)std::strtoul(
                buffer + std::strlen("shakeInterval="), NULL, 0
            );
        } else if (std::strstr(buffer, "titleSpeedRandom=") == buffer) {
            titleSpeedRandom = (int)std::strtoul(
                buffer + std::strlen("titleSpeedRandom="), NULL, 0
            );
        } else if (std::strstr(buffer, "titleSpeedOther=") == buffer) {
            titleSpeedOther = (int)std::strtoul(
                buffer + std::strlen("titleSpeedOther="), NULL, 0
            );
        }
        /* minor enhancement options */
        else if (std::strstr(buffer, "activePlayer=") == buffer) {
            enhancementsOptions.activePlayer = (int)std::strtoul(
                buffer + std::strlen("activePlayer="), NULL, 0
            );
        } else if (std::strstr(buffer, "u5spellMixing=") == buffer) {
            enhancementsOptions.u5spellMixing = (int)std::strtoul(
                buffer + std::strlen("u5spellMixing="), NULL, 0
            );
        } else if (std::strstr(buffer, "u5shrines=") == buffer) {
            enhancementsOptions.u5shrines = (int)std::strtoul(
                buffer + std::strlen("u5shrines="), NULL, 0
            );
        } else if (std::strstr(buffer, "slimeDivides=") == buffer) {
            enhancementsOptions.slimeDivides = (int)std::strtoul(
                buffer + std::strlen("slimeDivides="), NULL, 0
            );
        } else if (std::strstr(buffer, "gazerSpawnsInsects=") == buffer) {
            enhancementsOptions.gazerSpawnsInsects = (int)std::strtoul(
                buffer + std::strlen("gazerSpawnsInsects="), NULL, 0
            );
        } else if (std::strstr(buffer, "textColorization=") == buffer) {
            enhancementsOptions.textColorization = (int)std::strtoul(
                buffer + std::strlen("textColorization="), NULL, 0
            );
        } else if (std::strstr(buffer, "c64chestTraps=") == buffer) {
            enhancementsOptions.c64chestTraps = (int)std::strtoul(
                buffer + std::strlen("c64chestTraps="), NULL, 0
            );
        } else if (std::strstr(buffer, "smartEnterKey=") == buffer) {
            enhancementsOptions.smartEnterKey = (int)std::strtoul(
                buffer + std::strlen("smartEnterKey="), NULL, 0
            );
        }
        /* major enhancement options */
        else if (std::strstr(buffer, "peerShowsObjects=") == buffer) {
            enhancementsOptions.peerShowsObjects = (int)std::strtoul(
                buffer + std::strlen("peerShowsObjects="), NULL, 0
            );
        } else if (std::strstr(buffer, "u5combat=") == buffer) {
            enhancementsOptions.u5combat = (int)std::strtoul(
                buffer + std::strlen("u5combat="), NULL, 0
            );
        } else if (std::strstr(buffer, "innAlwaysCombat=") == buffer) {
            innAlwaysCombat = (int)std::strtoul(
                buffer + std::strlen("innAlwaysCombat="), NULL, 0
            );
        } else if (std::strstr(buffer, "campingAlwaysCombat=") == buffer) {
            campingAlwaysCombat = (int)std::strtoul(
                buffer + std::strlen("campingAlwaysCombat="), NULL, 0
            );
        }
        /* mouse options */
        else if (std::strstr(buffer, "mouseEnabled=") == buffer) {
            mouseOptions.enabled = (int)std::strtoul(
                buffer + std::strlen("mouseEnabled="), NULL, 0
            );
        } else if (std::strstr(buffer, "logging=") == buffer) {
            logging = buffer + std::strlen("logging=");
        } else if (std::strstr(buffer, "game=") == buffer) {
            game = buffer + std::strlen("game=");
        }
        /* graphics enhancements options */
        else if (std::strstr(buffer, "renderTileTransparency=") == buffer) {
            enhancementsOptions.u4TileTransparencyHack = (int)std::strtoul(
                buffer + std::strlen("renderTileTransparency="), NULL, 0
            );
        } else if (std::strstr(buffer, "transparentTilePixelShadowOpacity=")
                   == buffer) {
            enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity =
                (int)std::strtoul(
                    buffer + std::strlen("transparentTilePixelShadowOpacity="),
                    NULL,
                    0
                );
        } else if (std::strstr(buffer, "transparentTileShadowSize=")
                   == buffer) {
            enhancementsOptions.u4TileTransparencyHackShadowBreadth =
                (int)std::strtoul(
                    buffer + std::strlen("transparentTileShadowSize="),
                    NULL,
                    0
                );
        }
        /**
         * FIXME: this is just to avoid an error for those who
         * have not written a new xu4.cfg file since these items
         * were removed.  Remove them after a reasonable
         * amount of time
         *
         * remove:  attackspeed, minorEnhancements,
         * majorEnhancements, vol
         */
        else if (std::strstr(buffer, "attackspeed=") == buffer) {
            /* do nothing */
        } else if (std::strstr(buffer, "minorEnhancements=") == buffer) {
            enhancements = (int)std::strtoul(
                buffer + std::strlen("minorEnhancements="),
                NULL,
                0
            );
        } else if (std::strstr(buffer, "majorEnhancements=") == buffer) {
            /* do nothing */
        } else if (std::strstr(buffer, "vol=") == buffer) {
            musicVol = soundVol = (int)std::strtoul(
                buffer + std::strlen("vol="), NULL, 0
            );
        }
        /***/
        else {
            errorWarning(
                "invalid line in settings file %s", buffer
            );
        }
    }
    std::fclose(settingsFile);
    eventTimerGranularity = (1000 / gameCyclesPerSecond);
    return true;
} // Settings::read


/**
 * Write the settings out into a human readable file.  This also
 * notifies observers that changes have been commited.
 */
bool Settings::write()
{
    std::FILE *settingsFile;
    settingsFile = std::fopen(filename.c_str(), "wt");
    if (!settingsFile) {
        errorWarning("can't write settings file");
        return false;
    }
    std::fprintf(
        settingsFile,
        "scale=%d\n"
        "fullscreen=%d\n"
        "filter=%s\n"
        "video=%s\n"
        "gemLayout=%s\n"
        "lineOfSight=%s\n"
        "screenShakes=%d\n"
        "gamma=%d\n"
        "musicVol=%d\n"
        "soundVol=%d\n"
        "volumeFades=%d\n"
        "shortcutCommands=%d\n"
        "keydelay=%d\n"
        "keyinterval=%d\n"
        "filterMoveMessages=%d\n"
        "battlespeed=%d\n"
        "enhancements=%d\n"
        "gameCyclesPerSecond=%d\n"
        "debug=%d\n"
        "battleDiff=%s\n"
        "validateXml=%d\n"
        "spellEffectSpeed=%d\n"
        "campTime=%d\n"
        "innTime=%d\n"
        "shrineTime=%d\n"
        "shakeInterval=%d\n"
        "titleSpeedRandom=%d\n"
        "titleSpeedOther=%d\n"
        "activePlayer=%d\n"
        "u5spellMixing=%d\n"
        "u5shrines=%d\n"
        "slimeDivides=%d\n"
        "gazerSpawnsInsects=%d\n"
        "textColorization=%d\n"
        "c64chestTraps=%d\n"
        "smartEnterKey=%d\n"
        "peerShowsObjects=%d\n"
        "u5combat=%d\n"
        "innAlwaysCombat=%d\n"
        "campingAlwaysCombat=%d\n"
        "mouseEnabled=%d\n"
        "logging=%s\n"
        "game=%s\n"
        "renderTileTransparency=%d\n"
        "transparentTilePixelShadowOpacity=%d\n"
        "transparentTileShadowSize=%d\n",
        scale,
        fullscreen,
        filter.c_str(),
        videoType.c_str(),
        gemLayout.c_str(),
        lineOfSight.c_str(),
        screenShakes,
        gamma,
        musicVol,
        soundVol,
        volumeFades,
        shortcutCommands,
        keydelay,
        keyinterval,
        filterMoveMessages,
        battleSpeed,
        enhancements,
        gameCyclesPerSecond,
        debug,
        battleDiff.c_str(),
        validateXml,
        spellEffectSpeed,
        campTime,
        innTime,
        shrineTime,
        shakeInterval,
        titleSpeedRandom,
        titleSpeedOther,
        enhancementsOptions.activePlayer,
        enhancementsOptions.u5spellMixing,
        enhancementsOptions.u5shrines,
        enhancementsOptions.slimeDivides,
        enhancementsOptions.gazerSpawnsInsects,
        enhancementsOptions.textColorization,
        enhancementsOptions.c64chestTraps,
        enhancementsOptions.smartEnterKey,
        enhancementsOptions.peerShowsObjects,
        enhancementsOptions.u5combat,
        innAlwaysCombat,
        campingAlwaysCombat,
        mouseOptions.enabled,
        logging.c_str(),
        game.c_str(),
        enhancementsOptions.u4TileTransparencyHack,
        enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity,
        enhancementsOptions.u4TileTransparencyHackShadowBreadth
    );
    fsync(fileno(settingsFile));
    std::fclose(settingsFile);
    sync();
    setChanged();
    notifyObservers(NULL);
    return true;
}


/**
 * Return the path where user settings are stored.
 */
const std::string &Settings::getUserPath()
{
    return userPath;
}

const std::vector<std::string> &Settings::getBattleDiffs()
{
    return battleDiffs;
}
