/*
 * $Id$
 */
/** \mainpage xu4 Main Page
 *
 * \section intro_sec Introduction
 *
 * intro stuff goes here...
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

// #include <SDL.h>
#include "u4.h"
#include <cstdlib>
#include <cstring>
#include "armor.h"
#include "config.h"
#include "creature.h"
#include "debug.h"
#include "dialogueloader.h"
#include "dungeonview.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "imageloader.h"
#include "imageloader_u4.h"
#include "intro.h"
#include "map.h"
#include "maploader.h"
#include "mapmgr.h"
#include "music.h"
#include "object.h"
#include "person.h"
#include "progress_bar.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "tileset.h"
#include "u4file.h"
#include "utils.h"
#include "weapon.h"

bool verbose = false;
int quit = 0;
bool useProfile = false;
std::string profileName = "";
Performance perf("debug/performance.txt");


int main(int argc, char *argv[])
{
    U4FILE *avatar;
    Debug::initGlobal("debug/global.txt");
    if (!(avatar = u4fopen("AVATAR.EXE"))) {
        errorFatal(
            "xu4 erfordert die MS-DOS-Version von Ultima IV. "
            "Diese muss sich im gleichen Verzeichnisse befinden "
            "wie die ausfuehrbare Datei, oder in einem "
            "Unterverzeichnisse davon namens \"ultima4\"."
            "\n\nDies kannst Du erreichen, indem Du"
            "\n - \"ultima4.zip\" von "
            "https://ultima.thatfleminggent.com/ultima4.zip "
            "herunterlaedtst\n - die Datei \"ultima4.zip\" an den Ort "
            "der ausfuehrbaren Datei u4.exe kopierst."
            "\n\nBesuche die xu4-Webseite fuer weitere "
            "Informationen.\n\thttp://xu4.sourceforge.net/"
        );
    }
    u4fclose(avatar);
    unsigned int i;
    int skipIntro = 0;
    /*
     * if the -p or -profile arguments are passed to the application,
     * they need to be identified before the settings are initialized.
     */
    for (i = 1; i < static_cast<unsigned int>(argc); i++) {
        if (((std::strcmp(argv[i], "-p") == 0)
             || (std::strcmp(argv[i], "-profile") == 0)
             || (std::strcmp(argv[i], "--profile") == 0))
            && (static_cast<unsigned int>(argc) > i + 1)) {
            // when grabbing the profile name:
            // 1. trim leading whitespace
            // 2. truncate the string at 20 characters
            // 3. then strip any trailing whitespace
            profileName = argv[i + 1];
            profileName = profileName.erase(
                0, profileName.find_first_not_of(' ')
            );
            profileName.resize(20, ' ');
            profileName = profileName.erase(
                profileName.find_last_not_of(' ') + 1
            );
            // verify that profileName is valid, otherwise
            // do not use the profile
            if (!profileName.empty()) {
                useProfile = true;
            }
            break;
        }
    }
    /* initialize the settings */
    settings.init(useProfile, profileName);
    /* update the settings based upon command-line arguments */
    for (i = 1; i < static_cast<unsigned int>(argc); i++) {
        if ((std::strcmp(argv[i], "-filter") == 0)
            || (std::strcmp(argv[i], "--filter") == 0)) {
            if (static_cast<unsigned int>(argc) > i + 1) {
                settings.filter = argv[i + 1];
                i++;
            } else errorFatal(
                "%s is invalid alone: Requires a string for input."
                " See --help for more detail.\n",
                argv[i]
            );
#if 0
        }  else if ((std::strcmp(argv[i], "-scale") == 0)
                    && (static_cast<unsigned int>(argc) > i + 1)) {
            settings.scale = std::strtoul(argv[i + 1], nullptr, 0);
            i++;
#endif
        } else if ((std::strcmp(argv[i], "-p") == 0)
                   || (std::strcmp(argv[i], "-profile") == 0)
                   || (std::strcmp(argv[i], "--profile") == 0)) {
            if (static_cast<unsigned int>(argc) > i + 1) {
                // do nothing
                i++;
            } else errorFatal(
                "%s is invalid alone: Requires a number for input."
                " See --help for more detail.\n",
                argv[i]
            );
        } else if ((std::strcmp(argv[i], "-i") == 0)
                   || (std::strcmp(argv[i], "-skipintro") == 0)
                   || (std::strcmp(argv[i], "--skipintro") == 0)) {
            skipIntro = 1;
        } else if ((std::strcmp(argv[i], "-v") == 0)
                   || (std::strcmp(argv[i], "-verbose") == 0)
                   || (std::strcmp(argv[i], "--verbose") == 0)) {
            verbose = true;
        } else if ((std::strcmp(argv[i], "-f") == 0)
                   || (std::strcmp(argv[i], "-fullscreen") == 0)
                   || (std::strcmp(argv[i], "--fullscreen") == 0)) {
            settings.fullscreen = 1;
        } else if ((std::strcmp(argv[i], "-q") == 0)
                   || (std::strcmp(argv[i], "-quiet") == 0)
                   || (std::strcmp(argv[i], "--quiet") == 0)) {
            settings.musicVol = 0;
            settings.soundVol = 0;
        } else if ((std::strcmp(argv[i], "-h") == 0)
                   || (std::strcmp(argv[i], "-help") == 0)
                   || (std::strcmp(argv[i], "--help") == 0)) {
            std::printf("xu4: Ultima IV Neu Geschaffen und auf Deutsch\n");
            std::printf("v%s\n\n", VERSION);

            std::printf("-v, --verbose            Fuehrt xu4 im"
                        " ausfuehrlichen Modus aus."
                        " Mehr Konsolenausgabe.\n");
            std::printf("-q, --quiet              Setzt alle Lautstaerkeregler"
                        " auf Null.\n");
            std::printf("-f, --fullscreen         Fuehrt xu5g im Vollbildmodus"
                        " aus.\n");
            std::printf("-i, --skip-intro         Ueberspringt das Intro und"
                        " laedt den letzten Spielstand.\n");
#if 0
            std::printf("\n-s <Ganzzahl>,\n");
            std::printf("--scale <Ganzzahl>       Spezifiziert"
                        " Vergroesserungsfaktor.\n");
#endif
            std::printf("-p <Zeichenkette>,\n");
            std::printf("--profile <Zeichenkette> Uebergibt dem Programm"
                        " zusaetzliche Parameter.\n");
            std::printf("--filter <Zeichenkette>  Spezifiziert"
                        " Filteroptionen.\n");

            std::printf("\n-h, --help               Zeigt diese"
                        " Nachricht an.\n");

            std::printf("\nHomepage der englischen Version:"
                        " http://xu4.sourceforge.com\n");

            return 0;
        } else errorFatal(
            "Parameter unbekannt: %s\n\nVerwende --help, um eine Liste der"
            " unterstuetzten Parameter anzuzeigen.\n",
            argv[i]
        );
    }

    xu4_srandom();
    perf.start();
    screenInit();
    screenRedrawScreen();
    perf.end("Bildschirm wird aufgebaut");
    perf.start();
    soundInit();
    perf.end("Verschiedene Initialisierungen");
    perf.start();
    Tileset::loadAll();
    perf.end("Tileset::loadAll()");
    perf.start();
    creatureMgr->getInstance();
    perf.end("creatureMgr->getInstance()");
    intro = new IntroController();
    if (!skipIntro) {
        /* do the intro */
        perf.start();
        intro->init();
        perf.end("introInit()");
        perf.start();
        intro->preloadMap();
        perf.end("intro->preloadMap()");
        perf.start();
        musicMgr->init();
        perf.end("musicMgr->init()");
        /* give a performance report */
        if (settings.debug) {
            perf.report();
        }
        eventHandler->pushController(intro);
        eventHandler->run();
        eventHandler->setController(intro);
        eventHandler->popController();
        intro->deleteIntro();
    }
    eventHandler->setControllerDone(false);
    if (quit) {
        Tileset::unloadAll();
        MapMgr::destroy();
        U4ZipPackageMgr::destroy();
        delete musicMgr;
        soundDelete();
        screenDelete();
        Config::destroy();
        Weapon::cleanup();
        Armor::cleanup();
        Object::cleanup();
        DialogueLoader::cleanup();
        ImageLoader::cleanup();
        MapLoader::cleanup();
        DungeonView::cleanup();
        delete creatureMgr;
        delete intro;
        delete eventHandler;
        U4PaletteLoader::cleanup();
        delete &settings;
        delete &u4Path;
        return quit > 1 ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    perf.reset();
    /* play the game! */
    perf.start();
    game = new GameController();
    game->init();
    perf.end("gameInit()");
    /* give a performance report */
    if (settings.debug) {
        perf.report("\n===============================\n\n");
    }
    eventHandler->pushController(game);
    eventHandler->run();
    eventHandler->setController(game);
    eventHandler->popController();
    Tileset::unloadAll();
    MapMgr::destroy();
    U4ZipPackageMgr::destroy();
    delete musicMgr;
    soundDelete();
    screenDelete();
    Config::destroy();
    Weapon::cleanup();
    Armor::cleanup();
    Object::cleanup();
    DialogueLoader::cleanup();
    ImageLoader::cleanup();
    MapLoader::cleanup();
    DungeonView::cleanup();
    delete creatureMgr;
    delete intro;
    delete game;
    delete eventHandler;
    U4PaletteLoader::cleanup();
    delete &settings;
    delete &u4Path;
    return quit > 1 ? EXIT_FAILURE : EXIT_SUCCESS;
} // main
