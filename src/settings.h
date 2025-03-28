/*
 * $Id$
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include <string>
#include <vector>
#include "observable.h"
#include "types.h"


#define MIN_SHAKE_INTERVAL 50
#define MAX_BATTLE_SPEED 10
#define MAX_KEY_DELAY 1000
#define MAX_KEY_INTERVAL 100
#define MAX_CYCLES_PER_SECOND 20
#define MIN_CYCLES_PER_SECOND 2
#define MAX_SPELL_EFFECT_SPEED 10
#define MAX_CAMP_TIME 10
#define MAX_INN_TIME 10
#define MAX_SHRINE_TIME 20
#define MAX_SHAKE_INTERVAL 200
#define MAX_VOLUME 10

#ifdef RASB_PI
# define DEFAULT_SCALE 1
#else
# define DEFAULT_SCALE 2
#endif
#define DEFAULT_FULLSCREEN 0
#define DEFAULT_FILTER "Point"
#define DEFAULT_VIDEO_TYPE "EGA"
#define DEFAULT_GEM_LAYOUT "Standard"
#define DEFAULT_LINEOFSIGHT "DOS"
#define DEFAULT_SCREEN_SHAKES 1
#define DEFAULT_GAMMA 100
#define DEFAULT_MUSIC_VOLUME 10
#define DEFAULT_SOUND_VOLUME 10
#define DEFAULT_VOLUME_FADES 1
#define DEFAULT_SHORTCUT_COMMANDS 0
#define DEFAULT_KEY_DELAY 500
#define DEFAULT_KEY_INTERVAL 30
#define DEFAULT_FILTER_MOVE_MESSAGES 0
#define DEFAULT_BATTLE_SPEED 5
#define DEFAULT_ENHANCEMENTS 1
#define DEFAULT_CYCLES_PER_SECOND 4
#define DEFAULT_ANIMATION_FRAMES_PER_SECOND 24
#define DEFAULT_DEBUG 0
#define DEFAULT_VALIDATE_XML 1
#define DEFAULT_SPELL_EFFECT_SPEED 10
#define DEFAULT_CAMP_TIME 10
#define DEFAULT_INN_TIME 8
#define DEFAULT_SHRINE_TIME 16
#define DEFAULT_SHAKE_INTERVAL 100
#define DEFAULT_BATTLE_DIFFICULTY "Normal"
#define DEFAULT_LOGGING ""
#define DEFAULT_TITLE_SPEED_RANDOM 150
#define DEFAULT_TITLE_SPEED_OTHER 30
#define DEFAULT_PAUSE_FOR_EACH_TURN 100
#define DEFAULT_PAUSE_FOR_EACH_MOVEMENT 10
// --Tile transparency stuff
#define DEFAULT_SHADOW_PIXEL_OPACITY 64
#define DEFAULT_SHADOW_PIXEL_SIZE 2

class SettingsEnhancementOptions {
public:
    SettingsEnhancementOptions()
        :activePlayer(false),
         u5spellMixing(false),
         u5shrines(false),
         u5combat(false),
         slimeDivides(false),
         gazerSpawnsInsects(false),
         textColorization(false),
         c64chestTraps(true),
         smartEnterKey(false),
         peerShowsObjects(false),
         u4TileTransparencyHack(false),
         u4TileTransparencyHackPixelShadowOpacity(
             DEFAULT_SHADOW_PIXEL_OPACITY
         ),
         u4TileTransparencyHackShadowBreadth(
             DEFAULT_SHADOW_PIXEL_SIZE
         )
    {
    }


    bool operator==(const SettingsEnhancementOptions &) const;
    bool operator!=(const SettingsEnhancementOptions &) const;
    bool activePlayer;
    bool u5spellMixing;
    bool u5shrines;
    bool u5combat;
    bool slimeDivides;
    bool gazerSpawnsInsects;
    bool textColorization;
    bool c64chestTraps;
    bool smartEnterKey;
    bool peerShowsObjects;
    bool u4TileTransparencyHack;
    int u4TileTransparencyHackPixelShadowOpacity;
    int u4TileTransparencyHackShadowBreadth;
};

class MouseOptions {
public:
    MouseOptions()
        :enabled(false)
    {
    }

    bool operator==(const MouseOptions &) const;
    bool operator!=(const MouseOptions &) const;
    bool enabled;
};


/**
 * SettingsData stores all the settings information.
 */
class SettingsData {
public:
    SettingsData()
        :battleSpeed(DEFAULT_BATTLE_SPEED),
         campingAlwaysCombat(0),
         campTime(DEFAULT_CAMP_TIME),
         debug(DEFAULT_DEBUG),
         enhancements(DEFAULT_ENHANCEMENTS),
         enhancementsOptions(),
         filterMoveMessages(DEFAULT_FILTER_MOVE_MESSAGES),
         fullscreen(DEFAULT_FULLSCREEN),
         gameCyclesPerSecond(DEFAULT_CYCLES_PER_SECOND),
         screenAnimationFramesPerSecond(DEFAULT_ANIMATION_FRAMES_PER_SECOND),
         innAlwaysCombat(0),
         innTime(DEFAULT_INN_TIME),
         keydelay(DEFAULT_KEY_DELAY),
         keyinterval(DEFAULT_KEY_INTERVAL),
         mouseOptions(),
         musicVol(DEFAULT_MUSIC_VOLUME),
#if 0
         scale(DEFAULT_SCALE),
#endif
         screenShakes(DEFAULT_SCREEN_SHAKES),
         gamma(DEFAULT_GAMMA),
         shakeInterval(DEFAULT_SHAKE_INTERVAL),
         shortcutCommands(DEFAULT_SHORTCUT_COMMANDS),
         shrineTime(DEFAULT_SHRINE_TIME),
         soundVol(DEFAULT_SOUND_VOLUME),
         spellEffectSpeed(DEFAULT_SPELL_EFFECT_SPEED),
         validateXml(DEFAULT_VALIDATE_XML),
         volumeFades(DEFAULT_VOLUME_FADES),
         titleSpeedRandom(DEFAULT_TITLE_SPEED_RANDOM),
         titleSpeedOther(DEFAULT_TITLE_SPEED_OTHER),
         pauseForEachTurn(DEFAULT_PAUSE_FOR_EACH_TURN),
         pauseForEachMovement(DEFAULT_PAUSE_FOR_EACH_MOVEMENT),
         filter(DEFAULT_FILTER),
         gemLayout(DEFAULT_GEM_LAYOUT),
         lineOfSight(DEFAULT_LINEOFSIGHT),
         videoType(DEFAULT_VIDEO_TYPE),
         battleDiff(DEFAULT_BATTLE_DIFFICULTY),
         logging(DEFAULT_LOGGING),
         game("Ultima IV")
    {
    }

    virtual ~SettingsData() = default;
    bool operator==(const SettingsData &) const;
    bool operator!=(const SettingsData &) const;
    int battleSpeed;
    bool campingAlwaysCombat;
    int campTime;
    bool debug;
    bool enhancements;
    SettingsEnhancementOptions enhancementsOptions;
    bool filterMoveMessages;
    bool fullscreen;
    int gameCyclesPerSecond;
    int screenAnimationFramesPerSecond;
    bool innAlwaysCombat;
    int innTime;
    int keydelay;
    int keyinterval;
    MouseOptions mouseOptions;
    int musicVol;
    static const int scale = DEFAULT_SCALE;
    bool screenShakes;
    int gamma;
    int shakeInterval;
    bool shortcutCommands;
    int shrineTime;
    int soundVol;
    int spellEffectSpeed;
    bool validateXml;
    bool volumeFades;
    int titleSpeedRandom;
    int titleSpeedOther;
    // Settings that aren't in file yet
    int pauseForEachTurn;
    int pauseForEachMovement;
    /**
     * Strings, classes, and other objects that cannot
     * be bitwise-compared must be placed here at the
     * end of the list so that our == and != operators
     * function correctly
     */
    std::string filter;
    std::string gemLayout;
    std::string lineOfSight;
    std::string videoType;
    std::string battleDiff;
    std::string logging;
    std::string game;
};


/**
 * The settings class is a singleton that holds all the settings
 * information.  It is dynamically initialized when first accessed.
 */
class Settings
    :public SettingsData,
     public Observable<Settings *> {
public:
    void init(const bool useProfile, const std::string &profileName);

    virtual ~Settings() = default;

    static Settings &getInstance()
    {
        if (__builtin_expect((instance == nullptr), false)) {
            instance = new Settings();
        }
        return *instance;
    }

    void setData(const SettingsData &data);
    bool read();
    bool write();
    const std::string &getUserPath() const;
    const std::vector<std::string> &getBattleDiffs() const;

private:
    typedef std::map<std::string, int, std::less<std::string> > SettingsMap;

    Settings();
    static Settings *instance;
    std::string userPath;
    std::string filename;
    std::vector<std::string> battleDiffs;
};

/* the global settings */
#define settings (Settings::getInstance())

#endif // ifndef SETTINGS_H
