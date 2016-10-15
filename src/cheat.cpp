/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdlib>
#include <cstring>

#include "cheat.h"
#include "location.h"
#include "map.h"
#include "context.h"
#include "game.h"
#include "mapmgr.h"
#include "moongate.h"
#include "portal.h"
#include "player.h"
#include "screen.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"
#include "weapon.h"

CheatMenuController::CheatMenuController(GameController *game)
    :game(game)
{
}

bool CheatMenuController::keyPressed(int key)
{
    int i;
    bool valid = true;
    if ((key >= 'A') && (key <= ']')) {
        key = xu4_tolower(key);
    }
    switch (key) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        screenMessage("Tor %d!\n", key - '0');
        if (c->location->map->isWorldMap()) {
            const Coords *moongate = moongateGetGateCoordsForPhase(key - '1');
            if (moongate) {
                c->location->coords = *moongate;
            }
        } else {
            soundPlay(SOUND_ERROR);
            screenMessage("HIER NICHT!\n");
        }
        break;
    case 'a':
    {
        int newTrammelphase = c->saveGame->trammelphase + 1;
        if (newTrammelphase > 7) {
            newTrammelphase = 0;
        }
        screenMessage("MONDLAUF!\n");
        while (c->saveGame->trammelphase != newTrammelphase) {
            game->updateMoons(true);
        }
        break;
    }
    case 'c':
        collisionOverride = !collisionOverride;
        screenMessage(
            "KOLLISONSTEST %s!\n", collisionOverride ? "AUS" : "EIN"
        );
        break;
    case 'e':
        screenMessage("AUSR]STUNG!\n");
        for (i = ARMR_NONE + 1; i < ARMR_MAX; i++) {
            c->saveGame->armor[i] = 8;
        }
        for (i = WEAP_HANDS + 1; i < WEAP_MAX; i++) {
            const Weapon *weapon = Weapon::get(static_cast<WeaponType>(i));
            if (weapon->loseWhenUsed() || weapon->loseWhenRanged()) {
                c->saveGame->weapons[i] = 99;
            } else {
                c->saveGame->weapons[i] = 8;
            }
        }
        break;
    case 'f':
        screenMessage("VOLLE PUNKTE!\n");
        for (i = 0; i < c->saveGame->members; i++) {
            c->saveGame->players[i].str = 50;
            c->saveGame->players[i].dex = 50;
            c->saveGame->players[i].intel = 50;
            if (c->saveGame->players[i].hpMax < 800) {
                c->saveGame->players[i].xp = 9999;
                c->saveGame->players[i].hpMax = 800;
                c->saveGame->players[i].hp = 800;
            }
        }
        break;
    case 'g':
    {
        screenMessage("GEH ZU: ");
        std::string dest = lowercase(gameGetInput(8));
        bool found = false;
        for (unsigned int p = 0; p < c->location->map->portals.size(); p++) {
            MapId destid = c->location->map->portals[p]->destid;
            std::string destNameLower =
                lowercase(mapMgr->get(destid)->getName());
            if (destNameLower.find(dest) != std::string::npos) {
                screenMessage(
                    "\n%s\n", mapMgr->get(destid)->getName().c_str()
                );
                c->location->coords = c->location->map->portals[p]->coords;
                found = true;
                break;
            }
        }
        if (!found) {
            MapCoords coords = c->location->map->getLabel(dest);
            if (coords != MapCoords::nowhere) {
                screenMessage("\n%s\n", dest.c_str());
                c->location->coords = coords;
                found = true;
            }
        }
        if (!found) {
            screenMessage(
                "\nKANN %s\nNICHT FINDEN!\n", uppercase(dest).c_str()
            );
        }
        break;
    }
    case 'h':
    {
        screenMessage(
            "HILFE:\n" "1-8   - TOR\n"
            "F1-F8 - +TUGEND\n"
            "A - MONDLAUF\n"
            "C - KOLLISION\n"
            "E - WERKZEUG\n"
            "F - VOLLE PUNKTE\n"
            "G - GEHE ZU\n"
            "H - HILFE\n"
            "I - DINGE\n"
            "J - BEGLEITER\n"
            "K - ZEIG KARMA\n"
            "(MEHR)"
        );
        ReadChoiceController pauseController("");
        eventHandler->pushController(&pauseController);
        pauseController.waitFor();
        screenMessage(
            "\n"
            "L - ORT\n"
            "M - MIXTUREN\n"
            "O - OPAZIT[T\n"
            "P - PEER\n"
            "R - REAGENZIEN\n"
            "S - BESCHW\\RE\n"
            "T - TRANSPORTE\n"
            "V - VOLLTUGEND\n"
            "W - WIND\n"
            "X - VERLASSEN\n"
            "Y - STEIG AUF\n"
            "(MEHR)"
        );
        eventHandler->pushController(&pauseController);
        pauseController.waitFor();
        screenMessage(
            "\n"
            "Z - STEIG AB\n"
        );
        break;
    }
    case 'i':
        screenMessage("DINGE!\n");
        c->saveGame->torches = 99;
        c->saveGame->gems = 99;
        c->saveGame->keys = 99;
        c->saveGame->sextants = 1;
        c->saveGame->items =
            ITEM_SKULL
            | ITEM_CANDLE
            | ITEM_BOOK
            | ITEM_BELL
            | ITEM_KEY_C
            | ITEM_KEY_L
            | ITEM_KEY_T
            | ITEM_HORN
            | ITEM_WHEEL;
        c->saveGame->stones = 0xff;
        c->saveGame->runes = 0xff;
        c->saveGame->food = 999900;
        c->saveGame->gold = 9999;
        c->stats->update();
        break;
    case 'j':
        screenMessage("BEGLEITER!\n");
        for (int m = c->saveGame->members; m < 8; m++) {
            if (c->party->canPersonJoin(c->saveGame->players[m].name, NULL)) {
                c->party->join(c->saveGame->players[m].name);
            }
        }
        c->stats->update();
        break;
    case 'k':
        screenMessage("KARMA!\n\n");
        for (i = 0; i < 8; i++) {
            unsigned int j;
            screenMessage("%s:", getVirtueName(static_cast<Virtue>(i)));
            for (j = 13;
                 j > std::strlen(getVirtueName(static_cast<Virtue>(i)));
                 j--) {
                screenMessage(" ");
            }
            if (c->saveGame->karma[i] > 0) {
                screenMessage("%.2d\n", c->saveGame->karma[i]);
            } else {
                screenMessage("--\n");
            }
        }
        break;
    case 'l':
        if (c->location->map->isWorldMap()) {
            screenMessage(
                "\nORT:\nWELTKARTE\nX: %d\nY: %d\n",
                c->location->coords.x,
                c->location->coords.y
            );
        } else {
            screenMessage(
                "\nORT:\n%s\nX: %d\nY: %d\nZ: %d\n",
                c->location->map->getName().c_str(),
                c->location->coords.x,
                c->location->coords.y,
                c->location->coords.z
            );
        }
        break;
    case 'm':
        screenMessage("MIXTUREN!\n");
        for (i = 0; i < SPELL_MAX; i++) {
            c->saveGame->mixtures[i] = 99;
        }
        break;
    case 'o':
        c->opacity = !c->opacity;
        screenMessage("OPAZIT[T %s!\n", c->opacity ? "EIN" : "AUS");
        break;
    case 'p':
        if ((c->location->viewMode == VIEW_NORMAL)
            || (c->location->viewMode == VIEW_DUNGEON)) {
            c->location->viewMode = VIEW_GEM;
        } else if (c->location->context == CTX_DUNGEON) {
            c->location->viewMode = VIEW_DUNGEON;
        } else {
            c->location->viewMode = VIEW_NORMAL;
        }
        screenMessage("\nANSICHT!\n");
        break;
    case 'r':
        screenMessage("REAGENZIEN!\n");
        for (i = 0; i < REAG_MAX; i++) {
            c->saveGame->reagents[i] = 99;
        }
        break;
    case 's':
        screenMessage("BESCHW\\RE!\n");
        screenMessage("WAS?\n");
        summonCreature(gameGetInput());
        break;
    case 't':
        if (c->location->map->isWorldMap()) {
            MapCoords coords = c->location->coords;
            static MapTile
                horse = c->location->map->tileset->getByName("horse")->getId(),
                ship = c->location->map->tileset->getByName("ship")->getId(),
                balloon = c->location->map->tileset->getByName("balloon")
                ->getId();
            MapTile *choice;
            Tile *tile;
            screenMessage("ERZEUGE TRASNPORT!\nWELCHEN? ");
            // Get the transport of choice
            char transport = ReadChoiceController::get("spb \033\015");
            switch (transport) {
            case 's':
                choice = &ship;
                break;
            case 'p':
                choice = &horse;
                break;
            case 'b':
                choice = &balloon;
                break;
            default:
                choice = NULL;
                break;
            }
            if (choice) {
                ReadDirController readDir;
                tile = c->location->map->tileset->get(choice->getId());
                screenMessage("\n%s\n", tile->getName().c_str());
                // Get the direction in which to
                // create the transport
                eventHandler->pushController(&readDir);
                screenMessage("WOHIN-");
                coords.move(readDir.waitFor(), c->location->map);
                if (coords != c->location->coords) {
                    bool ok = false;
                    MapTile *ground =
                        c->location->map->tileAt(coords, WITHOUT_OBJECTS);
                    screenMessage(
                        "%s\n", getDirectionName(readDir.getValue())
                    );
                    switch (transport) {
                    case 's':
                        ok = ground->getTileType()->isSailable();
                        break;
                    case 'p':
                    case 'b':
                        ok = ground->getTileType()->isWalkable();
                        break;
                    default:
                        break;
                    }
                    if (choice && ok) {
                        c->location->map->addObject(*choice, *choice, coords);
                        screenMessage(
                            "%s ERZEUGT!\n", tile->getName().c_str()
                        );
                    } else if (!choice) {
                        soundPlay(SOUND_ERROR);
                        screenMessage("UNG]LTIGER\nTRANSPORT!\n");
                    } else {
                        soundPlay(SOUND_ERROR);
                        screenMessage(
                            "KANN %s NICHT PLATZIEREN!\n",
                            tile->getName().c_str()
                        );
                    }
                }
            } else {
                screenMessage("KEINER!\n");
            }
        }
        break;
    case 'v':
        screenMessage("\nVOLLE TUGEND!\n");
        for (i = 0; i < 8; i++) {
            c->saveGame->karma[i] = 0;
        }
        c->stats->update();
        break;
    case 'w':
    {
        screenMessage("WINDRICHTUNG ('L' zum SPERREN):\n");
        WindCmdController ctrl;
        eventHandler->pushController(&ctrl);
        ctrl.waitFor();
        break;
    }
    case 'x':
        screenMessage("\nVERLASSE!\n");
        if (!game->exitToParentMap()) {
            soundPlay(SOUND_ERROR);
            screenMessage("HIER NICHT!\n");
        }
        musicMgr->play();
        break;
    case 'y':
        screenMessage("STEIG AUF!\n");
        if ((c->location->context & CTX_DUNGEON)
            && (c->location->coords.z > 0)) {
            c->location->coords.z--;
        } else {
            screenMessage("VERLASSE...\n");
            game->exitToParentMap();
            musicMgr->play();
        }
        break;
    case 'z':
        screenMessage("STEIG AB!\n");
        if ((c->location->context & CTX_DUNGEON)
            && (c->location->coords.z < 7)) {
            c->location->coords.z++;
        } else {
            soundPlay(SOUND_ERROR);
            screenMessage("HIER NICHT!\n");
        }
        break;
    case U4_FKEY + 0:
    case U4_FKEY + 1:
    case U4_FKEY + 2:
    case U4_FKEY + 3:
    case U4_FKEY + 4:
    case U4_FKEY + 5:
    case U4_FKEY + 6:
    case U4_FKEY + 7:
        screenMessage(
            "STEIGERE %s!\n", getVirtueName(static_cast<Virtue>(key - U4_FKEY))
        );
        if (c->saveGame->karma[key - U4_FKEY] == 99) {
            c->saveGame->karma[key - U4_FKEY] = 0;
        } else if (c->saveGame->karma[key - U4_FKEY] != 0) {
            c->saveGame->karma[key - U4_FKEY] += 10;
        }
        if (c->saveGame->karma[key - U4_FKEY] > 99) {
            c->saveGame->karma[key - U4_FKEY] = 99;
        }
        c->stats->update();
        break;
    case U4_ESC:
    case U4_ENTER:
    case U4_SPACE:
        screenMessage("NICHTS!\n");
        break;
    default:
        valid = false;
        break;
    } // switch
    if (valid) {
        doneWaiting();
        screenPrompt();
    }
    return valid;
} // CheatMenuController::keyPressed


/**
 * Summons a creature given by 'creatureName'. This can either be given
 * as the creature's name, or the creature's id.  Once it finds the
 * creature to be summoned, it calls gameSpawnCreature() to spawn it.
 */
void CheatMenuController::summonCreature(const std::string &name)
{
    const Creature *m = NULL;
    std::string creatureName = name;
    trim(creatureName);
    if (creatureName.empty()) {
        screenMessage("\n");
        return;
    }
    /* find the creature by its id and spawn it */
    unsigned int id = std::atoi(creatureName.c_str());
    if (id > 0) {
        m = creatureMgr->getById(id);
    }
    if (!m) {
        m = creatureMgr->getByName(creatureName);
    }
    if (m) {
        if (gameSpawnCreature(m)) {
            screenMessage("\n%s BESCHWOREN!\n", m->getName().c_str());
        } else {
            soundPlay(SOUND_ERROR);
            screenMessage("\n\nKEIN PLATZ F]R %s!\n\n", m->getName().c_str());
        }
        return;
    }
    screenMessage("\n%s UNBEKANNT!\n", creatureName.c_str());
} // CheatMenuController::summonCreature

bool WindCmdController::keyPressed(int key)
{
    switch (key) {
    case U4_UP:
    case U4_LEFT:
    case U4_DOWN:
    case U4_RIGHT:
        c->windDirection = keyToDirection(key);
        screenMessage(
            "WIND %s!\n",
            getDirectionName(static_cast<Direction>(c->windDirection))
        );
        doneWaiting();
        return true;
    case 'l':
        c->windLock = !c->windLock;
        screenMessage("WINDRICHTUNG %sSPERRT!\n", c->windLock ? "GE" : "ENT");
        doneWaiting();
        return true;
    }
    return KeyHandler::defaultHandler(key, NULL);
}
