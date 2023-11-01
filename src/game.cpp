/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <atomic>
#include <cctype>
#include <cstring>
#include <ctime>
#include <map>
#include <unistd.h>
#include "u4.h"

#include "game.h"

#include "annotation.h"
#include "armor.h"
#include "camp.h"
#include "cheat.h"
#include "city.h"
#include "conversation.h"
#include "debug.h"
#include "dungeon.h"
#include "combat.h"
#include "context.h"
#include "death.h"
#include "debug.h"
#include "direction.h"
#include "error.h"
#include "event.h"
#include "intro.h"
#include "item.h"
#include "imagemgr.h"
#include "location.h"
#include "mapmgr.h"
#include "menu.h"
#include "creature.h"
#include "moongate.h"
#include "movement.h"
#include "music.h"
#include "names.h"
#include "person.h"
#include "player.h"
#include "portal.h"
#include "progress_bar.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "sound.h"
#include "spell.h"
#include "stats.h"
#include "tilemap.h"
#include "tileset.h"
#include "utils.h"
#include "script.h"
#include "weapon.h"
#include "dungeonview.h"

GameController *game = nullptr;

/*-----------------*/
/* Functions BEGIN */
/* main game functions */
static std::time_t gameTimeSinceLastCommand(void);
static bool gameSave(void);

/* spell functions */
static void gameCastSpell(unsigned int spell, int caster, int param);
static bool gameSpellMixHowMany(int spell, int num, Ingredients *ingredients);
static void mixReagents();
static bool mixReagentsForSpellU4(int spell);
static bool mixReagentsForSpellU5(int spell);
#if 0
static void mixReagentsSuper();
#endif
static void newOrder();

/* conversation functions */
static bool talkAt(const Coords &coords, int distance);
static void talkRunConversation(
    Conversation &conv, Person *talker, bool showPrompt
);

/* action functions */
static bool attackAt(const Coords &coords);
static bool destroyAt(const Coords &coords);
static bool getChestTrapHandler(int player);
static bool jimmyAt(const Coords &coords);
static bool openAt(const Coords &coords);
static void wearArmor(int player = -1);
static void ztatsFor(int player = -1);

/* checking functions */
static void gameLordBritishCheckLevels(void);

/* creature functions */
void gameDestroyAllCreatures(void);
static void gameFixupObjects(Map *map);
static void gameCreatureAttack(Creature *obj);

/* Functions END */
/*---------------*/


// extern Object *party[8];

extern int quit;

const std::string tmpstr = "/tmp/";

Context *c = nullptr;
Debug gameDbg("debug/game.txt", "Game");
MouseArea mouseAreas[] = {
    {
        3,
        { { 8, 8 }, { 8, 184 }, { 96, 96 } },
        MC_WEST,
        { U4_ENTER, 0, U4_LEFT }
    },
    {
        3,
        { { 8, 8 }, { 184, 8 }, { 96, 96 } },
        MC_NORTH,
        { U4_ENTER, 0, U4_UP }
    },
    {
        3,
        { { 184, 8 }, { 184, 184 }, { 96, 96 } },
        MC_EAST,
        { U4_ENTER, 0, U4_RIGHT }
    },
    {
        3,
        { { 8, 184 }, { 184, 184 }, { 96, 96 } },
        MC_SOUTH,
        { U4_ENTER, 0, U4_DOWN }
    },
    {}
};

ReadPlayerController::ReadPlayerController()
    :ReadChoiceController("012345678 \033\n\r")
{
}

ReadPlayerController::~ReadPlayerController()
{
}

bool ReadPlayerController::keyPressed(int key)
{
    bool valid = ReadChoiceController::keyPressed(key);
    if (valid) {
        if (value == '0') {
            value = '9';
            return valid;
        }
        if ((value < '1') || (value > ('0' + c->saveGame->members))) {
            value = '0';
        }
    } else {
        value = '0';
    }
    return valid;
}

int ReadPlayerController::getPlayer()
{
    return value - '1';
}

int ReadPlayerController::waitFor()
{
    ReadChoiceController::waitFor();
    return getPlayer();
}

bool AlphaActionController::keyPressed(int key)
{
    if (xu4_islower(key)) {
        key = xu4_toupper(key);
    }
    if ((key >= 'A') && (key <= xu4_toupper(lastValidLetter))) {
        value = key - 'A';
        doneWaiting();
    } else if ((key == U4_SPACE) || (key == U4_ESC) || (key == U4_ENTER)) {
        screenMessage("\n");
        value = -1;
        doneWaiting();
    } else {
        screenMessage("\n%s", uppercase(prompt).c_str());
        screenRedrawScreen();
        return KeyHandler::defaultHandler(key, nullptr);
    }
    return true;
}

int AlphaActionController::get(
    char lastValidLetter, const std::string &prompt, EventHandler *eh
)
{
    if (!eh) {
        eh = eventHandler;
    }
    AlphaActionController ctrl(lastValidLetter, prompt);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

GameController::GameController()
    :mapArea(BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W, VIEWPORT_H),
     paused(false),
     pausedTimer(0)
{
}

GameController::~GameController()
{
    delete c;
}

void GameController::initScreen()
{
    Image *screen = imageMgr->get("screen")->image;
    screen->fillRect(0, 0, screen->width(), screen->height(), 0, 0, 0);
    screenRedrawScreen();
}

void GameController::initScreenWithoutReloadingState()
{
    musicMgr->play();
    imageMgr->get(BKGD_BORDERS)->image->draw(0, 0);
    c->stats->update(); /* draw the party stats */
    screenPrompt();
    eventHandler->pushMouseAreaSet(mouseAreas);
    eventHandler->setScreenUpdate(&gameUpdateScreen);
}

void GameController::init()
{
    std::FILE *saveGameFile, *monstersFile, *dngMapFile;
    TRACE(gameDbg, "gameInit() running.");
    initScreen();
#if 0
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, 4);
    pb.setBorderColor(255, 255, 255);
    pb.setBorderWidth(1);
    pb.setColor(0, 149, 255);
    screenTextAt(13, 11, "%s", "Lade Spiel...");
#endif
    /* initialize the global game context */
    c = new Context;
    c->saveGame = new SaveGame;
    TRACE_LOCAL(gameDbg, "Global context initialized.");
    /* initialize conversation and game state variables */
    c->line = TEXT_AREA_H - 1;
    c->col = 0;
    c->moonPhase = 0;
    c->windDirection = dirRandomDir(MASK_DIR_ALL);
    c->windLock = false;
    c->aura = new Aura();
    c->horseSpeed = 0;
    c->opacity = 1;
    c->lastCommandTime = std::time(nullptr);
    c->willPassTurn = false;
    c->lastShip = nullptr;
    /* load in the save game */
    // First the temporary just-inited save game...
    saveGameFile = std::fopen(
        (tmpstr + PARTY_SAV_BASE_FILENAME).c_str(), "rb"
    );
    if (!saveGameFile) {
        // ...and if that fails the real, main save game
        saveGameFile = std::fopen(
            (settings.getUserPath() + PARTY_SAV_BASE_FILENAME).c_str(), "rb"
        );
    }
    if (saveGameFile) {
        c->saveGame->read(saveGameFile);
        std::fclose(saveGameFile);
    } else {
        errorFatal("no savegame found!");
    }
    TRACE_LOCAL(gameDbg, "Save game loaded.");
#if 0
    ++pb;
#endif
    /* initialize our party */
    c->party = new Party(c->saveGame);
    c->party->addObserver(this);
    c->stats = new StatsArea();
    /* set the map to the world map by default */
    setMap(mapMgr->get(MAP_WORLD), 0, nullptr);
    c->location->map->clearObjects();
    TRACE_LOCAL(gameDbg, "World map set.");
#if 0
    ++pb;
#endif
    /* initialize our start location */
    Map *map = mapMgr->get(MapId(c->saveGame->location));
    TRACE_LOCAL(gameDbg, "Initializing start location.");
    /* if our map is not the world map, then load our map */
    ASSERT(
        map-> type == Map::WORLD || map->type == Map::DUNGEON,
        "Initial Map must be World or Dungeon map!"
    );
    if (map->type != Map::WORLD) {
        setMap(map, 1, nullptr);
        dngMapFile = std::fopen(
            (settings.getUserPath() + DNGMAP_SAV_BASE_FILENAME).c_str(), "rb"
        );
        if (dngMapFile) {
            Dungeon *dungeon = dynamic_cast<Dungeon *>(map);
            ASSERT(dungeon, "Map to Dungeon Conversion failed!");
            dungeon->tempData = dungeon->data;
            dungeon->data.clear();
            dungeon->tempDataSubTokens = dungeon->dataSubTokens;
            dungeon->dataSubTokens.clear();
            unsigned int i;
            int intMapData;
            unsigned char mapData;
            for (i = 0; i < (DNG_HEIGHT * DNG_WIDTH * dungeon->levels); i++) {
                intMapData = std::fgetc(dngMapFile);
                if (intMapData == EOF) {
                    std::fclose(dngMapFile);
                    errorFatal("DngMapFile read error!");
                }
                mapData = static_cast<unsigned char>(intMapData);
                switch (mapData & 0xF0) {
                case 0x80:
                case 0x90:
                case 0xA0:
                case 0xD0:
                    break;
                default:
                    mapData &= 0xF0; /* ignore monsters in dngmap.sav */
                }
                MapTile tile = map->tfrti(mapData);
                /* determine what type of tile it is */
                dungeon->data.push_back(tile);
                dungeon->dataSubTokens.push_back(mapData % 16);
            }
            std::fclose(dngMapFile);
        } // if(dngMapFile)
    } else {
        /* Enable opacity iff not flying (can fly only on the world map) */
        c->opacity = !c->saveGame->balloonstate;
    } // if(map->type != Map::WORLD)
    /* initialize the moons */
    initMoons();
    /**
     * Translate info from the savegame to something we can use
     */
    if (c->location->prev) {
        c->location->coords =
            MapCoords(c->saveGame->x, c->saveGame->y, c->saveGame->dnglevel);
        c->location->prev->coords =
            MapCoords(c->saveGame->dngx, c->saveGame->dngy);
    } else {
        c->location->coords = MapCoords(
            c->saveGame->x,
            c->saveGame->y,
            static_cast<int>(c->saveGame->dnglevel)
        );
    }
    c->saveGame->orientation =
        static_cast<Direction>(c->saveGame->orientation + DIR_WEST);
    /**
     * Fix the coordinates if they're out of bounds.  This happens every
     * time on the world map because (z == -1) is no longer valid.
     * To maintain compatibility with u4dos, this value gets translated
     * when the game is saved and loaded
     */
    if (MAP_IS_OOB(c->location->map, c->location->coords)) {
        c->location->coords.putInBounds(c->location->map);
    }
    TRACE_LOCAL(gameDbg, "Loading monsters.");
#if 0
    ++pb;
#endif
    /* load in monsters.sav */
    monstersFile = std::fopen(
        (tmpstr + MONSTERS_SAV_BASE_FILENAME).c_str(), "rb"
    );
    if (!monstersFile) {
        monstersFile = std::fopen(
            (settings.getUserPath() + MONSTERS_SAV_BASE_FILENAME).c_str(), "rb"
        );
    }
    if (monstersFile) {
        saveGameMonstersRead(c->location->map->monsterTable, monstersFile);
        std::fclose(monstersFile);
    }
    gameFixupObjects(c->location->map);
    /* we have previous creature information as well, load it! */
    if (c->location->prev) {
        monstersFile = std::fopen(
            (settings.getUserPath() + OUTMONST_SAV_BASE_FILENAME).c_str(), "rb"
        );
        if (monstersFile) {
            saveGameMonstersRead(
                c->location->prev->map->monsterTable, monstersFile
            );
            std::fclose(monstersFile);
        }
        gameFixupObjects(c->location->prev->map);
    }
    spellSetEffectCallback(&gameSpellEffect);
    itemSetDestroyAllCreaturesCallback(&gameDestroyAllCreatures);
#if 0
    ++pb;
#endif
    TRACE_LOCAL(gameDbg, "Settings up reagent menu.");
    c->stats->resetReagentsMenu();
    /* add some observers */
    c->aura->addObserver(c->stats);
    c->party->addObserver(c->stats);
    initScreenWithoutReloadingState();
    TRACE(gameDbg, "gameInit() completed successfully.");
} // GameController::init


/**
 * Saves the game state into party.sav and monsters.sav.
 */
static bool gameSave()
{
    std::FILE *saveGameFile, *monstersFile, *dngMapFile;
    SaveGame save = *c->saveGame;
    /*************************************************/
    /* Make sure the savegame struct is accurate now */
    if (c->location->prev) {
        save.x = c->location->coords.x;
        save.y = c->location->coords.y;
        save.dnglevel = c->location->coords.z;
        save.dngx = c->location->prev->coords.x;
        save.dngy = c->location->prev->coords.y;
    } else {
        save.x = c->location->coords.x;
        save.y = c->location->coords.y;
        save.dnglevel = c->location->coords.z;
        save.dngx = c->saveGame->dngx;
        save.dngy = c->saveGame->dngy;
    }
    save.location = c->location->map->id;
    save.orientation =
        static_cast<Direction>(c->saveGame->orientation - DIR_WEST);
    /* Done making sure the savegame struct is accurate */
    /****************************************************/
    saveGameFile = std::fopen(
        (settings.getUserPath() + PARTY_SAV_BASE_FILENAME).c_str(), "wb"
    );
    if (!saveGameFile) {
        screenMessage("Error opening " PARTY_SAV_BASE_FILENAME "\n");
        return false;
    }
    if (!save.write(saveGameFile)) {
        screenMessage("Error writing to " PARTY_SAV_BASE_FILENAME "\n");
        std::fflush(saveGameFile);
        fsync(fileno(saveGameFile));
        std::fclose(saveGameFile);
        sync();
        return false;
    }
    std::fflush(saveGameFile);
    fsync(fileno(saveGameFile));
    std::fclose(saveGameFile);
    sync();
    monstersFile = std::fopen(
        (settings.getUserPath() + MONSTERS_SAV_BASE_FILENAME).c_str(), "wb"
    );
    if (!monstersFile) {
        screenMessage("Error opening " MONSTERS_SAV_BASE_FILENAME "\n");
        return false;
    }
    /* fix creature animations so they are compatible with u4dos */
    c->location->map->resetObjectAnimations();
    /* fill the monster table so we can save it */
    c->location->map->fillMonsterTable();
    if (!saveGameMonstersWrite(c->location->map->monsterTable, monstersFile)) {
        screenMessage("Error writing to " MONSTERS_SAV_BASE_FILENAME "\n");
        std::fflush(monstersFile);
        fsync(fileno(monstersFile));
        std::fclose(monstersFile);
        sync();
        return false;
    }
    std::fflush(monstersFile);
    fsync(fileno(monstersFile));
    std::fclose(monstersFile);
    sync();
    /**
     * Write dungeon info
     */
    if (c->location->context & CTX_DUNGEON) {
        unsigned int x, y, z;
        typedef std::map<CreatureId, int> DngCreatureIdMap;
        static DngCreatureIdMap id_map;
        /**
         * Map creatures to u4dos dungeon creature Ids
         */
        if (id_map.size() == 0) {
            id_map[RAT_ID] = 1;
            id_map[BAT_ID] = 2;
            id_map[GIANT_SPIDER_ID] = 3;
            id_map[GHOST_ID] = 4;
            id_map[SLIME_ID] = 5;
            id_map[TROLL_ID] = 6;
            id_map[GREMLIN_ID] = 7;
            id_map[MIMIC_ID] = 8;
            id_map[REAPER_ID] = 9;
            id_map[INSECT_SWARM_ID] = 10;
            id_map[GAZER_ID] = 11;
            id_map[PHANTOM_ID] = 12;
            id_map[ORC_ID] = 13;
            id_map[SKELETON_ID] = 14;
            id_map[ROGUE_ID] = 15;
        }
        dngMapFile = std::fopen(
            (settings.getUserPath() + DNGMAP_SAV_BASE_FILENAME).c_str(), "wb"
        );
        if (!dngMapFile) {
            screenMessage("Error opening " DNGMAP_SAV_BASE_FILENAME "\n");
            return false;
        }
        for (z = 0; z < c->location->map->levels; z++) {
            for (y = 0; y < c->location->map->height; y++) {
                for (x = 0; x < c->location->map->width; x++) {
                    unsigned char tile = c->location->map->ttrti(
                        *c->location->map->tileAt(
                            MapCoords(x, y, z), WITHOUT_OBJECTS
                        )
                    );

                    /**
                     * Add the creature to the tile if tile w/o subtokens
                     */
                    switch(tile & 0xF0) {
                    case 0x80: // Traps
                    case 0x90: // Fountains
                    case 0xA0: // Energy Fields
                    case 0xD0: // Rooms
                        //FIXME: What does U4DOS do in this case?
                        //does it prevent all creatures from stepping there?
                        break; //For now, just ignore creature
                    default: // Other tile, has no subtokens, can add creature
                        Object *obj =
                            c->location->map->objectAt(MapCoords(x, y, z));
                        if (obj && (obj->getType() == Object::CREATURE)) {
                            const Creature *m = dynamic_cast<Creature *>(obj);
                            DngCreatureIdMap::iterator m_id =
                                id_map.find(m->getId());
                            if (m_id != id_map.end()) {
                                tile |= m_id->second;
                            }
                        }
                        if (obj && (obj->getTile().getTileType()->isChest())) {
                            tile = (tile & 0x0f) | DUNGEON_CHEST;
                        }
                    }
                    // Write the tile
                    if (std::fputc(tile, dngMapFile) == EOF) {
                        screenMessage(
                            "Error writing to " DNGMAP_SAV_BASE_FILENAME "\n"
                        );
                        std::fflush(dngMapFile);
                        fsync(fileno(dngMapFile));
                        std::fclose(dngMapFile);
                        sync();
                        return false;
                    }
                }
            }
        }
        std::fflush(dngMapFile);
        fsync(fileno(dngMapFile));
        std::fclose(dngMapFile);
        sync();
        /**
         * Write outmonst.sav
         */
        monstersFile = std::fopen(
            (settings.getUserPath() + OUTMONST_SAV_BASE_FILENAME).c_str(), "wb"
        );
        if (!monstersFile) {
            screenMessage("Error opening " OUTMONST_SAV_BASE_FILENAME "\n");
            return false;
        }
        /* fix creature animations so they are compatible with u4dos */
        c->location->prev->map->resetObjectAnimations();
        /* fill the monster table so we can save it */
        c->location->prev->map->fillMonsterTable();
        if (!saveGameMonstersWrite(
                c->location->prev->map->monsterTable, monstersFile
            )) {
            screenMessage("Error writing to " OUTMONST_SAV_BASE_FILENAME "\n");
            std::fflush(monstersFile);
            fsync(fileno(monstersFile));
            std::fclose(monstersFile);
            sync();
            return false;
        }
        std::fflush(monstersFile);
        fsync(fileno(monstersFile));
        std::fclose(monstersFile);
        sync();
    }
    return true;
} // gameSave


/**
 * Sets the view mode.
 */
void gameSetViewMode(ViewMode newMode)
{
    c->location->viewMode = newMode;
}

void gameUpdateScreen()
{
    switch (c->location->viewMode) {
    case VIEW_NORMAL:
        screenUpdate(&game->mapArea, true, false);
        break;
    case VIEW_GEM:
        screenGemUpdate();
        break;
    case VIEW_RUNE:
        screenUpdate(&game->mapArea, false, false);
        break;
    case VIEW_DUNGEON:
        screenUpdate(&game->mapArea, true, false);
        break;
    case VIEW_DEAD:
        screenUpdate(&game->mapArea, true, true);
        break;
    case VIEW_CODEX:
        screenUpdate(&game->mapArea, false, false);
        break;
    case VIEW_MIXTURES:
        /* still testing */
        break;
    default:
        ASSERT(0, "invalid view mode: %d", c->location->viewMode);
    }
}

void GameController::setMap(
    Map *map,
    bool saveLocation,
    const Portal *portal,
    TurnCompleter *turnCompleter
)
{
    int viewMode;
    LocationContext context;
    int activePlayer = c->party->getActivePlayer();
    MapCoords coords;
    if (!turnCompleter) {
        turnCompleter = this;
    }
    if (portal) {
        coords = portal->start;
    } else {
        coords = MapCoords(map->width / 2, map->height / 2);
    }
    /* If we don't want to save the location, then just return to the
     *  previous location, as there may still be ones in the stack we
     * want to keep */
    if (!saveLocation) {
        // don't quench torch if going through altar room into other dungeon
        bool shouldQuenchTorch = (map->type != Map::DUNGEON);
        exitToParentMap(shouldQuenchTorch);
    }
    switch (map->type) {
    case Map::WORLD:
        context = CTX_WORLDMAP;
        viewMode = VIEW_NORMAL;
        break;
    case Map::DUNGEON:
        context = CTX_DUNGEON;
        viewMode = VIEW_DUNGEON;
        if (portal) {
            c->saveGame->orientation = DIR_EAST;
        }
        break;
    case Map::COMBAT:
        /* set these to -1 just to be safe; we don't need them */
        coords = MapCoords(-1, -1);
        context = CTX_COMBAT;
        viewMode = VIEW_NORMAL;
        /* different active player for combat, defaults to 'None' */
        activePlayer = -1;
        break;
    case Map::SHRINE:
        context = CTX_SHRINE;
        viewMode = VIEW_NORMAL;
        break;
    case Map::CITY:
    default:
        context = CTX_CITY;
        viewMode = VIEW_NORMAL;
        break;
    } // switch
    c->location = new Location(
        coords, map, viewMode, context, turnCompleter, c->location
    );
    c->location->addObserver(this);
    c->party->setActivePlayer(activePlayer);
    /* now, actually set our new tileset */
    mapArea.setTileset(map->tileset);
    /* add inhabitants */
    City *city = dynamic_cast<City *>(map);
    if (city) city->addPeople();
} // GameController::setMap


/**
 * Exits the current map and location and returns to its parent location
 * This restores all relevant information from the previous location,
 * such as the map, map position, etc. (such as exiting a city)
 **/
bool GameController::exitToParentMap(bool shouldQuenchTorch)
{
    if (!c->location) {
        return false;
    }
    if (c->location->prev != nullptr) {
        // Create the balloon for Hythloth
        if (c->location->map->id == MAP_HYTHLOTH) {
            createBalloon(c->location->prev->map);
        }
        // free map info only if previous location was on a different map
        if (c->location->prev->map != c->location->map) {
            c->location->map->annotations->clear();
            c->location->map->clearObjects();
            Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
            if (dungeon && (dungeon->tempData.size() > 0)) {
                dungeon->data = dungeon->tempData;
                dungeon->tempData.clear();
                dungeon->dataSubTokens = dungeon->tempDataSubTokens;
                dungeon->tempDataSubTokens.clear();
            }
            /* quench the torch if we're on the world map */
            if (c->location->prev->map->isWorldMap() && shouldQuenchTorch) {
                c->party->quenchTorch();
            }
        }
        locationFree(&c->location);
        // restore the tileset to the one the current map uses
        mapArea.setTileset(c->location->map->tileset);
        return true;
    }
    return false;
} // GameController::exitToParentMap

/**
 * Terminates a game turn.  This performs the post-turn housekeeping
 * tasks like adjusting the party's food, incrementing the number of
 * moves, etc.
 */
void GameController::finishTurn()
{
    extern std::atomic_bool deathSequenceRunning;
    if (deathSequenceRunning) {
        /* none of this makes sense if party is already dead */
        return;
    }
    c->lastCommandTime = std::time(nullptr);
    Creature *attacker = nullptr;
    while (1) {
        /* adjust food and moves */
        c->party->endTurn();
        /* count down the aura, if there is one */
        c->aura->passTurn();
        gameCheckHullIntegrity();
        /* update party stats */
        c->stats->setView(STATS_PARTY_OVERVIEW);
        screenUpdate(&this->mapArea, true, false);
        screenWait(1);
        /* Creatures cannot spawn, move or attack while the avatar is
           on the balloon */
        if (!c->party->isFlying()) {
            // apply effects from tile avatar is standing on
            c->party->applyEffect(
                c->location->map->tileTypeAt(
                    c->location->coords, WITH_GROUND_OBJECTS
                )->getEffect()
            );
            // Move creatures and see if something is attacking the avatar
            attacker = c->location->map->moveObjects(c->location->coords);
            // Something's attacking!  Start combat!
            if (attacker) {
                gameCreatureAttack(attacker);
                return;
            }
            // cleanup old creatures and spawn new ones
            creatureCleanup();
            checkRandomCreatures();
            checkBridgeTrolls();
        }
        /* update map annotations */
        c->location->map->annotations->passTurn();
        if (!c->party->isImmobilized()) {
            break;
        }
        if (c->party->isDead()) {
            deathStart(0);
            return;
        } else {
            Controller *controller = eventHandler->getController();
            if ((controller != nullptr)
                && ((eventHandler->getController() == game)
                    || (dynamic_cast<CombatController *>(
                            eventHandler->getController()
                        ) != nullptr))) {
                /* pass the turn, and redraw the text area so the prompt
                   is shown */
                screenPrompt();
                screenWait(4);
                controller->keyPressed(26);
                screenRedrawTextArea(
                    TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H
                );
                break;
            }
            /* screenMessage("\n\020Zzzzz\n"); */
        }
    }
    if (c->location->context == CTX_DUNGEON) {
        Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
        if (c->party->getTorchDuration() <= 0) {
            screenMessage("ES IST DUNKEL!\n");
        } else {
            c->party->burnTorch();
        }
        /* handle dungeon traps */
        if (dungeon->currentToken() == DUNGEON_TRAP) {
            dungeonHandleTrap(
                static_cast<TrapType>(dungeon->currentSubToken())
            );
            // a little kludgey to have a second test for this
            // right here.  But without it you can survive an
            // extra turn after party death and do some things
            // that could cause a crash, like Hole up and Camp.
            if (c->party->isDead()) {
                deathStart(0);
                return;
            }
        }
    }
    /* draw a prompt */
    screenPrompt();
#if 0
    screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);
#endif
} // GameController::finishTurn


/**
 * Show an attack flash at x, y on the current map.
 * This is used for 'being hit' or 'being missed'
 * by weapons, cannon fire, spells, etc.
 */
void GameController::flashTile(const Coords &coords, MapTile tile, int frames)
{
    c->location->map->annotations->add(coords, tile, true);
    screenTileUpdate(&game->mapArea, coords);
    screenWait(frames);
    c->location->map->annotations->remove(coords, tile);
    screenTileUpdate(&game->mapArea, coords, false);
}

void GameController::flashTile(
    const Coords &coords, const std::string &tilename, int timeFactor
)
{
    Tile *tile = c->location->map->tileset->getByName(tilename);
    ASSERT(tile, "no tile named '%s' found in tileset", tilename.c_str());
    flashTile(coords, tile->getId(), timeFactor);
}

/**
 * Provide feedback to user after a party event happens.
 */
void GameController::update(Party *, PartyEvent &event)
{
    int i;
    switch (event.type) {
    case PartyEvent::LOST_EIGHTH:
        // inform a player he has lost zero or more eighths of avatarhood.
        screenMessage(
            "\n %cDU VERLIERST\n  EIN ACHTEL!%c\n", FG_YELLOW, FG_WHITE
        );
        break;
    case PartyEvent::ADVANCED_LEVEL:
        screenMessage(
            "%c%s, DU BIST JETZT STUFE %d.%c\n",
            FG_YELLOW,
            uppercase(event.player->getName()).c_str(),
            event.player->getRealLevel(),
            FG_WHITE
        );
        gameSpellEffect('r', -1, SOUND_MAGIC); // Same as resurrect spell
        break;
    case PartyEvent::STARVING:
        screenMessage("\n%cHUNGERN!!!%c\n", FG_YELLOW, FG_WHITE);
        soundPlay(SOUND_PC_STRUCK, false);
        // 2 damage to each party member for starving!
        for (i = 0; i < c->saveGame->members; i++) {
            c->party->member(i)->applyDamage(2);
        }
        break;
    default:
        break;
    }
} // GameController::update


/**
 * Provide feedback to user after a movement event happens.
 */
void GameController::update(Location *location, MoveEvent &event)
{
    switch (location->map->type) {
    case Map::DUNGEON:
        avatarMovedInDungeon(event);
        break;
    case Map::COMBAT:
        // FIXME: let the combat controller handle it
        dynamic_cast<CombatController *>(eventHandler->getController())
            ->movePartyMember(event);
        break;
    default:
        avatarMoved(event);
        break;
    }
}

void gameSpellEffect(int spell, int player, Sound sound)
{
    int time;
    Spell::SpecialEffects effect = Spell::SFX_INVERT;
    game->paused = true;
    game->pausedTimer = 0;
    if (player >= 0) {
        c->stats->highlightPlayer(player);
    }
    time = settings.spellEffectSpeed * 800 / settings.gameCyclesPerSecond;
    time -= time % 10;
    soundPlay(sound, false, time);
    ///The following effect multipliers are not accurate
    if (spell == 't') {
        effect = Spell::SFX_TREMOR;
    }
    switch (effect) {
    case Spell::SFX_NONE:
        break;
    case Spell::SFX_TREMOR:
    case Spell::SFX_INVERT:
        gameUpdateScreen();
        game->mapArea.highlight(
            0, 0, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT
        );
        EventHandler::sleep(time);
        game->mapArea.unhighlight();
        screenUpdate(&game->mapArea, true, false);
        screenRedrawScreen();
        if (effect == Spell::SFX_TREMOR) {
            gameUpdateScreen();
            soundPlay(SOUND_RUMBLE, false);
            screenShake(8);
        }
        break;
    }
    game->paused = false;
} // gameSpellEffect

static void gameCastSpell(unsigned int spell, int caster, int param)
{
    SpellCastError spellError;
    std::string msg;
    if (!spellCast(spell, caster, param, &spellError, true)) {
        msg = spellGetErrorMessage(spell, spellError);
        if (!msg.empty()) {
            soundPlay(SOUND_FLEE);
            screenMessage("%s", msg.c_str());
        }
    }
}


/**
 * The main key handler for the game.  Interpretes each key as a
 * command - 'a' for attack, 't' for talk, etc.
 */
bool GameController::keyPressed(int key)
{
    bool valid = true;
    bool endTurn = true;
    Object *obj;
    MapTile *tile;
    if ((key >= 'A') && (key <= ']')) {
        key = xu4_tolower(key);
    }

    /* Translate context-sensitive action key into a useful command */
    if ((key == U4_ENTER)
        && settings.enhancements
        && settings.enhancementsOptions.smartEnterKey) {
        /* Attempt to guess based on the character's surroundings etc, what
         * action they want */
        /* Do they want to board something? */
        if (c->transportContext == TRANSPORT_FOOT) {
            obj = c->location->map->objectAt(c->location->coords);
            if (obj &&
                (obj->getTile().getTileType()->isShip()
                 || obj->getTile().getTileType()->isHorse()
                 || obj->getTile().getTileType()->isBalloon())) {
                key = 'l';
            }
        }
        /* Klimb/Descend Balloon */
        else if (c->transportContext == TRANSPORT_BALLOON) {
            if (c->party->isFlying()) {
                key = 'y';
            } else {
                key = 'q';
            }
        }
        /* X-it transport */
        else {
            key = 'g';
        }
        /* Klimb? */
        if ((c->location->map->portalAt(c->location->coords, ACTION_KLIMB)
             != nullptr)) {
            key = 'q';
        }
        /* Descend? */
        else if ((c->location->map->portalAt(
                      c->location->coords, ACTION_DESCEND
                  ) != nullptr)) {
            key = 'y';
        }
        if (c->location->context == CTX_DUNGEON) {
            Dungeon *dungeon = static_cast<Dungeon *>(c->location->map);
            bool up = dungeon->ladderUpAt(c->location->coords);
            bool down = dungeon->ladderDownAt(c->location->coords);
            if (up && down) {
                // On double ladder, prefer "up".
                // This is consistent with the previous code.
                // Ideally, I would have a UI here as well.
                key = 'q';
            } else if (up) {
                key = 'q';
            } else {
                key = 'y';
            }
        }
        /* Enter? */
        if (c->location->map->portalAt(c->location->coords, ACTION_ENTER)
            != nullptr) {
            key = 'b';
        }
        /* Get Chest? */
        if (!c->party->isFlying()) {
            tile = c->location->map->tileAt(
                c->location->coords, WITH_GROUND_OBJECTS
            );
            if (tile->getTileType()->isChest()) {
                key = 't';
            }
        }
        /* None of these? Default to search */
        if (key == U4_ENTER) {
            key = 'n';
        }
    }
    if ((c->location->context & CTX_DUNGEON)
        && std::strchr("albkdp|usgh", key)) {
        soundPlay(SOUND_ERROR);
        screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
    } else {
        switch (key) {
        case U4_UP:
        case U4_DOWN:
        case U4_LEFT:
        case U4_RIGHT:
        {
            /* move the avatar */
            std::string previous_map = c->location->map->fname;
            MoveResult retval = c->location->move(keyToDirection(key), true);
            /* horse doubles speed (make sure we're on the same map
               as the previous move first) */
            if (retval & (MOVE_SUCCEEDED | MOVE_SLOWED)
                && (c->transportContext == TRANSPORT_HORSE)
                && c->horseSpeed) {
                gameUpdateScreen(); /* to give it a smooth look of movement */
                if (previous_map == c->location->map->fname) {
                    c->location->move(keyToDirection(key), false);
                }
            }
            /* let the movement handler decide to end the turn */
            endTurn = !!(retval & MOVE_END_TURN);
            break;
        }
        case U4_FKEY:
        case U4_FKEY + 1:
        case U4_FKEY + 2:
        case U4_FKEY + 3:
        case U4_FKEY + 4:
        case U4_FKEY + 5:
        case U4_FKEY + 6:
        case U4_FKEY + 7:
            /* teleport to dungeon entrances! */
            if (settings.debug
                && (c->location->context & CTX_WORLDMAP)
                && (c->transportContext & TRANSPORT_FOOT_OR_HORSE)) {
                int portal = 16 + (key - U4_FKEY); /* find dungeon portal */
                c->location->coords =
                    c->location->map->portals[portal]->coords;
            } else {
                valid = false;
            }
            break;
        case U4_FKEY + 8:
            /* the altar room of truth */
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(mapMgr->get(MAP_DECEIT), 1, nullptr);
                c->location->coords = MapCoords(1, 0, 7);
                c->saveGame->orientation = DIR_SOUTH;
            } else {
                valid = false;
            }
            break;
        case U4_FKEY + 9:
            /* the altar room of love */
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(mapMgr->get(MAP_DESPISE), 1, nullptr);
                c->location->coords = MapCoords(3, 2, 7);
                c->saveGame->orientation = DIR_SOUTH;
            } else {
                valid = false;
            }
            break;
        case U4_FKEY + 10:
            /* the altar room of courage */
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(mapMgr->get(MAP_DESTARD), 1, nullptr);
                c->location->coords = MapCoords(7, 6, 7);
                c->saveGame->orientation = DIR_SOUTH;
            } else {
                valid = false;
            }
            break;
        case U4_FKEY + 11:
            /* show torch duration */
            if (settings.debug) {
                screenMessage("Torch: %d\n", c->party->getTorchDuration());
                screenPrompt();
            } else {
                valid = false;
            }
            break;
        case 3:
            /* ctrl-C */
            if (settings.debug) {
                screenMessage("Cmd (h = help):");
                CheatMenuController cheatMenuController(this);
                eventHandler->pushController(&cheatMenuController);
                cheatMenuController.waitFor();
            } else {
                valid = false;
            }
            break;
        case 4:
            /* ctrl-D */
            if (settings.debug) {
                destroy();
            } else {
                valid = false;
            }
            break;
        case 8:
            /* ctrl-H */
            if (settings.debug) {
                screenMessage("Help!\n");
                screenPrompt();
                /* Help! send me to Lord British (who conveniently is
                   right around where you are)! */
                setMap(mapMgr->get(100), 1, nullptr);
                c->location->coords.x = 19;
                c->location->coords.y = 8;
                c->location->coords.z = 0;
            } else {
                valid = false;
            }
            break;
        case 22:
            /* ctrl-V */
            if (settings.debug && (c->location->context == CTX_DUNGEON)) {
                screenMessage(
                    "3-D view %s\n",
                    DungeonViewer.toggle3DDungeonView() ? "on" : "off"
                );
                endTurn = false;
            } else {
                valid = false;
            }
            break;
        case ' ':
            screenMessage("Aussetzen\n");
            break;
        case 26: // Game sends this when party asleep -> no settings.debug mask
            /* Ctrl-Z */
            screenMessage("Zzzzz\n");
            break;
        case '+':
        case '-':
        case U4_KEYPAD_ENTER:
            if (settings.debug) {
                int old_cycles = settings.gameCyclesPerSecond;
                if ((key == '+')
                    && (++settings.gameCyclesPerSecond
                        >= MAX_CYCLES_PER_SECOND)) {
                    settings.gameCyclesPerSecond = MAX_CYCLES_PER_SECOND;
                } else if (
                    (key == '-')
                    && (--settings.gameCyclesPerSecond
                        <= MIN_CYCLES_PER_SECOND)
                ) {
                    settings.gameCyclesPerSecond = MIN_CYCLES_PER_SECOND;
                } else if (key == U4_KEYPAD_ENTER) {
                    settings.gameCyclesPerSecond = DEFAULT_CYCLES_PER_SECOND;
                }
                if (old_cycles != settings.gameCyclesPerSecond) {
                    eventTimerGranularity =
                        (1000 / settings.gameCyclesPerSecond);
                    eventHandler->getTimer()->reset(eventTimerGranularity);
                    if (settings.gameCyclesPerSecond
                        == DEFAULT_CYCLES_PER_SECOND) {
                        screenMessage("Speed: Normal\n");
                    } else if (key == '+') {
                        screenMessage(
                            "Speed Up (%d)\n", settings.gameCyclesPerSecond
                        );
                    } else {
                        screenMessage(
                            "Speed Down (%d)\n", settings.gameCyclesPerSecond
                        );
                    }
                } else if (settings.gameCyclesPerSecond
                           == DEFAULT_CYCLES_PER_SECOND) {
                    screenMessage("Speed: Normal\n");
                }
                endTurn = false;
            } else {
                valid = false;
            }
            break;
        /* handle music volume adjustments */
        case ',':
            if (settings.debug) {
                // decrease the volume if possible
                screenMessage(
                    "Music: %d%s\n", musicMgr->decreaseMusicVolume(), "%"
                );
                endTurn = false;
            } else {
                valid = false;
            }
            break;
        case '.':
            if (settings.debug) {
                // increase the volume if possible
                screenMessage(
                    "Music: %d%s\n", musicMgr->increaseMusicVolume(), "%"
                );
                endTurn = false;
            } else {
                valid = false;
            }
            break;
            /* handle sound volume adjustments */
        case '<':
            if (settings.debug) {
                // decrease the volume if possible
                screenMessage(
                    "Sound: %d%s\n", musicMgr->decreaseSoundVolume(), "%"
                );
                soundPlay(SOUND_FLEE);
                endTurn = false;
            } else {
                valid = false;
            }
            break;
        case '>':
            if (settings.debug) {
                // increase the volume if possible
                screenMessage(
                    "Sound: %d%s\n", musicMgr->increaseSoundVolume(), "%"
                );
                soundPlay(SOUND_FLEE);
                endTurn = false;
            } else {
                valid = false;
            }
            break;
        case 'a':
            attack();
            break;
        case 'l':
            board();
            break;
        case 'z':
            castSpell();
            break;
        case 'y':
        {
            // unload the map for the second level of Lord British's Castle.
            // The reason why is that Lord British's farewell is dependent
            // on the number of party members. Instead of just redoing the
            // dialog, it's a bit severe, but easier to unload the whole level.
            bool cleanMap =
                (c->party->size() == 1 && c->location->map->id == 100);
            if (!usePortalAt(
                    c->location, c->location->coords, ACTION_DESCEND
                )) {
                if (c->transportContext == TRANSPORT_BALLOON) {
                    screenMessage("Ballon Landen\n");
                    if (!c->party->isFlying()) {
                        soundPlay(SOUND_ERROR);
                        screenMessage(
                            "%cSCHON GELANDET!%c\n", FG_GREY, FG_WHITE
                        );
                    } else if (c->location->map->tileTypeAt(
                                   c->location->coords, WITH_OBJECTS
                               )->canLandBalloon()) {
                        c->saveGame->balloonstate = 0;
                        c->opacity = 1;
                    } else {
                        soundPlay(SOUND_ERROR);
                        screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
                    }
                } else {
                    soundPlay(SOUND_ERROR);
                    screenMessage("%cAbw{rts WOHIN?%c\n", FG_GREY, FG_WHITE);
                }
            } else if (cleanMap) {
                mapMgr->unloadMap(100);
            }
            break;
        }
        case 'b':
            if (!usePortalAt(c->location, c->location->coords, ACTION_ENTER)) {
                if (!c->location->map->portalAt(
                        c->location->coords, ACTION_ENTER
                    )) {
                    soundPlay(SOUND_ERROR);
                    screenMessage("%cBetreten - WAS?%c\n", FG_GREY, FG_WHITE);
                }
            } else {
                endTurn = false; /* entering a portal doesn't end the turn */
            }
            break;
        case 'k':
            fire();
            break;
        case 't':
            getChest();
            break;
        case 'c':
            holeUp();
            break;
        case 'f':
            screenMessage("Fackel z}nden\n");
            if (c->location->context == CTX_DUNGEON) {
                if (!c->party->lightTorch()) {
                    soundPlay(SOUND_ERROR);
                    screenMessage("%cKEINE ]BRIG!%c\n", FG_GREY, FG_WHITE);
                }
            } else {
                soundPlay(SOUND_ERROR);
                screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
            }
            break;
        case 'd':
            jimmy();
            break;
        case 'q':
            if (!usePortalAt(c->location, c->location->coords, ACTION_KLIMB)) {
                if (c->transportContext == TRANSPORT_BALLOON) {
                    c->saveGame->balloonstate = 1;
                    c->opacity = 0;
                    screenMessage("Aufsteigen\n");
                } else {
                    soundPlay(SOUND_ERROR);
                    screenMessage("%cAufw{rts WOHIN?%c\n", FG_GREY, FG_WHITE);
                }
            }
            break;
        case 'p':
            /* can't use sextant in dungeon or in combat */
            if (c->location->context & ~(CTX_DUNGEON | CTX_COMBAT)) {
                if (c->saveGame->sextants >= 1) {
                    screenMessage(
                        "Position\nmit Sextant\n"
                        "Breite: %c'%c\"\nL{nge : %c'%c\"\n",
                        c->location->coords.y / 16 + 'A',
                        c->location->coords.y % 16 + 'A',
                        c->location->coords.x / 16 + 'A',
                        c->location->coords.x % 16 + 'A'
                    );
                } else {
                    soundPlay(SOUND_ERROR);
                    screenMessage("%cPosition WOMIT?%c\n", FG_GREY, FG_WHITE);
                }
            } else {
                soundPlay(SOUND_ERROR);
                screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
            }
            break;
        case 'm':
            mixReagents();
            break;
        case 'o':
            newOrder();
            break;
        case '|':
        case 'u':
            opendoor();
            break;
        case 'j':
            peer();
            break;
        case 'e':
            screenMessage("Ende&Speichern\n%d Z]GE...\n", c->saveGame->moves);
            if (c->location->context & CTX_CAN_SAVE_GAME) {
                gameSave();
                EventHandler::simulateDiskLoad(2000);
                screenMessage("\nGESPEICHERT!\n\nReise beenden?");
                int c = ReadChoiceController::get("jn\015 \033");
                if (c == 'j') {
                    quit = 1;
                    EventHandler::end();
                }
                screenMessage("\n");
            } else {
                soundPlay(SOUND_ERROR);
                screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
            }
            break;
        case 'w':
            readyWeapon();
            break;
        case 'n':
            if (c->location->context == CTX_DUNGEON) {
                dungeonSearch();
            } else if (c->party->isFlying()) {
                soundPlay(SOUND_ERROR);
                screenMessage(
                    "Nachschauen...\n%cNUR DRIFT!%c\n", FG_GREY, FG_WHITE
                );
            } else {
                screenMessage("Nachschauen...\n");
                EventHandler::simulateDiskLoad(2000);
                const ItemLocation *item = itemAtLocation(
                    c->location->map, c->location->coords
                );
                if (item) {
                    if ((*item->isItemInInventory != nullptr)
                        && (*item->isItemInInventory)(item->data)) {
                        screenMessage(
                            "%cHIER IST NICHTS!%c\n", FG_GREY, FG_WHITE
                        );
                    } else {
                        if (item->name) {
                            screenMessage(
                                "DU FINDEST...\n%s!\n",
                                uppercase(item->name).c_str()
                            );
                        }
                        (*item->putItemInInventory)(item->data);
                    }
                } else {
                    screenMessage("%cHIER IST NICHTS!%c\n", FG_GREY, FG_WHITE);
                }
            }
            break;
        case 's':
            talk();
            break;
        case 'v':
            c->willPassTurn = false;
            screenMessage("Verwenden...\n");
            EventHandler::simulateDiskLoad(2000);
            screenMessage("WELCHES DING:\n?");
            if (settings.enhancements) {
                /* a little xu4 enhancement: show items in inventory when
                   prompted for an item to use */
                c->stats->setView(STATS_ITEMS);
            }
            itemUse(gameGetInput().c_str());
            if (c->location->viewMode == VIEW_CODEX) {
                break;
            }
            if (settings.enhancements) {
                c->stats->setView(STATS_PARTY_OVERVIEW);
            }
            c->lastCommandTime = std::time(nullptr);
            c->willPassTurn = true;
            break;
        case 'x':
            if (musicMgr->toggle()) {
                screenMessage("Xound EIN\n");
            } else {
                screenMessage("Xound AUS\n");
            }
            endTurn = false;
            break;
        case 'r':
            wearArmor();
            break;
        case 'g':
            if ((c->transportContext != TRANSPORT_FOOT)
                && !c->party->isFlying()) {
                Object *obj = c->location->map->addObject(
                    c->party->getTransport(),
                    c->party->getTransport(),
                    c->location->coords
                );
                if (c->transportContext == TRANSPORT_SHIP) {
                    c->lastShip = obj;
                }
                Tile *avatar = c->location->map->tileset->getByName("avatar");
                ASSERT(avatar, "no avatar tile found in tileset");
                c->party->setTransport(avatar->getId());
                c->horseSpeed = 0;
                screenMessage("Zu Fu~ Gehen\n");
            } else {
                soundPlay(SOUND_ERROR);
                screenMessage(
                    "Zu Fu~ Gehen\n%cHIER NICHT!%c\n", FG_GREY, FG_WHITE
                );
            }
            break;
        case 'h':
            if (c->transportContext == TRANSPORT_HORSE) {
                if (c->horseSpeed == 0) {
                    screenMessage("\"H}hott!\"\n");
                    c->horseSpeed = 1;
                } else {
                    screenMessage("\"Brrr!\"\n");
                    c->horseSpeed = 0;
                }
            } else {
                soundPlay(SOUND_ERROR);
                screenMessage(
                    "\"H}hott!\"\n%cHIER NICHT!%c\n", FG_GREY, FG_WHITE
                );
            }
            break;
        case 'i':
            ztatsFor();
            break;
        case 'c' + U4_ALT:
            if (settings.debug && c->location->map->isWorldMap()) {
                /* first teleport to the abyss */
                c->location->coords.x = 0xe9;
                c->location->coords.y = 0xe9;
                setMap(mapMgr->get(MAP_ABYSS), 1, nullptr);
                /* then to the final altar */
                c->location->coords.x = 7;
                c->location->coords.y = 7;
                c->location->coords.z = 7;
            }
            break;
        case 'h' + U4_ALT:
        {
            ReadChoiceController pauseController("");
            screenMessage(
                "Tastenreferenz:\n"
                "Pfeile: Gehen\n"
                "A: Angreifen\n"
                "B: Betreten\n"
                "C: Campieren\n"
                "D: Dietrich\n"
                "E: Ende\n"
                "F: Fackel\n"
                "G: Zu Fu~ Gehen\n"
                "H: H}hott/Brrr\n"
                "I: Info\n"
                "(mehr)"
            );
            eventHandler->pushController(&pauseController);
            pauseController.waitFor();
            screenMessage(
                "\n"
                "J: Juwel\n"
                "K: Kanone\n"
                "L: Losfahren\n"
                "M: Mischen\n"
                "N: Nachschauen\n"
                "O: Ordnung\n"
                "\\: \\ffnen\n"
                "P: Position\n"
                "Q: Aufw{rts\n"
                "R: R}stung\n"
                "S: Sprechen\n"
                "(more)"
            );
            eventHandler->pushController(&pauseController);
            pauseController.waitFor();
            screenMessage(
                "\n"
                "T: Truhe\n"
                "V: Verwenden\n"
                "W: Waffe\n"
                "X: Xound an/aus\n"
                "Y: Abw{rts\n"
                "Z: Zaubern\n"
                "Leer: Aussetzen\n"
                ",: - Musik Vol\n"
                ".: + Musik Vol\n"
                "<: - Sound Vol\n"
                ">: + Sound Vol\n"
                "(more)"
            );
            eventHandler->pushController(&pauseController);
            pauseController.waitFor();
            screenMessage(
                "\n"
                "Alt-Q: Hauptmen}\n"
                "Alt-V: Version\n"
                "Alt-X: Abbruch\n"
                "\n"
                "\n"
                "\n"
                "\n"
                "\n"
                "\n"
                "\n"
                "\n"
            );
            screenPrompt();
            break;
        }
        case 'q' + U4_ALT:
        {
            // TODO - implement loop in main() and let quit fall back to there
            // Quit to the main menu
            endTurn = false;
            screenMessage("ZUR]CK INS MEN]?");
            char choice = ReadChoiceController::get("jn \n\033");
            // screenMessage("%c", choice);
            if (choice != 'j') {
                screenMessage("\n");
                break;
            }
            eventHandler->setScreenUpdate(nullptr);
            eventHandler->popController();
            eventHandler->pushController(intro);
            // Stop the music and hide the cursor
            // before returning to the menu.
            musicMgr->pause();
            screenHideCursor();
            intro->init();
            eventHandler->run();
            if (!quit) {
                eventHandler->setControllerDone(false);
                eventHandler->popController();
                eventHandler->pushController(this);
                if (intro->hasInitiatedNewGame()) {
                    // Loads current savegame
                    init();
                } else {
                    // Inits screen stuff without renewing game
                    initScreen();
                    initScreenWithoutReloadingState();
                }
                this->mapArea.reinit();
                intro->deleteIntro();
                eventHandler->run();
            }
            break;
        }
        case 'v' + U4_ALT:
            screenMessage("%s\n", VERSION);
            endTurn = false;
            break;
            // Turn sound effects on/off
        case 's' + U4_ALT:
            // FIXME: there's probably a more intuitive key combination
            // for this
            settings.soundVol = !settings.soundVol;
            screenMessage("Sounds %s!\n", settings.soundVol ? "EIN" : "AUS");
            endTurn = false;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (settings.enhancements
                && settings.enhancementsOptions.activePlayer) {
                gameSetActivePlayer(key - '1');
            } else {
                soundPlay(SOUND_ERROR);
                screenMessage("%cWAS?%c\n", FG_GREY, FG_WHITE);
            }
            endTurn = false;
            break;
        default:
            valid = false;
            break;
        } // switch
    } // switch
    if (valid && endTurn) {
        if (eventHandler->getController() == game) {
            c->location->turnCompleter->finishTurn();
        }
    } else if (!endTurn) {
        /* if our turn did not end, then manually redraw the text prompt */
        screenPrompt();
    }
    return valid || KeyHandler::defaultHandler(key, nullptr);
} // GameController::keyPressed

std::string gameGetInput(int maxlen)
{
    screenEnableCursor();
    screenShowCursor();
    return ReadStringController::get(
        maxlen, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line
    );
}

int gameGetPlayer(bool canBeDisabled, bool canBeActivePlayer, bool zeroIsValid)
{
    int player;
#if 0
    if (c->saveGame->members <= 1) {
        player = 0;
    } else
#endif
    {
        if (canBeActivePlayer && (c->party->getActivePlayer() >= 0)) {
            player = c->party->getActivePlayer();
        } else {
            ReadPlayerController readPlayerController;
            eventHandler->pushController(&readPlayerController);
            player = readPlayerController.waitFor();
        }
        if ((player == -1) || ((player == 8) && !zeroIsValid)) {
            screenMessage("\bNIEMAND\n");
            return -1;
        }
    }
#if 0
    c->col--; // display the selected character name, in place of the number
    if ((player >= 0) && (player < 8)) {
        // Write player's name after prompt
        screenMessage("%s\n", c->saveGame->players[player].name);
    }
#endif
    if (!canBeDisabled && c->party->member(player)->isDisabled()) {
        soundPlay(SOUND_ERROR);
        screenMessage("%cAUSSER GEFECHT!%c\n", FG_GREY, FG_WHITE);
        return -1;
    }
    ASSERT(
        (player == 8) || (player < c->party->size()),
        "player %d, but only %d members\n",
        player,
        c->party->size()
    );
    return player;
} // gameGetPlayer

Direction gameGetDirection()
{
    ReadDirController dirController;
    screenMessage("-");
    eventHandler->pushController(&dirController);
    Direction dir = dirController.waitFor();
    if (dir == DIR_NONE) {
        screenMessage("NICHTS\n");
        return dir;
    } else {
        screenMessage("%s\n", getDirectionName(dir));
        return dir;
    }
}

static bool gameSpellMixHowMany(int spell, int num, Ingredients *ingredients)
{
    int i;
    /* entered 0 mixtures, don't mix anything! */
    if (num == 0) {
        screenMessage("\nKEINE GEMISCHT!\n");
        ingredients->revert();
        return false;
    }
    /* if they ask for more than will give them 99, only use what they need */
    if (num > 99 - c->saveGame->mixtures[spell]) {
        num = 99 - c->saveGame->mixtures[spell];
        soundPlay(SOUND_ERROR);
        screenMessage("\n%cDU BRAUCHST NUR %d!%c\n", FG_GREY, num, FG_WHITE);
    }
    screenMessage("\nMISCHE %d...\n", num);
    /* see if there's enough reagents to make number of mixtures requested */
    if (!ingredients->checkMultiple(num)) {
        soundPlay(SOUND_ERROR);
        screenMessage(
            "\n%cDU HAST NICHT GENUG REAGENZIEN, UM %d ZAUBER ZU MISCHEN!%c\n",
            FG_GREY,
            num,
            FG_WHITE
        );
        ingredients->revert();
        return false;
    }
    screenMessage("\nDU MISCHST DIE REAGENZIEN, UND...\n");
    if (spellMix(spell, ingredients)) {
        screenMessage("ERFOLG!\n\n");
        /* mix the extra spells */
        ingredients->multiply(num);
        for (i = 0; i < num - 1; i++) {
            spellMix(spell, ingredients);
        }
    } else {
        screenMessage("ES VERPUFFT!\n\n");
    }
    return true;
} // gameSpellMixHowMany

bool ZtatsController::keyPressed(int key)
{
    switch (key) {
    case U4_UP:
    case U4_LEFT:
        c->stats->prevItem();
        return true;
    case U4_DOWN:
    case U4_RIGHT:
        c->stats->nextItem();
        return true;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        if (c->saveGame->members >= key - '0') {
            c->stats->setView(StatsView(STATS_CHAR1 + key - '1'));
        }
        return true;
    case '0':
        c->stats->setView(STATS_WEAPONS);
        return true;
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        c->stats->setView(StatsView(STATS_PARTY_OVERVIEW));
        doneWaiting();
        return true;
    default:
        return KeyHandler::defaultHandler(key, nullptr);
    }
}

void destroy()
{
    screenMessage("Zerst|re Objekt\nRICHTUNG");
    Direction dir = gameGetDirection();
    if (dir == DIR_NONE) {
        return;
    }
    std::vector<Coords> path = gameGetDirectionalActionPath(
        MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 1, 1, nullptr, true
    );
    for (std::vector<Coords>::iterator i = path.begin();
         i != path.end();
         i++) {
        if (destroyAt(*i)) {
            return;
        }
    }
    soundPlay(SOUND_ERROR);
    screenMessage("%cDA IST NICHTS!%c\n", FG_GREY, FG_WHITE);
}

static bool destroyAt(const Coords &coords)
{
    Object *obj = c->location->map->objectAt(coords);
    if (obj) {
        if (isCreature(obj)) {
            Creature *c = dynamic_cast<Creature *>(obj);
            screenMessage("%s ZERST\\RT!\n", uppercase(c->getName()).c_str());
        } else {
            Tile *t = c->location->map->tileset->get(obj->getTile().id);
            screenMessage("%s ZERST\\RT!\n", uppercase(t->getName()).c_str());
        }
        c->location->map->removeObject(obj);
        screenPrompt();
        return true;
    }
    return false;
}

void attack()
{
    screenMessage("Angreifen");
    if (c->party->isFlying()) {
        soundPlay(SOUND_ERROR);
        screenMessage("-%cNUR DRIFT!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    Direction dir = gameGetDirection();
    if (dir == DIR_NONE) {
        return;
    }
    std::vector<Coords> path = gameGetDirectionalActionPath(
        MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 1, 1, nullptr, true
    );
    for (std::vector<Coords>::iterator i = path.begin();
         i != path.end();
         i++) {
        if (attackAt(*i)) {
            return;
        }
    }
    soundPlay(SOUND_ERROR);
    screenMessage("%cKEIN FEIND!%c\n", FG_GREY, FG_WHITE);
}


/**
 * Attempts to attack a creature at map coordinates x,y.  If no
 * creature is present at that point, zero is returned.
 */
static bool attackAt(const Coords &coords)
{
    Object *under;
    const Tile *ground;
    Creature *m;
    m = dynamic_cast<Creature *>(c->location->map->objectAt(coords));
    /* nothing attackable: move on to next tile */
    if ((m == nullptr) || !m->isAttackable()) {
        return false;
    }
    /* attack successful */
    /// TODO: CHEST: Make a user option to not make chests change battlefield
    /// map (1 of 2)
    ground = c->location->map->tileTypeAt(
        c->location->coords, WITH_GROUND_OBJECTS
    );
    if (!ground->isChest()) {
        ground = c->location->map->tileTypeAt(
            c->location->coords, WITHOUT_OBJECTS
        );
        if ((under = c->location->map->objectAt(c->location->coords))
            && under->getTile().getTileType()->isShip()) {
            ground = under->getTile().getTileType();
        }
    }
    /* You're attacking a townsperson!  Alert the guards! */
    if ((m->getType() == Object::PERSON)
        && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR)) {
        c->location->map->alertGuards();
    }
    /* not good karma to be killing the innocent.  Bad avatar! */
    if (m->isGood()
        || ((m->getType() == Object::PERSON)
            && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR))) {
        c->party->adjustKarma(KA_ATTACKED_GOOD);
    }
    EventHandler::simulateDiskLoad(1000, false);
    CombatController *cc = new CombatController(
        CombatMap::mapForTile(
            ground, c->party->getTransport().getTileType(), m
        )
    );
    cc->init(m);
    musicMgr->play();
    cc->begin();
    return true;
} // attackAt

void board()
{
    if (c->transportContext != TRANSPORT_FOOT) {
        soundPlay(SOUND_ERROR);
        screenMessage("Losfahren\n%cKANN NICHT!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    Object *obj = c->location->map->objectAt(c->location->coords);
    if (!obj) {
        soundPlay(SOUND_ERROR);
        screenMessage("Losfahren\n%cWOMIT?%c\n", FG_GREY, FG_WHITE);
        return;
    }
    const Tile *tile = obj->getTile().getTileType();
    if (tile->isShip()) {
        screenMessage("Losfahren\nmit Schiff!\n");
        if (c->lastShip != obj) {
            c->party->setShipHull(50);
        }
    } else if (tile->isHorse()) {
        screenMessage("Losreiten\nauf Pferd!\n");
    } else if (tile->isBalloon()) {
        screenMessage("Losfahren\nmit Ballon!\n");
    } else {
        soundPlay(SOUND_ERROR);
        screenMessage("Losfahren\n%cWOMIT?%c\n", FG_GREY, FG_WHITE);
        return;
    }
    c->party->setTransport(obj->getTile());
    c->location->map->removeObject(obj);
} // board

void castSpell(int player)
{
    if (player == -1) {
        screenMessage("Zaubern\nSpieler-");
        player = gameGetPlayer(false, true, false);
    }
    if (player == -1) {
        return;
    }
    // get the spell to cast
    c->stats->setView(STATS_MIXTURES);
    screenMessage("\nZAUBER-");
    // ### Put the iPad thing too.
    int spell = AlphaActionController::get('z', "ZAUBER-");
    if (spell == -1) {
        return;
    }
    // Prints spell name at prompt
    screenMessage("%s!\n", uppercase(spellGetName(spell)).c_str());
    c->stats->setView(STATS_PARTY_OVERVIEW);
    // if we can't really cast this spell, skip the extra parameters
    if (spellCheckPrerequisites(spell, player) != CASTERR_NOERROR) {
        gameCastSpell(spell, player, 0);
        return;
    }
    // Get the final parameters for the spell
    switch (spellGetParamType(spell)) {
    case Spell::PARAM_NONE:
        gameCastSpell(spell, player, 0);
        break;
    case Spell::PARAM_PHASE:
    {
        screenMessage("ZU PHASE-");
        int choice = ReadChoiceController::get("12345678 \033\n");
        if ((choice < '1') || (choice > '8')) {
            screenMessage("KEINE!\n");
        } else {
            screenMessage("\n");
            gameCastSpell(spell, player, choice - '1');
        }
        break;
    }
    case Spell::PARAM_PLAYER:
    {
        screenMessage("WEN-");
        int subject = gameGetPlayer(true, false, false);
        screenMessage("\n");
        if (subject != -1) {
            gameCastSpell(spell, player, subject);
        }
        break;
    }
    case Spell::PARAM_DIR:
        if (c->location->context == CTX_DUNGEON) {
            gameCastSpell(spell, player, c->saveGame->orientation);
        } else {
            screenMessage("RICHTUNG");
            Direction dir = gameGetDirection();
            if (dir != DIR_NONE) {
                gameCastSpell(spell, player, static_cast<int>(dir));
            }
        }
        break;
    case Spell::PARAM_TYPEDIR:
    {
        screenMessage("ENERGIETYP?");
        EnergyFieldType fieldType = ENERGYFIELD_NONE;
        char key = ReadChoiceController::get("fbgs \033\n\r");
        switch (key) {
        case 'f':
            fieldType = ENERGYFIELD_FIRE;
            break;
        case 'b':
            fieldType = ENERGYFIELD_LIGHTNING;
            break;
        case 'g':
            fieldType = ENERGYFIELD_POISON;
            break;
        case 's':
            fieldType = ENERGYFIELD_SLEEP;
            break;
        default:
            break;
        }
        if (fieldType != ENERGYFIELD_NONE) {
            screenMessage("\n");
            Direction dir;
            if (c->location->context == CTX_DUNGEON) {
                dir = static_cast<Direction>(c->saveGame->orientation);
            } else {
                screenMessage("RICHTUNG");
                dir = gameGetDirection();
            }
            if (dir != DIR_NONE) {
                /* Need to pack both dir and fieldType into param */
                int param = fieldType << 4;
                param |= static_cast<int>(dir);
                gameCastSpell(spell, player, param);
            }
        } else {
            /* Invalid input here = spell failure */
            screenMessage("FEHLSCHLAG!\n");
            /*
             * Confirmed both mixture loss and mp loss in this situation in
             * the original Ultima IV (at least, in the Amiga version.)
             */
            // c->saveGame->mixtures[castSpell]--;
            c->party->member(player)->adjustMp(-spellGetRequiredMP(spell));
        }
        break;
    }
    case Spell::PARAM_FROMDIR:
    {
        screenMessage("AUS RICHTUNG");
        Direction dir = gameGetDirection();
        if (dir != DIR_NONE) {
            gameCastSpell(spell, player, static_cast<int>(dir));
        }
        break;
    }
    } // switch
} // castSpell

void fire()
{
    if (c->transportContext != TRANSPORT_SHIP) {
        soundPlay(SOUND_ERROR);
        screenMessage("Kanone feuern\n%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    screenMessage("Kanone feuern\nRichtung");
    Direction dir = gameGetDirection();
    if (dir == DIR_NONE) {
        return;
    }
    // can only fire broadsides
    int broadsidesDirs = dirGetBroadsidesDirs(c->party->getDirection());
    if (!DIR_IN_MASK(dir, broadsidesDirs)) {
        soundPlay(SOUND_ERROR);
        screenMessage("%cNUR BREITSEITEN!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    // nothing (not even mountains!) can block cannonballs
    std::vector<Coords> path = gameGetDirectionalActionPath(
        MASK_DIR(dir),
        broadsidesDirs,
        c->location->coords,
        1,
        3,
        nullptr,
        false
    );
    for (std::vector<Coords>::iterator i = path.begin();
         i != path.end();
         i++) {
        if (fireAt(*i, true)) {
            return;
        }
    }
} // fire

bool fireAt(const Coords &coords, bool originAvatar)
{
    bool validObject = false;
    bool hitsAvatar = false;
    bool objectHit = false;
    Object *obj = nullptr;
    MapTile tile(c->location->map->tileset->getByName("miss_flash")->getId());
    GameController::flashTile(coords, tile, 1);
    obj = c->location->map->objectAt(coords);
    Creature *m = dynamic_cast<Creature *>(obj);
    if (obj && (obj->getType() == Object::CREATURE) && m->isAttackable()) {
        validObject = true;
    }
    /* See if it's an object to be destroyed (the avatar cannot destroy
       the balloon) */
    else if (
        obj
        && (obj->getType() == Object::UNKNOWN)
        && !(obj->getTile().getTileType()->isBalloon() && originAvatar)
    ) {
        validObject = true;
    }
    /* Does the cannon hit the avatar? */
    if (coords == c->location->coords) {
        validObject = true;
        hitsAvatar = true;
    }
    if (validObject) {
        /* always displays as a 'hit' though the object may not be
           destroyed */
        /* Is it a pirate ship firing at US? */
        if (hitsAvatar) {
            GameController::flashTile(coords, "hit_flash", 4);
            c->party->applyEffect(EFFECT_FIRE);
        }
        /* inanimate objects get destroyed instantly, while creatures
           get a chance */
        else if (obj->getType() == Object::UNKNOWN) {
            soundPlay(SOUND_NPC_STRUCK, false);
            GameController::flashTile(coords, "hit_flash", 4);
            c->location->map->removeObject(obj);
        }
        /* only the avatar can hurt other creatures with cannon fire */
        else if (originAvatar) {
            soundPlay(SOUND_NPC_STRUCK, false);
            GameController::flashTile(coords, "hit_flash", 4);
            if (xu4_random(4) == 0) { /* reverse-engineered from u4dos */
                Creature *m = dynamic_cast<Creature *>(obj);
                if (!m || m->getId() != LORDBRITISH_ID) {
                    c->location->map->removeObject(obj);
                }
            }
        }
        objectHit = true;
    }
    return objectHit;
} // fireAt


/**
 * Get the chest at the current x,y of the current context for player 'player'
 */
void getChest(int player)
{
    if (player != -2) {
        screenMessage("Truhe |ffnen\n");
    }
    if (c->party->isFlying()) {
        soundPlay(SOUND_ERROR);
        screenMessage("%cNUR DRIFT!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    // first check to see if a chest exists at the current location
    // if one exists, prompt the player for the opener, if necessary
    MapCoords coords;
    c->location->getCurrentPosition(&coords);
    const Tile *tile = c->location->map->tileTypeAt(
        coords, WITH_GROUND_OBJECTS
    );
    /* get the object for the chest, if it is indeed an object */
    Object *obj = c->location->map->objectAt(coords);
    if (obj && !obj->getTile().getTileType()->isChest()) {
        obj = nullptr;
    }
    if (tile->isChest() || obj) {
        // if a spell was cast to open this chest,
        // player will equal -2, otherwise player
        // will default to -1 or the default character
        // number if one was earlier specified
        if (player == -1) {
            screenMessage("Wer |ffnet-");
            player = gameGetPlayer(false, true, false);
            if (player == -1) {
                return;
            }
            screenMessage("\n");
        }
        if (obj) {
            c->location->map->removeObject(obj);
        } else {
            TileId newTile = c->location->getReplacementTile(coords, tile);
            c->location->map->annotations->add(coords, newTile, false, true);
        }
        // see if the chest is trapped and handle it
        if (getChestTrapHandler(player)) {
            screenMessage(
                "SIE ENTH[LT:\n%02d-GOLD!\n", c->party->getChest()
            );
        } else {
            screenMessage("%cTRUHE ZERST\\RT!%c\n", FG_RED, FG_WHITE);
        }
        screenPrompt();
        if (isCity(c->location->map) && (obj == nullptr)) {
            c->party->adjustKarma(KA_STOLE_CHEST);
        }
    } else {
        soundPlay(SOUND_ERROR);
        screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
    }
} // getChest


/**
 * Called by getChest() to handle possible traps on chests
 * Returns true if trap was either not there or avoided
 * Returns false if trap was set off (destroying gold along with chest)
 **/
static bool getChestTrapHandler(int player)
{
    TileEffect trapType = EFFECT_FIRE;
    int passTest = !xu4_random(2); /* xu4-enhanced */
    /* Chest is trapped! 50/50 chance */
    if (passTest) {
        /* Figure out which trap the chest has */
        int randNum = xu4_random(4);
        switch (randNum & xu4_random(4)) {
        case 0:
            trapType = EFFECT_FIRE;
            break; /* acid trap (56% chance - 9/16) */
        case 1:
            trapType = EFFECT_SLEEP;
            break; /* sleep trap (19% chance - 3/16) */
        case 2:
            trapType = EFFECT_POISON;
            break; /* poison trap (19% chance - 3/16) */
        case 3:
            trapType = EFFECT_LAVA;
            break; /* bomb trap (6% chance - 1/16) */
        default:
            ASSERT(0, "Wrong logic in getChestTrapHandler()!");
        }
        /* apply the effects from the trap */
        if (trapType == EFFECT_FIRE) {
            screenMessage("\n%cS[URE%cFALLE!\n", FG_BLUE, FG_WHITE);
        } else if (trapType == EFFECT_POISON) {
            screenMessage("\n%cGIFT%cFALLE!\n", FG_GREEN, FG_WHITE);
        } else if (trapType == EFFECT_SLEEP) {
            screenMessage("\n%cSCHLAF%cFALLE!\n", FG_PURPLE, FG_WHITE);
        } else if (trapType == EFFECT_LAVA) {
            screenMessage("\n%cBOMBEN%cFALLE!\n", FG_RED, FG_WHITE);
        }
        // player is < 0 during the 'O'pen spell (immune to traps)
        //
        // if the chest was opened by a PC, see if the trap was
        // evaded by testing the PC's dex
        //
        if ((player >= 0)
            && (c->saveGame->players[player].dex + 25 < xu4_random(100))) {
            if (trapType == EFFECT_LAVA) { /* bomb trap */
                c->party->applyEffect(trapType);
            } else {
                c->party->member(player)->applyEffect(trapType);
            }
        } else {
            soundPlay(SOUND_FLEE);
            screenMessage("VERMIEDEN!\n\n");
            return true;
        }
        return trapType != EFFECT_LAVA; //xu4-enhanced: Bomb traps destroy
    }
    screenMessage("\n");
    return true;
} // getChestTrapHandler

void holeUp()
{
    screenMessage("Campieren...\n");
    if (!(c->location->context & (CTX_WORLDMAP | CTX_DUNGEON))) {
        soundPlay(SOUND_ERROR);
        screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    if (c->transportContext != TRANSPORT_FOOT) {
        soundPlay(SOUND_ERROR);
        screenMessage("%cNUR ZU FUSS!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    CombatController *cc = new CampController();
    cc->init(nullptr);
    cc->begin();
}

/**
 * Initializes the moon state according to the savegame file. This method of
 * initializing the moons (rather than just setting them directly) is necessary
 * to make sure trammel and felucca stay in sync
 */
void GameController::initMoons()
{
    int trammelphase = c->saveGame->trammelphase,
        feluccaphase = c->saveGame->feluccaphase;

    ASSERT(c != nullptr, "Game context doesn't exist!");
    ASSERT(c->saveGame != nullptr, "Savegame doesn't exist!");
    c->saveGame->trammelphase = c->saveGame->feluccaphase = 0;
    c->moonPhase = 0;
    while ((c->saveGame->trammelphase != trammelphase)
           || (c->saveGame->feluccaphase != feluccaphase)) {
        updateMoons(false);
    }
}


/**
 * Updates the phases of the moons and shows
 * the visual moongates on the map, if desired
 */
void GameController::updateMoons(bool showmoongates)
{
    int realMoonPhase, oldTrammel, trammelSubphase;
    const Coords *gate;
    if (c->location->map->isWorldMap() || !showmoongates) {
        oldTrammel = c->saveGame->trammelphase;
        if (++c->moonPhase >= MOON_PHASES * MOON_SECONDS_PER_PHASE * 4) {
            c->moonPhase = 0;
        }
        trammelSubphase = c->moonPhase % (MOON_SECONDS_PER_PHASE * 4 * 3);
        realMoonPhase = (c->moonPhase / (4 * MOON_SECONDS_PER_PHASE));
        c->saveGame->trammelphase = realMoonPhase / 3;
        c->saveGame->feluccaphase = realMoonPhase % 8;
        if (c->saveGame->trammelphase > 7) {
            c->saveGame->trammelphase = 7;
        }
        if (showmoongates) {
            /* update the moongates if trammel changed */
            if (trammelSubphase == 0) {
                gate = moongateGetGateCoordsForPhase(oldTrammel);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x40)
                    );
                }
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x40)
                    );
                }
            } else if (trammelSubphase == 1) {
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x40)
                    );
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x41)
                    );
                }
            } else if (trammelSubphase == 2) {
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x41)
                    );
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x42)
                    );
                }
            } else if (trammelSubphase == 3) {
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x42)
                    );
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x43)
                    );
                }
            } else if ((trammelSubphase > 3)
                       && (trammelSubphase
                           < (MOON_SECONDS_PER_PHASE * 4 * 3) - 3)) {
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x43)
                    );
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x43)
                    );
                }
            } else if (trammelSubphase
                       == (MOON_SECONDS_PER_PHASE * 4 * 3) - 3) {
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x43)
                    );
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x42)
                    );
                }
            } else if (trammelSubphase
                       == (MOON_SECONDS_PER_PHASE * 4 * 3) - 2) {
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x42)
                    );
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x41)
                    );
                }
            } else if (trammelSubphase
                       == (MOON_SECONDS_PER_PHASE * 4 * 3) - 1) {
                gate =
                    moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(
                        *gate, c->location->map->tfrti(0x41)
                    );
                    c->location->map->annotations->add(
                        *gate, c->location->map->tfrti(0x40)
                    );
                }
            }
        }
    }
} // GameController::updateMoons


/**
 * Handles feedback after avatar moved during normal 3rd-person view.
 */
void GameController::avatarMoved(MoveEvent &event)
{
    if (event.userEvent) {
        // is filterMoveMessages even used?  it doesn't look like the
        // option is hooked up in the configuration menu
        if (!settings.filterMoveMessages) {
            switch (c->transportContext) {
            case TRANSPORT_FOOT:
            case TRANSPORT_HORSE:
                screenMessage("%s\n", getDirectionName(event.dir));
                break;
            case TRANSPORT_SHIP:
                if (event.result & MOVE_TURNED) {
                    screenMessage("%s drehen\n", getDirectionName(event.dir));
                } else if (event.result & MOVE_SLOWED) {
                    screenMessage("%cLANGSAM VORAN!%c\n", FG_GREY, FG_WHITE);
                } else {
                    screenMessage("%s fahren\n", getDirectionName(event.dir));
                }
                break;
            case TRANSPORT_BALLOON:
                soundPlay(SOUND_ERROR);
                screenMessage("%cNUR DRIFT!%c\n", FG_GREY, FG_WHITE);
                break;
            default:
                ASSERT(
                    0,
                    "bad transportContext %d in avatarMoved()",
                    c->transportContext
                );
            }
        }
        /* movement was blocked */
        if (event.result & MOVE_BLOCKED) {
            /* if shortcuts are enabled, try them! */
            if (settings.shortcutCommands) {
                MapCoords new_coords = c->location->coords;
                MapTile *tile;
                new_coords.move(event.dir, c->location->map);
                tile = c->location->map->tileAt(new_coords, WITH_OBJECTS);
                if (tile->getTileType()->isDoor()) {
                    openAt(new_coords);
                    event.result = static_cast<MoveResult>(
                        MOVE_SUCCEEDED | MOVE_END_TURN
                    );
                } else if (tile->getTileType()->isLockedDoor()) {
                    jimmyAt(new_coords);
                    event.result = static_cast<MoveResult>(
                        MOVE_SUCCEEDED | MOVE_END_TURN
                    );
                }
#if 0
                else if (mapPersonAt(c->location->map, new_coords)
                         != nullptr) {
                    talkAtCoord(newx, newy, 1, nullptr);
                    event.result = MOVE_SUCCEEDED | MOVE_END_TURN;
                }
#endif
            }
            /* if we're still blocked */
            if ((event.result & MOVE_BLOCKED)
                && !settings.filterMoveMessages) {
                soundPlay(SOUND_BLOCKED, false);
                screenMessage("%cBLOCKIERT!%c\n", FG_GREY, FG_WHITE);
            }
        } else if ((c->transportContext == TRANSPORT_FOOT)
                   || (c->transportContext == TRANSPORT_HORSE)) {
            /* movement was slowed */
            if (event.result & MOVE_SLOWED) {
                soundPlay(SOUND_WALK_SLOWED);
                screenMessage("%cLANGSAM VORAN!%c\n", FG_GREY, FG_WHITE);
            } else {
                soundPlay(SOUND_WALK_NORMAL);
            }
        }
    }
    /* simulate disk load when part of next 16x16 square becomes visible */
    if ((event.result & MOVE_SUCCEEDED) &&
        (c->location->context == CTX_WORLDMAP)) {
        unsigned int globalX = c->location->coords.x >> 4;
        unsigned int globalY = c->location->coords.y >> 4;
        unsigned int localX  = c->location->coords.x & 0xF;
        unsigned int localY  = c->location->coords.y & 0xF;
        unsigned int activeX = c->location->coords.active_x;
        unsigned int activeY = c->location->coords.active_y;
        if (
            (event.dir == DIR_WEST ) &&
            (globalX == activeX) &&
            (localX == 4)
        ) {
            c->location->coords.active_x = (activeX - 1) & 0xF;
            EventHandler::simulateDiskLoad(500);
        }
        if (
            (event.dir == DIR_NORTH) &&
            (globalY == activeY) &&
            (localY ==  4)
        ) {
            c->location->coords.active_y = (activeY - 1) & 0xF;
            EventHandler::simulateDiskLoad(500);
        }
        if (
            (event.dir == DIR_EAST ) &&
            (globalX == ((activeX + 1) & 0xF)) &&
            (localX == 11)
        ) {
            c->location->coords.active_x = (activeX + 1) & 0xF;
            EventHandler::simulateDiskLoad(500);
        }
        if (
            (event.dir == DIR_SOUTH) &&
            (globalY == ((activeY + 1) & 0xF)) &&
            (localY == 11)
        ) {
            c->location->coords.active_y = (activeY + 1) & 0xF;
            EventHandler::simulateDiskLoad(500);
        }
    }
    /* exited map */
    if (event.result & MOVE_EXIT_TO_PARENT) {
        screenMessage("%cVERLASSEN...%c\n", FG_GREY, FG_WHITE);
        EventHandler::simulateDiskLoad(2000, false);
        exitToParentMap();
        musicMgr->play();
    }
    /* things that happen while not on board the balloon */
    if (c->transportContext & ~TRANSPORT_BALLOON) {
        checkSpecialCreatures(event.dir);
    }
    /* things that happen while on foot or horseback */
    if ((c->transportContext & TRANSPORT_FOOT_OR_HORSE)
        && !(event.result & (MOVE_SLOWED | MOVE_BLOCKED))) {
        if (checkMoongates()) {
            event.result = static_cast<MoveResult>(
                MOVE_MAP_CHANGE | MOVE_END_TURN
            );
        }
    }
} // GameController::avatarMoved


/**
 * Handles feedback after moving the avatar in the 3-d dungeon view.
 */
void GameController::avatarMovedInDungeon(MoveEvent &event)
{
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    Direction realDir = dirNormalize(
        static_cast<Direction>(c->saveGame->orientation),
        event.dir
    );
    if (!settings.filterMoveMessages) {
        if (event.userEvent) {
            if (event.result & MOVE_TURNED) {
                if (dirRotateCCW(
                        static_cast<Direction>(c->saveGame->orientation)
                    ) == realDir) {
                    screenMessage("Links um\n");
                } else {
                    screenMessage("Rechts um\n");
                }
            }
            /* show 'Advance' or 'Retreat' in dungeons */
            else {
                screenMessage(
                    "%s\n",
                    realDir == c->saveGame->orientation ?
                    "Vormarsch" :
                    "R}ckzug"
                );
            }
        }
        if (event.result & MOVE_BLOCKED) {
            soundPlay(SOUND_BLOCKED, false);
            screenMessage("%cBLOCKIERT!%c\n", FG_GREY, FG_WHITE);
        }
    }

    /* if we're exiting the map, do this */
    if (event.result & MOVE_EXIT_TO_PARENT) {
        screenMessage("%cVERLASSEN...%c\n", FG_GREY, FG_WHITE);
        EventHandler::simulateDiskLoad(2000, false);
        exitToParentMap();
        musicMgr->play();
    }
    /* check to see if we're entering a dungeon room */
    if (event.result & MOVE_SUCCEEDED) {
        if (dungeon->currentToken() == DUNGEON_ROOM) {
            /* get room number */
            int room = static_cast<int>(dungeon->currentSubToken());
            /**
             * recalculate room for the abyss -- there are 16 rooms for every
             * 2 levels, each room marked with 0xD* where (* == room number
             * 0-15). for levels 1 and 2, there are 16 rooms, levels 3 and 4
             * there are 16 rooms, etc.
             */
            if (c->location->map->id == MAP_ABYSS) {
                room = (0x10 * (c->location->coords.z / 2)) + room;
            }
            Dungeon *dng = dynamic_cast<Dungeon *>(c->location->map);
            dng->currentRoom = room;
            /* set the map and start combat! */
            EventHandler::simulateDiskLoad(1000, false);
            CombatController *cc = new CombatController(dng->roomMaps[room]);
            cc->initDungeonRoom(room, dirReverse(realDir));
            musicMgr->play();
            cc->begin();
        }
    }
} // GameController::avatarMovedInDungeon

void jimmy()
{
    screenMessage("Dietrich");
    Direction dir = gameGetDirection();
    if (dir == DIR_NONE) {
        return;
    }
    std::vector<Coords> path = gameGetDirectionalActionPath(
        MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 1, 1, nullptr, true
    );
    for (std::vector<Coords>::iterator i = path.begin();
         i != path.end();
         i++) {
        if (jimmyAt(*i)) {
            return;
        }
    }
    soundPlay(SOUND_ERROR);
    screenMessage("%cWOZU?%c\n", FG_GREY, FG_WHITE);
}


/**
 * Attempts to jimmy a locked door at map coordinates x,y.  The locked
 * door is replaced by a permanent annotation of an unlocked door
 * tile.
 */
static bool jimmyAt(const Coords &coords)
{
    MapTile *tile = c->location->map->tileAt(coords, WITH_OBJECTS);
    if (!tile->getTileType()->isLockedDoor()) {
        return false;
    }
    if (c->saveGame->keys) {
        Tile *door = c->location->map->tileset->getByName("door");
        ASSERT(door, "no door tile found in tileset");
        c->saveGame->keys--;
        c->location->map->annotations->add(coords, door->getId());
        screenMessage("\nENTRIEGELT!\n");
    } else {
        soundPlay(SOUND_ERROR);
        screenMessage("%cKEINE ]BRIG!%c\n", FG_GREY, FG_WHITE);
    }
    return true;
}

void opendoor()
{
    ///  XXX: Pressing "o" should close any open door.
    screenMessage("\\ffnen");
    if (c->party->isFlying()) {
        soundPlay(SOUND_ERROR);
        screenMessage("-%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    Direction dir = gameGetDirection();
    if (dir == DIR_NONE) {
        return;
    }
    std::vector<Coords> path = gameGetDirectionalActionPath(
        MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 1, 1, nullptr, true
    );
    for (std::vector<Coords>::iterator i = path.begin();
         i != path.end();
         i++) {
        if (openAt(*i)) {
            return;
        }
    }
    soundPlay(SOUND_ERROR);
    screenMessage("%cHIER NICHT!%c\n", FG_GREY, FG_WHITE);
}


/**
 * Attempts to open a door at map coordinates x,y.  The door is
 * replaced by a temporary annotation of a floor tile for 4 turns.
 */
static bool openAt(const Coords &coords)
{
    const Tile *tile = c->location->map->tileTypeAt(coords, WITH_OBJECTS);
    if (!tile->isDoor() && !tile->isLockedDoor()) {
        return false;
    }
    if (tile->isLockedDoor()) {
        soundPlay(SOUND_ERROR);
        screenMessage("%cKANN NICHT!%c\n", FG_GREY, FG_WHITE);
        return true;
    }
    Tile *floor = c->location->map->tileset->getByName("brick_floor");
    ASSERT(floor, "no floor tile found in tileset");
    c->location->map->annotations->add(
        coords, floor->getId(), false, true
    )->setTTL(4);
    screenMessage("\nGE\\FFNET!\n");
    return true;
}


/**
 * Readies a weapon for a player.  Prompts for the player and/or the
 * weapon if not provided.
 */
void readyWeapon(int player)
{
    // get the player if not provided
    if (player == -1) {
        screenMessage("Waffe halten\nf}r-");
        player = gameGetPlayer(true, false, false);
        if (player == -1) {
            return;
        }
        screenMessage("\n");
    }
    // get the weapon to use
    c->stats->setView(STATS_WEAPONS);
    screenMessage("WAFFE-");
    WeaponType weapon = static_cast<WeaponType>(
        AlphaActionController::get(WEAP_MAX + 'a' - 1, "WAFFE-")
    );
    c->stats->setView(STATS_PARTY_OVERVIEW);
    if (weapon == -1) {
        return;
    }
    PartyMember *p = c->party->member(player);
    const Weapon *w = Weapon::get(weapon);
    if (!w) {
        screenMessage("\n");
        return;
    }
    switch (p->setWeapon(w)) {
    case EQUIP_SUCCEEDED:
        screenMessage("%s\n", uppercase(w->getName()).c_str());
        break;
    case EQUIP_NONE_LEFT:
        soundPlay(SOUND_ERROR);
        screenMessage("\n%cKEINE ]BRIG!%c\n", FG_GREY, FG_WHITE);
        break;
    case EQUIP_CLASS_RESTRICTED:
        soundPlay(SOUND_ERROR);
        screenMessage("%s\n", uppercase(w->getName()).c_str());
        screenMessage(
            "\n%cEIN%s %s DARF %s BENUTZEN!%c\n",
            FG_GREY,
            (p->getSex() == SEX_FEMALE ? "E" : ""),
            uppercase(
                getClassNameTranslated(p->getClass(), p->getSex())
            ).c_str(),
            uppercase(w->getNeg()).c_str(),
            FG_WHITE
        );
        break;
    }
} // readyWeapon

void talk()
{
    screenMessage("Sprechen");
    if (c->party->isFlying()) {
        soundPlay(SOUND_ERROR);
        screenMessage("-%cNUR DRIFT!%c\n", FG_GREY, FG_WHITE);
        return;
    }
    Direction dir = gameGetDirection();
    if (dir == DIR_NONE) {
        return;
    }
    std::vector<Coords> path = gameGetDirectionalActionPath(
        MASK_DIR(dir),
        MASK_DIR_ALL,
        c->location->coords,
        1,
        2,
        &Tile::canTalkOverTile,
        true
    );
    int dist = 1;
    for (std::vector<Coords>::iterator i = path.begin();
         i != path.end();
         i++) {
        if (talkAt(*i, dist++)) {
            return;
        }
    }
    screenMessage("KOMISCH, KEINE ANTWORT!\n");
}

/**
 * Mixes reagents.  Prompts for a spell, then which reagents to
 * include in the mix.
 */
static void mixReagents()
{
    /*  uncomment this line to activate new spell mixing code */
    // return mixReagentsSuper();
    bool done = false;
    screenMessage("Mischen...\n");
    EventHandler::simulateDiskLoad(2000);
    while (!done) {
            // Verify that there are reagents remaining in the inventory
        bool found = false;
        for (int i = 0; i < 8; i++) {
            if (c->saveGame->reagents[i] > 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            soundPlay(SOUND_ERROR);
            screenMessage("%cKEINE ]BRIG!%c", FG_GREY, FG_WHITE);
            done = true;
        } else {
            screenMessage("F]R SPRUCH-");
            c->stats->setView(STATS_MIXTURES);
            int choice = ReadChoiceController::get(
                "abcdefghijklmnopqrstuvwxyz \033\n\r"
            );
            if ((choice == ' ')
                || (choice == '\033')
                || (choice == '\n')
                || (choice == '\r')) {
                break;
            }
            int spell = choice - 'a';
            screenMessage("%s\n", uppercase(spellGetName(spell)).c_str());
            // ensure the mixtures for the spell isn't already maxed out
            if (c->saveGame->mixtures[spell] == 99) {
                screenMessage(
                    "\n%cDU KANNST NICHT NOCH MEHR VON DIESEM ZAUBER "
                    "MISCHEN!%c\n",
                    FG_GREY,
                    FG_WHITE
                );
                break;
            }
            // Reset the reagent spell mix menu by removing
            // the menu highlight from the current item, and
            // hiding reagents that you don't have
            c->stats->resetReagentsMenu();
            c->stats->setView(MIX_REAGENTS);
            if (settings.enhancements
                && settings.enhancementsOptions.u5spellMixing) {
                done = mixReagentsForSpellU5(spell);
            } else {
                done = mixReagentsForSpellU4(spell);
            }
        }
    }
    c->stats->setView(STATS_PARTY_OVERVIEW);
    screenMessage("\n\n");
} // mixReagents


/**
 * Prompts for spell reagents to mix in the traditional Ultima IV
 * style.
 */
static bool mixReagentsForSpellU4(int spell)
{
    Ingredients ingredients;
    screenMessage("REAGENZ-");
    while (1) {
        int choice = ReadChoiceController::get("abcdefgh\n\r \033");
        // done selecting reagents? mix it up and prompt to mix
        // another spell
        if ((choice == '\n') || (choice == '\r') || (choice == ' ')) {
            screenMessage("\n\nDU MISCHST DIE REAGENZIEN, UND...\n");
            if (spellMix(spell, &ingredients)) {
                screenMessage("ERFOLG!\n\n");
            } else {
                screenMessage("ES VERPUFFT!\n\n");
            }
            return false;
        }
        // escape: put ingredients back and quit mixing
        if (choice == '\033') {
            ingredients.revert();
            return true;
        }
        screenMessage("\n");
        if (!ingredients.addReagent(static_cast<Reagent>(choice - 'a'))) {
            soundPlay(SOUND_ERROR);
            screenMessage("%cKEINE ]BRIG!%c\n", FG_GREY, FG_WHITE);
        }
        screenMessage("REAGENZ-");
    }
    return true;
} // mixReagentsForSpellU4


/**
 * Prompts for spell reagents to mix with an Ultima V-like menu.
 */
static bool mixReagentsForSpellU5(int spell)
{
    Ingredients ingredients;

    screenDisableCursor();
    // reset the menu, highlighting the first item
    c->stats->getReagentsMenu()->reset();
    ReagentsMenuController getReagentsController(
        c->stats->getReagentsMenu(), &ingredients, c->stats->getMainArea()
    );
    eventHandler->pushController(&getReagentsController);
    getReagentsController.waitFor();
    c->stats->getMainArea()->disableCursor();
    screenEnableCursor();
    screenMessage("WIE VIELE? ");
    int howmany =
        ReadIntController::get(2, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    gameSpellMixHowMany(spell, howmany, &ingredients);
    return true;
}


/**
 * Exchanges the position of two players in the party.  Prompts the
 * user for the player numbers.
 */
static void newOrder()
{
    screenMessage("Ordnung {ndern\nTAUSCHE-");
    int player1 = gameGetPlayer(true, false, false);
    if (player1 == -1) {
        return;
    }
    if (player1 == 0) {
        soundPlay(SOUND_ERROR);
        screenMessage(
            "\n%s, DU MUSST F]HREN!\n",
            uppercase(c->party->member(0)->getName()).c_str()
        );
        return;
    }
    screenMessage("\nGEGEN-");
    int player2 = gameGetPlayer(true, false, false);
    if (player2 == -1) {
        return;
    }
    if (player2 == 0) {
        soundPlay(SOUND_ERROR);
        screenMessage(
            "\n%s, DU MUSST F]HREN!\n",
            uppercase(c->party->member(0)->getName()).c_str()
        );
        return;
    }
    if (player1 == player2) {
        soundPlay(SOUND_ERROR);
        screenMessage("\n%cWAS?%c\n", FG_GREY, FG_WHITE);
        return;
    }
    c->party->swapPlayers(player1, player2);
    screenMessage("\n");
} // newOrder


/**
 * Peers at a city from A-P (Lycaeum telescope) and functions like a gem
 */
bool gamePeerCity(int city, void *)
{
    Map *peerMap;
    peerMap = mapMgr->get(static_cast<MapId>(city + 1));
    if (peerMap != nullptr) {
        game->setMap(peerMap, 1, nullptr);
        c->location->viewMode = VIEW_GEM;
        game->paused = true;
        game->pausedTimer = 0;
        screenDisableCursor();
        ReadChoiceController::get("\015 \033");
        game->exitToParentMap();
        screenEnableCursor();
        game->paused = false;
        return true;
    }
    return false;
}


/**
 * Peers at a gem
 */
void peer(bool useGem)
{
    if (useGem) {
        if (c->saveGame->gems <= 0) {
            soundPlay(SOUND_ERROR);
            screenMessage(
                "Juwel ansehen\n%cKEINE ]BRIG!%c\n", FG_GREY, FG_WHITE
            );
            return;
        }
        c->saveGame->gems--;
        screenMessage("Juwel ansehen\n");
    }
    game->paused = true;
    game->pausedTimer = 0;
    musicMgr->gem();
    screenDisableCursor();
    c->location->viewMode = VIEW_GEM;
    ReadChoiceController::get("\015 \033");
    musicMgr->play();
    screenEnableCursor();
    screenMessage("\n%c", CHARSET_PROMPT);
    c->location->viewMode = VIEW_NORMAL;
    game->paused = false;
}


/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
static bool talkAt(const Coords &coords, int distance)
{
    City *city;
    /* can't have any conversations outside of town */
    if (!isCity(c->location->map)) {
        screenMessage("KOMISCH, KEINE ANTWORT!\n");
        return true;
    }
    city = dynamic_cast<City *>(c->location->map);
    Person *talker = city->personAt(coords);
    /* make sure we have someone we can talk with */
    if (!talker || !talker->canConverse()) {
        return false;
    }
    /* Prevent talking to ghosts & skeletons over the counter */
    if (distance > 1 && (talker->getId() != VILLAGER_ID)) {
        return false;
    }
    /* No response from alerted guards... does any monster both
     * attack and talk besides Nate the Snake? */
    if ((talker->getMovementBehavior() == MOVEMENT_ATTACK_AVATAR)
        && (talker->getId() != PYTHON_ID)) {
        return false;
    }
    /* if we've come that far, it's certain that there is
       somebody to talk to... so delay now. */
    if (talker->getNpcType() >= NPC_VENDOR_WEAPONS) {
        EventHandler::simulateDiskLoad(2000, false);
    } else {
        EventHandler::simulateDiskLoad(500, false);
    }
    /* if we're talking to Lord British and the avatar is dead,
       LB resurrects them! */
    if ((talker->getNpcType() == NPC_LORD_BRITISH)
        && (c->party->member(0)->getStatus() == STAT_DEAD)) {
        c->willPassTurn = false;
        screenMessage(
            "%s, DU SOLLST WIEDER LEBEN!\n",
            uppercase(c->party->member(0)->getName()).c_str()
        );
        c->party->member(0)->setStatus(STAT_GOOD);
        c->party->member(0)->heal(HT_FULLHEAL);
        gameSpellEffect('r', -1, SOUND_LBHEAL);
    }
    Conversation conv;
    TRACE_LOCAL(gameDbg, "Setting up script information providers.");
    conv.script->addProvider("party", c->party);
    conv.script->addProvider("context", c);
    conv.state = Conversation::INTRO;
    conv.reply = talker->getConversationText(&conv, "");
    conv.playerInput.erase();
    talkRunConversation(conv, talker, false);
    return true;
} // talkAt


/**
 * Executes the current conversation until it is done.
 */
static void talkRunConversation(
    Conversation &conv, Person *talker, bool showPrompt
)
{
    c->willPassTurn = false;
    while (conv.state != Conversation::DONE) {
        // TODO: instead of calculating linesused again, cache the
        // result in person.cpp somewhere.
        int linesused = linecount(conv.reply.front(), TEXT_AREA_W);
        screenMessage("%s", uppercase(conv.reply.front()).c_str());
        conv.reply.pop_front();
        /* if all chunks haven't been shown, wait for a key and process
           next chunk*/
        int size = conv.reply.size();
        if (size > 0) {
            ReadChoiceController::get("");
            continue;
        }
        /* otherwise, clear current reply and proceed based on conversation
           state */
        conv.reply.clear();
        /* they're attacking you! */
        if (conv.state == Conversation::ATTACK) {
            conv.state = Conversation::DONE;
            talker->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
        }
        if (conv.state == Conversation::DONE) {
            break;
        }
        /* When Lord British heals the party */
        else if (conv.state == Conversation::FULLHEAL) {
            int i;
            for (i = 0; i < c->party->size(); i++) {
                c->party->member(i)->heal(HT_CURE); // cure the party
                c->party->member(i)->heal(HT_FULLHEAL); // heal the party
            }
            // same spell effect as 'r'esurrect
            gameSpellEffect('r', -1, SOUND_MAGIC);
            conv.state = Conversation::TALK;
        }
        /* When Lord British checks and advances each party member's level */
        else if (conv.state == Conversation::ADVANCELEVELS) {
            gameLordBritishCheckLevels();
            conv.state = Conversation::TALK;
        }
        if (showPrompt) {
            std::string prompt = talker->getPrompt(&conv);
            if (!prompt.empty()) {
                if ((conv.state == Conversation::ASK)
                    || (conv.state == Conversation::CONFIRMATION)
                    || (linesused + linecount(prompt, TEXT_AREA_W)
                        > TEXT_AREA_H)) {
                    ReadChoiceController::get("");
                }
                screenMessage("%s", prompt.c_str());
            }
        }
        int maxlen;
        switch (conv.getInputRequired(&maxlen)) {
        case Conversation::INPUT_STRING:
            conv.playerInput = gameGetInput(maxlen);
            conv.reply = talker->getConversationText(
                &conv, conv.playerInput.c_str()
            );
            conv.playerInput.erase();
            showPrompt = true;
            break;
        case Conversation::INPUT_CHARACTER:
        {
            char message[2];
            int choice = ReadChoiceController::get("");
            message[0] = choice;
            message[1] = '\0';
            conv.reply = talker->getConversationText(&conv, message);
            conv.playerInput.erase();
            showPrompt = true;
            break;
        }
        case Conversation::INPUT_NONE:
            conv.state = Conversation::DONE;
            break;
        }
    }
    if (conv.reply.size() > 0) {
        screenMessage("%s", uppercase(conv.reply.front()).c_str());
    }
    c->lastCommandTime = std::time(nullptr);
    c->willPassTurn = true;
} // talkRunConversation


/**
 * Changes a player's armor.  Prompts for the player and/or the armor
 * type if not provided.
 */
static void wearArmor(int player)
{
    // get the player if not provided
    if (player == -1) {
        screenMessage("R}stung tragen\nf}r-");
        player = gameGetPlayer(true, false, false);
        if (player == -1) {
            return;
        }
        screenMessage("\n");
    }
    c->stats->setView(STATS_ARMOR);
    screenMessage("R]STUNG-");
    ArmorType armor = static_cast<ArmorType>(
        AlphaActionController::get(ARMR_MAX + 'a' - 1, "R}STUNG-")
    );
    c->stats->setView(STATS_PARTY_OVERVIEW);
    if (armor == -1) {
        return;
    }
    const Armor *a = Armor::get(armor);
    PartyMember *p = c->party->member(player);
    if (!a) {
        screenMessage("\n");
        return;
    }
    switch (p->setArmor(a)) {
    case EQUIP_SUCCEEDED:
        screenMessage("%s\n", uppercase(a->getName()).c_str());
        break;
    case EQUIP_NONE_LEFT:
        soundPlay(SOUND_ERROR);
        screenMessage("\n%cKEINE ]BRIG!%c\n", FG_GREY, FG_WHITE);
        break;
    case EQUIP_CLASS_RESTRICTED:
        soundPlay(SOUND_ERROR);
        screenMessage("%s\n", uppercase(a->getName()).c_str());
        screenMessage(
            "\n%cEIN%s %s DARF %s BENUTZEN!%c\n",
            FG_GREY,
            (p->getSex() == SEX_FEMALE ? "E" : ""),
            uppercase(
                getClassNameTranslated(p->getClass(), p->getSex())
            ).c_str(),
            uppercase(a->getNeg()).c_str(),
            FG_WHITE
        );
        break;
    }
} // wearArmor


/**
 * Called when the player selects a party member for ztats
 */
static void ztatsFor(int player)
{
    // get the player if not provided
    if (player == -1) {
        screenMessage("Info f}r-");
        player = gameGetPlayer(true, false, true);
        if (player == -1) {
            return;
        }
    }
    screenMessage("\n");
    // Reset the reagent spell mix menu by removing
    // the menu highlight from the current item, and
    // hiding reagents that you don't have
    c->stats->resetReagentsMenu();
    c->stats->setView(StatsView(STATS_CHAR1 + player));
    ZtatsController ctrl;
    eventHandler->pushController(&ctrl);
    ctrl.waitFor();
    c->stats->setView(STATS_PARTY_OVERVIEW);
}


/**
 * This function is called every quarter second.
 */
void GameController::timerFired()
{
    if (pausedTimer > 0) {
        pausedTimer--;
        if (pausedTimer <= 0) {
            pausedTimer = 0;
            paused = false; /* unpause the game */
        }
    }
    if (!paused && !pausedTimer) {
        // From u4apple2: 1/256 chance of wind change on every refresh cycle
        // u4appke2 refresh cycles happen about 6.4 times per second
        // since we use 4 second refresh cyles we use 1/160 chance instead
        // Wind never changes to opposite direction
        if ((xu4_random(160) == 0) && !c->windLock) {
            if (xu4_random(2) == 0) {
                c->windDirection =
                    dirRotateCW(static_cast<Direction>(c->windDirection));
            } else {
                c->windDirection =
                    dirRotateCCW(static_cast<Direction>(c->windDirection));
            }
        }
        /* balloon moves about 4.8 times per second on u4apple2
           i.e. 3 of 4 refresh cycles. We move it 4 times per second which
           is close enough for me */
        if ((c->transportContext == TRANSPORT_BALLOON)
            && c->party->isFlying()) {
            c->location->move(
                dirReverse(static_cast<Direction>(c->windDirection)),
                false
            );
        }
        updateMoons(true);
        screenCycle();
        /*
         * refresh the screen only if the timer queue is empty --
         * i.e. drop a frame if another timer event is about to be fired
         */
        if (eventHandler->timerQueueEmpty()) {
            gameUpdateScreen();
        }
        /*
         * force pass if no commands within last 20 seconds
         */
        if (c->willPassTurn
            && (eventHandler->getController() != nullptr)
            && ((eventHandler->getController() == game)
                || (dynamic_cast<CombatController *>(
                        eventHandler->getController()
                    ) != nullptr))
            && (gameTimeSinceLastCommand() > 20)) {
            /* pass the turn, and redraw the text area so the prompt is
               shown */
            if (eventHandler->getController()) {
                eventHandler->getController()->keyPressed(U4_SPACE);
            }
            screenRedrawTextArea(
                TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H
            );
        }
    }
} // GameController::timerFired


/**
 * Checks the hull integrity of the ship and handles
 * the ship sinking, if necessary
 */
void gameCheckHullIntegrity()
{
    int i;
    bool killAll = false;

    /* see if the ship has sunk */
    if ((c->transportContext == TRANSPORT_SHIP)
        && (c->saveGame->shiphull <= 0)) {
        screenMessage("\nDEIN SCHIFF SINKT!\n\n");
        killAll = true;
    }
    /* On foot in the water without ship, no land around */
    if (!collisionOverride
        && (c->transportContext == TRANSPORT_FOOT)
        && c->location->map->tileTypeAt(
            c->location->coords, WITHOUT_OBJECTS
        )->isSailable()
        && !c->location->map->tileTypeAt(
            c->location->coords, WITH_GROUND_OBJECTS
        )->isShip()
        && !c->location->map->getValidMoves(
            c->location->coords, c->party->getTransport()
        )) {
        screenMessage(
            "\nOHNE DEIN SCHIFF AUF SEE GEFANGEN, ERTRINKST DU!\n\n"
        );
        killAll = true;
    }
    if (killAll) {
        for (i = 0; i < c->party->size(); i++) {
            c->party->member(i)->setHp(0);
            c->party->member(i)->setStatus(STAT_DEAD);
        }
        c->stats->update();
        screenRedrawScreen();
        musicMgr->pause();
        soundPlay(SOUND_WHIRLPOOL, false, -1, true);
        deathStart(0);
    }
} // gameCheckHullIntegrity


/**
 * Checks for valid conditions and handles
 * special creatures guarding the entrance to the
 * abyss and to the shrine of humility
 */
void GameController::checkSpecialCreatures(Direction dir)
{
    int i;
    Object *obj;
    static const struct {
        int x, y;
        Direction dir;
    } pirateInfo[] = {
        { 224, 220, DIR_EAST }, /* N'M" O'A" */
        { 224, 228, DIR_EAST }, /* O'E" O'A" */
        { 226, 220, DIR_EAST }, /* O'E" O'C" */
        { 227, 228, DIR_EAST }, /* O'E" O'D" */
        { 228, 227, DIR_SOUTH }, /* O'D" O'E" */
        { 229, 225, DIR_SOUTH }, /* O'B" O'F" */
        { 229, 223, DIR_NORTH }, /* N'P" O'F" */
        { 228, 222, DIR_NORTH } /* N'O" O'E" */
    };
    /*
     * if heading east into pirates cove (O'A" N'N"), generate pirate
     * ships
     */
    if ((dir == DIR_EAST)
        && (c->location->coords.x == 0xdd)
        && (c->location->coords.y == 0xe0)) {
        creatureCleanup(true);
        for (i = 0; i < 8; i++) {
            obj = c->location->map->addCreature(
                creatureMgr->getById(PIRATE_ID),
                MapCoords(pirateInfo[i].x,
                          pirateInfo[i].y)
            );
            obj->setDirection(pirateInfo[i].dir);
        }
    }
    /*
     * if heading south towards the shrine of humility, generate
     * daemons unless horn has been blown
     */
    if ((dir == DIR_SOUTH)
        && (c->location->coords.x >= 229)
        && (c->location->coords.x < 234)
        && (c->location->coords.y >= 212)
        && (c->location->coords.y < 217)
        && (*c->aura != Aura::HORN)) {
        creatureCleanup(true);
        for (i = 0; i < 8; i++) {
            c->location->map->addCreature(
                creatureMgr->getById(DAEMON_ID),
                MapCoords(
                    231, c->location->coords.y + 1, c->location->coords.z
                )
            );
        }
    }
} // GameController::checkSpecialCreatures


/**
 * Checks for and handles when the avatar steps on a moongate
 */
bool GameController::checkMoongates()
{
    Coords dest;
    int trammel, felucca;
    trammel = c->saveGame->trammelphase;
    felucca = c->saveGame->feluccaphase;
    if (moongateFindActiveGateAt(
            trammel, felucca, c->location->coords, dest
        )) {
        // Default spell effect (screen inversion, no 'spell' sound effects)
        gameSpellEffect(-1, -1, SOUND_MOONGATE);
        EventHandler::simulateDiskLoad(2000);
        c->location->coords = dest;
        gameSpellEffect(-1, -1, SOUND_MOONGATE); // Again, after arriving
        if (moongateIsEntryToShrineOfSpirituality(trammel, felucca)) {
            Shrine *shrine_spirituality;
            shrine_spirituality =
                dynamic_cast<Shrine *>(mapMgr->get(MAP_SHRINE_SPIRITUALITY));
            if (!c->party->canEnterShrine(VIRT_SPIRITUALITY)) {
                return true;
            }
            setMap(shrine_spirituality, 1, nullptr);
            musicMgr->play();
            shrine_spirituality->enter();
        }
        return true;
    }
    return false;
} // GameController::checkMoongates


/**
 * Fixes objects initially loaded by saveGameMonstersRead,
 * and alters movement behavior accordingly to match the creature
 */
static void gameFixupObjects(Map *map)
{
    int i;
    Object *obj;
    int z;
    /* add stuff from the monster table to the map */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        SaveGameMonsterRecord *monster = &map->monsterTable[i];
        if (monster->prevTile != 0) {
            z = (map->type == Map::DUNGEON) ? monster->z : 0;
            Coords coords(monster->x, monster->y, z);
            // tile values stored in monsters.sav hardcoded to index
            // into base tilemap
            MapTile tile = TileMap::get("base")->translate(monster->tile),
                oldTile = TileMap::get("base")->translate(monster->prevTile);
            int limitForCreatures =
                map->type == Map::DUNGEON ?
                MONSTERTABLE_SIZE :
                MONSTERTABLE_CREATURES_SIZE;

            if (i < limitForCreatures) {
                Creature *creature = creatureMgr->getByTile(tile);
                /* make sure we really have a creature */
                if (creature) {
                    obj = map->addCreature(creature, coords);
                } else {
                    std::fprintf(
                        stderr,
                        "Error: A non-creature object was found in the "
                        "creature section of the monster table. (Tile: %s)\n",
                        tile.getTileType()->getName().c_str()
                    );
                    obj = map->addObject(tile, oldTile, coords);
                }
            } else {
                obj = map->addObject(tile, oldTile, coords);
            }
            /* set the map for our object */
            obj->setMap(map);
            /* set tile / prevtile, fixes pirate ship direction */
            obj->setTile(tile);
            obj->setPrevTile(oldTile);
            /* CHANGE: If first object in inanimate objects
               section is a ship, it becomes the lastShip */
            if (i == MONSTERTABLE_CREATURES_SIZE) {
                if (tile.getTileType()->isShip()) {
                    c->lastShip = obj;
                }
            }
        }
    }
} // gameFixupObjects

static std::time_t gameTimeSinceLastCommand()
{
    return std::time(nullptr) - c->lastCommandTime;
}


/**
 * Handles what happens when a creature attacks you
 */
static void gameCreatureAttack(Creature *m)
{
    Object *under;
    const Tile *ground;
    screenMessage("\nANGRIFF DURCH %s\n", uppercase(m->getName()).c_str());
    /// TODO: CHEST: Make a user option to not make chests change battlefield
    /// map (2 of 2)
    ground =
        c->location->map->tileTypeAt(c->location->coords, WITH_GROUND_OBJECTS);
    if (!ground->isChest()) {
        ground = c->location->map->tileTypeAt(
            c->location->coords, WITHOUT_OBJECTS
        );
        if ((under = c->location->map->objectAt(c->location->coords))
            && under->getTile().getTileType()->isShip()) {
            ground = under->getTile().getTileType();
        }
    }
    EventHandler::simulateDiskLoad(1000, false);
    CombatController *cc = new CombatController(
        CombatMap::mapForTile(
            ground, c->party->getTransport().getTileType(), m
        )
    );
    cc->init(m);
    musicMgr->play();
    cc->begin();
}


/**
 * Performs a ranged attack for the creature at x,y on the world map
 */
bool creatureRangeAttack(const Coords &coords, Creature *m)
{
    // int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;
    // Figure out what the ranged attack should look like
    MapTile tile(
        c->location->map->tileset->getByName(
            (m && !m->getWorldrangedtile().empty()) ?
            m->getWorldrangedtile() :
            "hit_flash"
        )->getId()
    );
    GameController::flashTile(coords, tile, 1);
    // See if the attack hits the avatar
    Object *obj = c->location->map->objectAt(coords);
    m = dynamic_cast<Creature *>(obj);
    // Does the attack hit the avatar?
    if (coords == c->location->coords) {
        /* always displays as a 'hit' */
        GameController::flashTile(coords, tile, 4);
        /* Actual damage from u4dos */
        c->party->applyEffect(EFFECT_FIRE);
        return true;
    }
    // Destroy objects that were hit
    else if (obj) {
        if (((obj->getType() == Object::CREATURE) && m->isAttackable())
            || (obj->getType() == Object::UNKNOWN)) {
            GameController::flashTile(coords, tile, 4);
            c->location->map->removeObject(obj);
            return true;
        }
    }
    return false;
} // creatureRangeAttack


/**
 * Gets the path of coordinates for an action.  Each tile in the
 * direction specified by dirmask, between the minimum and maximum
 * distances given, is included in the path, until blockedPredicate
 * fails.  If a tile is blocked, that tile is included in the path
 * only if includeBlocked is true.
 */
std::vector<Coords> gameGetDirectionalActionPath(
    int dirmask,
    int validDirections,
    const Coords &origin,
    int minDistance,
    int maxDistance,
    bool (*blockedPredicate)(const Tile *tile),
    bool includeBlocked
)
{
    std::vector<Coords> path;
    Direction dirx = DIR_NONE, diry = DIR_NONE;
    /* Figure out which direction the action is going */
    if (DIR_IN_MASK(DIR_WEST, dirmask)) {
        dirx = DIR_WEST;
    } else if (DIR_IN_MASK(DIR_EAST, dirmask)) {
        dirx = DIR_EAST;
    }
    if (DIR_IN_MASK(DIR_NORTH, dirmask)) {
        diry = DIR_NORTH;
    } else if (DIR_IN_MASK(DIR_SOUTH, dirmask)) {
        diry = DIR_SOUTH;
    }
    /*
     * try every tile in the given direction, up to the given range.
     * Stop when the the range is exceeded, or the action is blocked.
     */
    MapCoords t_c(origin);
    if (((dirx <= 0) || DIR_IN_MASK(dirx, validDirections))
        && ((diry <= 0) || DIR_IN_MASK(diry, validDirections))) {
        for (int distance = 0;
             distance <= maxDistance;
             distance++,
                 t_c.move(dirx, c->location->map),
                 t_c.move(diry, c->location->map)) {
            if (distance >= minDistance) {
                /* make sure our action isn't taking us off the map */
                if (MAP_IS_OOB(c->location->map, t_c)) {
                    break;
                }
                const Tile *tile =
                    c->location->map->tileTypeAt(t_c, WITH_GROUND_OBJECTS);
                /* should we see if the action is blocked before trying it? */
                if (!includeBlocked
                    && blockedPredicate
                    && !(*(blockedPredicate))(tile)) {
                    break;
                }
                path.push_back(t_c);
                /* see if the action was blocked only if it did not succeed */
                if (includeBlocked
                    && blockedPredicate
                    && !(*(blockedPredicate))(tile)) {
                    break;
                }
            }
        }
    }
    return path;
} // gameGetDirectionalActionPath


/**
 * Deals an amount of damage between 'minDamage' and 'maxDamage'
 * to each party member with a 50% chance for each member to
 * avoid the damage. If (minDamage == -1) or (minDamage >= maxDamage),
 * deals 'maxDamage' damage to each member.
 */
void gameDamageParty(int minDamage, int maxDamage)
{
    int i;
    int damage;
    int lastdmged = -1;
    extern std::atomic_bool deathSequenceRunning;
    if (deathSequenceRunning) {
        /* none of this makes sense if party is already dead */
        return;
    }
    soundPlay(SOUND_PC_STRUCK, false, -1, true);
    for (i = 0; i < c->party->size(); i++) {
        if (c->party->member(i)->getStatus() != STAT_DEAD
            && xu4_random(2) != 0) {
            damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
                xu4_random((maxDamage + 1) - minDamage) + minDamage :
                maxDamage;
            c->party->member(i)->applyDamage(damage);
            c->stats->highlightPlayer(i);
            soundPlay(SOUND_NPC_STRUCK, false);
            screenShake(1);
            lastdmged = i;
            EventHandler::sleep(50);
        }
    }
    // Un-highlight the last player
    if (lastdmged != -1) {
        c->stats->highlightPlayer(lastdmged);
    }
} // gameDamageParty


/**
 * Deals an amount of damage between 'minDamage' and 'maxDamage'
 * to the ship.  If (minDamage == -1) or (minDamage >= maxDamage),
 * deals 'maxDamage' damage to the ship.
 */
void gameDamageShip(int minDamage, int maxDamage)
{
    int damage;
    extern std::atomic_bool deathSequenceRunning;
    if (deathSequenceRunning) {
        /* none of this makes sense if party is already dead */
        return;
    }
    if (c->transportContext == TRANSPORT_SHIP) {
        damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
            xu4_random((maxDamage + 1) - minDamage) + minDamage :
            maxDamage;
        soundPlay(SOUND_PC_STRUCK, false);
        screenShake(1);
        c->party->damageShip(damage);
        gameCheckHullIntegrity();
    }
}


/**
 * Sets (or unsets) the active player
 */
void gameSetActivePlayer(int player)
{
    if (player == -1) {
        c->party->setActivePlayer(-1);
        screenMessage("Set Active Player: None!\n");
    } else if (player < c->party->size()) {
        screenMessage(
            "Set Active Player: %s!\n",
            c->party->member(player)->getName().c_str()
        );
        if (c->party->member(player)->isDisabled()) {
            screenMessage("Disabled!\n");
        } else {
            c->party->setActivePlayer(player);
        }
    }
}


/**
 * Removes creatures from the current map if they are too far away from
 * the avatar, or removes all creatures to make room for special creatures
 */
void GameController::creatureCleanup(bool allCreatures)
{
    ObjectDeque::iterator i;
    Map *map = c->location->map;
    for (i = map->objects.begin(); i != map->objects.end();) {
        Object *obj = *i;
        MapCoords o_coords = obj->getCoords();
        unsigned int globalX = o_coords.x >> 4u;
        unsigned int globalY = o_coords.y >> 4u;
        if ((obj->getType() == Object::CREATURE)
            && (allCreatures ||
                (
                    (o_coords.z == c->location->coords.z)
                    && (
                        (
                            (globalX != c->location->coords.active_x)
                            && (globalX !=
                                (((c->location->coords.active_x) + 1u) & 0xFu))
                        )
                        || (
                            (globalY != c->location->coords.active_y)
                            && (globalY !=
                                (((c->location->coords.active_y) + 1u) & 0xFu))
                        )
                    )
                )
               )
           ) {
            /* delete the object and remove it from the map */
            i = map->removeObject(i);
        } else {
            i++;
        }
    }
}

static int maxCreaturesPerLevel(int level)
{
    // not quite the same algorithm as in u4dos or u4apple2
    // but the two differ as well, and the u4dos algorithm
    // where each level has specfic but overlapping ranges
    // in the fixed-size monster table doesn't map well to
    // our object deques.
    // this allows a maximum of 20 monsters per dungeon.
    // u4dos allows 28, u4apple2 just 16
    static int c_per_l[8] = {1, 1, 2, 2, 3, 3, 4, 4};
    return c_per_l[level];
}

/**
 * Checks creature conditions and spawns new creatures if necessary
 */
void GameController::checkRandomCreatures()
{
    int i;
    bool isDungeon = c->location->context & CTX_DUNGEON;
    bool canSpawnHere =
        c->location->map->isWorldMap() || isDungeon;
    int spawnDivisor = isDungeon ?
#if  0
        (32 - (c->location->coords.z << 2)) :
#else
        1 :
#endif
        16;
    int numberOfCreatures =
        c->location->map->getNumberOfCreatures(
            isDungeon ? c->location->coords.z : -1
        );
    int maxCreatures =
        isDungeon ?
        maxCreaturesPerLevel(c->location->coords.z) :
        MAX_CREATURES_ON_MAP;
    /* If there are too many creatures already,
     * or we're not on outdoors/in a dungeon, don't worry about it! */
    if (
        (!canSpawnHere)
        || (numberOfCreatures >= maxCreatures)
        || (xu4_random(spawnDivisor) != 0)
    ) {
        return;
    }
    for (i = numberOfCreatures; i < maxCreatures; i++) {
        gameSpawnCreature(nullptr);
    }
}


/**
 * Handles trolls under bridges
 */
void GameController::checkBridgeTrolls()
{
    const Tile *bridge = c->location->map->tileset->getByName("bridge");
    if (!bridge) {
        return;
    }
    // TODO: CHEST: Make a user option to not make chests block bridge trolls
    if (!c->location->map->isWorldMap()
        || (c->location->map->tileAt(c->location->coords, WITH_OBJECTS)->id
            != bridge->getId())
        || (xu4_random(8) != 0)) {
        return;
    }
    screenMessage("\nBR]CKENTROLLE!\n");
    Creature *m = c->location->map->addCreature(
        creatureMgr->getById(TROLL_ID), c->location->coords
    );
    CombatController *cc = new CombatController(MAP_BRIDGE_CON);
    cc->init(m);
    cc->begin();
}


/**
 * Check the levels of each party member while talking to Lord British
 */
static void gameLordBritishCheckLevels()
{
    bool advanced = false;
    for (int i = 0; i < c->party->size(); i++) {
        PartyMember *player = c->party->member(i);
        if (player->getRealLevel() < player->getMaxLevel()) {
            // add an extra space to separate messages
            if (!advanced) {
                ReadChoiceController::get("");
                screenMessage("\n");
                advanced = true;
            }
        }
        player->advanceLevel();
    }
    screenMessage("\nDEIN BEGEHR:\n?");
}


/**
 * Spawns a creature (m) just offscreen of the avatar.
 * If (m==nullptr) then it finds its own creature to spawn and spawns it.
 */
bool gameSpawnCreature(const Creature *m)
{
    static const int MAX_TRIES = 0x100;
    const Creature *creature;
    MapCoords coords = c->location->coords;

    if (c->location->context & CTX_DUNGEON) {
        /* FIXME: for some reason dungeon monsters aren't spawning
           correctly */
        bool found = false;
        MapCoords new_coords;
        new_coords = MapCoords(
            xu4_random(c->location->map->width),
            xu4_random(c->location->map->height),
            coords.z
        );
        const Tile *tile =
            c->location->map->tileTypeAt(new_coords, WITH_OBJECTS);
        // U4DOS: Dungeon monsters spawn only on unoccupied floor tiles
        if (tile->isDungeonFloor()) {
            found = true;
        }
        if (!found) {
            return false;
        }
        coords = new_coords;
    } else { // not dungeon
        unsigned int x, y;
        int dx, dy;
        bool ok = false;
        int tries = 0;
        while (!ok && (tries < MAX_TRIES)) {
            x = (c->location->coords.active_x << 4u) + xu4_random(32);
            y = (c->location->coords.active_y << 4u) + xu4_random(32);
            x &= 0xFFu;
            y &= 0xFFu;
            dx = static_cast<int>(x) - coords.x;
            dy = static_cast<int>(y) - coords.y;
           // Make sure it's not in view of the player if not cheat-summoned
            if (
                !m
                && std::abs(dx) <= (VIEWPORT_W / 2)
                && std::abs(dy) <= (VIEWPORT_H / 2)
            ) {
                return false;
            }
            /* make sure we can spawn the creature there */
            if (m) {
                MapCoords new_coords = coords;
                new_coords.move(dx, dy, c->location->map);
                const Tile *tile =
                    c->location->map->tileTypeAt(new_coords, WITHOUT_OBJECTS);
                if ((m->sails() && tile->isSailable())
                    || (m->swims() && tile->isSwimable())
                    || (m->walks()
                        && tile->isCreatureWalkable()
                        && tile->willWanderOn())
                    || (m->flies() && tile->isFlyable())) {
                    ok = true;
                } else {
                    tries++;
                }
            } else {
                ok = true;
            }
        }
        if (ok) {
            coords.move(dx, dy, c->location->map);
        }
    }
    /* can't spawn creatures on top of the player */
    if (coords == c->location->coords) {
        return false;
    }
    /* figure out what creature to spawn */
    if (m) {
        creature = m;
    } else if (c->location->context & CTX_DUNGEON) {
        creature = creatureMgr->randomForDungeon(c->location->coords.z);
    } else {
        creature = creatureMgr->randomForTile(
            c->location->map->tileTypeAt(coords, WITHOUT_OBJECTS)
        );
    }
    if (creature) {
        c->location->map->addCreature(creature, coords);
        return true;
    } else {
        return false;
    }
} // gameSpawnCreature


/**
 * Destroys all creatures on the current map.
 */
void gameDestroyAllCreatures(void)
{
    int i;
    gameSpellEffect('t', -1, SOUND_MAGIC); /* same effect as tremor */
    if (c->location->context & CTX_COMBAT) {
        /* destroy all creatures in combat */
        for (i = 0; i < AREA_CREATURES; i++) {
            CombatMap *cm = getCombatMap();
            CreatureVector creatures = cm->getCreatures();
            CreatureVector::iterator obj;
            for (obj = creatures.begin(); obj != creatures.end(); obj++) {
                if ((*obj)->getId() != LORDBRITISH_ID) {
                    cm->removeObject(*obj);
                }
            }
        }
    } else {
        /* destroy all creatures on the map */
        ObjectDeque::iterator current;
        Map *map = c->location->map;
        for (current = map->objects.begin(); current != map->objects.end();) {
            Creature *m = dynamic_cast<Creature *>(*current);
            if (m) {
                /* the skull does not destroy Lord British */
                if (m->getId() != LORDBRITISH_ID) {
                    current = map->removeObject(current);
                } else {
                    current++;
                }
            } else {
                current++;
            }
        }
    }
    /* alert the guards! Really, the only one left should be LB himself :) */
    c->location->map->alertGuards();
} // gameDestroyAllCreatures


/**
 * Creates the balloon near Hythloth, but only if the balloon doesn't already
 * exist somewhere
 */
bool GameController::createBalloon(Map *map)
{
    ObjectDeque::iterator i;
    /* see if the balloon has already been created (and not destroyed) */
    for (i = map->objects.begin(); i != map->objects.end(); i++) {
        Object *obj = *i;
        if (obj->getTile().getTileType()->isBalloon()) {
            return false;
        }
    }
    const Tile *balloon = map->tileset->getByName("balloon");
    ASSERT(balloon, "no balloon tile found in tileset");
    map->addObject(
        balloon->getId(), balloon->getId(), map->getLabel("balloon")
    );
    return true;
}


// Colors assigned to reagents based on my best reading of them
// from the book of wisdom.  Maybe we could use BOLD to distinguish
// the two grey and the two red reagents.
const int colors[] = {
    FG_YELLOW, FG_GREY, FG_BLUE, FG_WHITE, FG_RED, FG_GREY, FG_GREEN, FG_RED
};
void showMixturesSuper(int page = 0)
{
    screenTextColor(FG_WHITE);
    for (int i = 0; i < 13; i++) {
        char buf[4];
        const Spell *s = getSpell(i + 13 * page);
        int line = i + 8;
        screenTextAt(2, line, "%s", s->name);
        std::snprintf(buf, 4, "%3hd", c->saveGame->mixtures[i + 13 * page]);
        screenTextAt(6, line, "%s", buf);
        screenShowChar(32, 9, line);
        int comp = s->components;
        for (int j = 0; j < 8; j++) {
            screenTextColor(colors[j]);
            screenShowChar(
                comp & (1 << j) ? CHARSET_BULLET : ' ', 10 + j, line
            );
        }
        screenTextColor(FG_WHITE);
        std::snprintf(buf, 3, "%2d", s->mp);
        screenTextAt(19, line, "%s", buf);
    }
}

#if 0
static void mixReagentsSuper()
{
    screenMessage("Mische Reagenzien\n");
    static int page = 0;
    struct ReagentShop {
        const char *name;
        int price[6];
    };
    ReagentShop shops[] = {
        { "BuccDen", { 6, 7, 9, 9, 9, 1 }
        }, { "Moonglo", { 2, 5, 6, 3, 6, 9 }
        }, { "Paws", { 3, 4, 2, 8, 6, 7 }
        }, { "SkaraBr", { 2, 4, 9, 6, 4, 8 }
        },
    };
    const int shopcount = sizeof(shops) / sizeof(shops[0]);
    int oldlocation = c->location->viewMode;
    c->location->viewMode = VIEW_MIXTURES;
    screenUpdate(&game->mapArea, true, true);
    screenTextAt(16, 2, "%s", "<-L{den");
    c->stats->setView(STATS_REAGENTS);
    screenTextColor(FG_PURPLE);
    screenTextAt(2, 7, "%s", "SPELL # Reagenz  MP");
    for (int i = 0; i < shopcount; i++) {
        int line = i + 1;
        ReagentShop *s = &shops[i];
        screenTextColor(FG_WHITE);
        screenTextAt(2, line, "%s", s->name);
        for (int j = 0; j < 6; j++) {
            screenTextColor(colors[j]);
            screenShowChar('0' + s->price[j], 10 + j, line);
        }
    }
    for (int i = 0; i < 8; i++) {
        screenTextColor(colors[i]);
        screenShowChar('A' + i, 10 + i, 6);
    }
    bool done = false;
    while (!done) {
        showMixturesSuper(page);
        screenMessage("F]R SPRUCH-");
        int spell =
            ReadChoiceController::get("abcdefghijklmnopqrstuvwxyz \033\n\r");
        if ((spell < 'a') || (spell > 'z')) {
            screenMessage("\nFERTIG.\n");
            done = true;
        } else {
            spell -= 'a';
            const Spell *s = getSpell(spell);
            screenMessage("%s\n", uppercase(s->name).c_str());
            page = (spell >= 13);
            showMixturesSuper(page);
            // how many can we mix?
            int mixQty = 99 - c->saveGame->mixtures[spell];
            int ingQty = 99;
            int comp = s->components;
            for (int i = 0; i < 8; i++) {
                if (comp & 1 << i) {
                    int reagentQty = c->saveGame->reagents[i];
                    if (reagentQty < ingQty) {
                        ingQty = reagentQty;
                    }
                }
            }
            screenMessage(
                "DU KANNST %d MISCHEN.\n", (mixQty > ingQty) ? ingQty : mixQty
            );
            screenMessage("WIE VIELE? ");
            int howmany = ReadIntController::get(
                2, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line
            );
            if (howmany == 0) {
                screenMessage("\nKEINE GEMISCHT!\n");
            } else if (howmany > mixQty) {
                soundPlay(SOUND_ERROR);
                screenMessage(
                    "\n%cDU KANNST NICHT MEHR SO VIELE VON DIESEN SPRUCH "
                    "MISCHEN!%c\n",
                    FG_GREY, FG_WHITE
                );
            } else if (howmany > ingQty) {
                soundPlay(SOUND_ERROR);
                screenMessage(
                    "\n%cDU HAST NICHT GENUG REAGENZIEN, UM %d "
                    "SPR]CHE ZU MISCHEN!%c\n",
                    FG_GREY,
                    howmany,
                    FG_WHITE
                );
            } else {
                c->saveGame->mixtures[spell] += howmany;
                for (int i = 0; i < 8; i++) {
                    if (comp & 1 << i) {
                        c->saveGame->reagents[i] -= howmany;
                    }
                }
                screenMessage("\nERFOLG!\n\n");
            }
        }
        c->stats->setView(STATS_REAGENTS);
    }
    c->location->viewMode = oldlocation;
} // mixReagentsSuper
#endif
