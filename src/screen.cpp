/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <atomic>
#include <cfloat>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "screen.h"

#include "config.h"
#include "context.h"
#include "coords.h"
#include "debug.h"
#include "direction.h"
#include "dungeonview.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "image.h"
#include "imagemgr.h"
#include "intro.h"
#include "location.h"
#include "map.h"
#include "names.h"
#include "savegame.h"
#include "settings.h"
#include "textcolor.h"
#include "textview.h"
#include "tile.h"
#include "tileanim.h"
#include "tileset.h"
#include "tileview.h"
#include "types.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"
#include "view.h"


namespace {
    enum LayoutType {
        LAYOUT_STANDARD,
        LAYOUT_GEM,
        LAYOUT_DUNGEON_GEM
    };


    class Layout {
    public:
        Layout()
            : type(LAYOUT_STANDARD),
              tile_shape({.width = 0, .height = 0}),
              viewport({.x = 0, .y = 0, .width = 0, .height = 0})
        {
        }

        std::string name;
        LayoutType type;
        struct {
            int width, height;
        } tile_shape;
        struct {
            int x, y;
            int width, height;
        } viewport;
    };
}

static void screenLoadGraphicsFromConf();
static Layout *screenLoadLayoutFromConf(const ConfigElement &conf);
static void screenShowGemTile(
    const Layout *layout, const Map *map, MapTile t, bool focus, int x, int y
);
static Layout *screenGetGemLayout(const Map *map);
static std::vector<Layout *> layouts;
static std::vector<TileAnimSet *> tileAnimSets;
static std::vector<std::string> gemLayoutNames;
static std::vector<std::string> filterNames;
static std::vector<std::string> lineOfSightStyles;
static Layout *gem_layout = nullptr;
static std::map<std::string, int> dungeonTileChars;
TileAnimSet *tileAnimations = nullptr;
static ImageInfo *charsetInfo = nullptr;
static ImageInfo *gemTilesInfo = nullptr;
static void screenFindLineOfSight(
    std::vector<MapTile> viewportTiles[VIEWPORT_WIDTH][VIEWPORT_HEIGHT]
);
static void screenFindLineOfSightDOS(
    std::vector<MapTile> viewportTiles[VIEWPORT_WIDTH][VIEWPORT_HEIGHT]
);
static void screenFindLineOfSightEnhanced(
    std::vector<MapTile> viewportTiles[VIEWPORT_WIDTH][VIEWPORT_HEIGHT]
);
static int screenNeedPrompt = 1;
std::atomic_int screenCurrentCycle(0);
static int screenCursorX = 0;
static int screenCursorY = 0;
static int screenCursorStatus = 0;
static int screenCursorEnabled = 1;
static int screenLos[VIEWPORT_WIDTH][VIEWPORT_HEIGHT];
static constexpr int BufferSize = 1024;
std::atomic_bool screenMoving;
// Just extern the system functions here. That way people aren't
// tempted to call them as part of the public API.
extern void screenInit_sys();
extern void screenDelete_sys();

void screenInit()
{
    screenMoving = true;
    filterNames.clear();
    filterNames.emplace_back("Point");
    filterNames.emplace_back("2xBi");
    filterNames.emplace_back("2xSaI");
    filterNames.emplace_back("Scale2x");
    lineOfSightStyles.clear();
    lineOfSightStyles.emplace_back("DOS");
    lineOfSightStyles.emplace_back("Enhanced");
    charsetInfo = nullptr;
    gemTilesInfo = nullptr;
    screenLoadGraphicsFromConf();
    if (verbose) {
        std::printf("using %s scaler\n", settings.filter.c_str());
    }
    screenInit_sys();
    /* if we can't use vga, reset to default:ega */
    if (!u4isUpgradeAvailable() && settings.videoType == "VGA") {
        settings.videoType = "EGA";
    }
    KeyHandler::setKeyRepeat(settings.keyDelay, settings.keyInterval);
    /* find the tile animations for our tileset */
    tileAnimations = nullptr;
    for (auto *tileAnimSet: tileAnimSets) {
        if (tileAnimSet->name == settings.videoType) {
            tileAnimations = tileAnimSet;
        }
    }
    if (!tileAnimations) {
        errorFatal(
            "unable to find tile animations for \"%s\" video "
            "mode in graphics.xml",
            settings.videoType.c_str()
        );
    }
    dungeonTileChars.clear();
    dungeonTileChars["unused"] = CHARSET_FLOOR;
    dungeonTileChars["brick_floor"] = CHARSET_FLOOR;
    dungeonTileChars["up_ladder"] = CHARSET_LADDER_UP;
    dungeonTileChars["down_ladder"] = CHARSET_LADDER_DOWN;
    dungeonTileChars["up_down_ladder"] = CHARSET_LADDER_UP_DOWN;
    dungeonTileChars["chest"] = '$';
    dungeonTileChars["magic_orb"] = CHARSET_ORB;
    dungeonTileChars["wind_trap"] = 'F';
    dungeonTileChars["falling_rock_trap"] = 'F';
    dungeonTileChars["pit_trap"] = 'F';
    dungeonTileChars["fountain_normal"] = 'Q';
    dungeonTileChars["fountain_healing"] = 'Q';
    dungeonTileChars["fountain_acid"] = 'Q';
    dungeonTileChars["fountain_cure"] = 'Q';
    dungeonTileChars["fountain_poison"] = 'Q';
    dungeonTileChars["secret_door"] = CHARSET_SECRET_DOOR;
    dungeonTileChars["brick_wall"] = CHARSET_WALL;
    dungeonTileChars["dungeon_door"] = CHARSET_ROOM;
    dungeonTileChars["avatar"] = CHARSET_RED_DOT;
    dungeonTileChars["dungeon_room_0"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_1"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_2"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_3"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_4"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_5"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_6"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_7"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_8"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_9"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_a"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_b"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_c"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_d"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_e"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_room_f"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_altar"] = CHARSET_ANKH;
    dungeonTileChars["dungeon_energy_field"] = '^';
    dungeonTileChars["dungeon_fire_field"] = '^';
    dungeonTileChars["dungeon_poison_field"] = '^';
    dungeonTileChars["dungeon_sleep_field"] = '^';
} // screenInit

void screenDelete()
{
    for (const auto *layout: layouts) {
        delete layout;
    }
    layouts.clear();
    for (const auto *tileAnimSet: tileAnimSets) {
        delete tileAnimSet;
    }
    screenDelete_sys();
    ImageMgr::destroy();
}


/**
 * Re-initializes the screen and implements any changes made in settings
 */
void screenReInit()
{
    /* delete intro stuff */
    intro->deleteIntro();
    /* unload tilesets, which will be reloaded lazily as needed */
    Tileset::unloadAllImages();
    ImageMgr::destroy();
    tileAnimations = nullptr;
    /* delete screen stuff */
    screenDelete();
    /* re-init screen stuff (loading new backgrounds, etc.) */
    screenInit();
    /* re-fix the backgrounds loaded and scale images, etc. */
    intro->init();
}

void screenTextAt(const int x, const int y, const char *fmt, ...)
{
    char buffer[BufferSize];
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, BufferSize, fmt, args);
    va_end(args);
    for (int i = 0; i < static_cast<int>(std::strlen(buffer)); i++) {
        screenShowChar(buffer[i], x + i, y);
    }
}

void screenPrompt()
{
    if (screenNeedPrompt && screenCursorEnabled && c->col == 0) {
        screenMessage("\n%c", CHARSET_PROMPT);
        screenNeedPrompt = 0;
    }
}

void screenMessage(const char *fmt, ...)
{
    if (!c) {
        // Because some cases (like the intro) don't have the
        // context initiated.
        return;
    }
    char buffer[BufferSize];
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, BufferSize, fmt, args);
    va_end(args);
    screenHideCursor();
    /* scroll the message area, if necessary */
    if (c->line == 12) {
        screenScrollMessageArea();
        c->line--;
    }
    for (unsigned int i = 0; i < std::strlen(buffer); i++) {
        // include whitespace and color-change codes
        const int word_len = static_cast<int>(
            std::strcspn(buffer + i, " \b\t\n\024\025\026\027\030\031")
        );
        /* backspace */
        if (buffer[i] == '\b') {
            c->col--;
            if (c->col < 0) {
                c->col += 16;
                c->line--;
            }
            continue;
        }
        /* color-change codes */
        switch (buffer[i]) {
        case FG_GREY:
        case FG_BLUE:
        case FG_PURPLE:
        case FG_GREEN:
        case FG_RED:
        case FG_YELLOW:
        case FG_WHITE:
            screenTextColor(buffer[i]);
            continue;
        default:
            ;
        }
        /* check for word wrap */
        if (c->col + word_len > 16 || buffer[i] == '\n' || c->col == 16) {
            if (buffer[i] == '\n' || buffer[i] == ' ') {
                i++;
            }
            c->line++;
            c->col = 0;
            screenMessage("%s", buffer + i);
            return;
        }
        /* code for move cursor right */
        if (buffer[i] == 0x12) {
            c->col++;
            continue;
        }
#if 0 // Not needed with fixed German language files.
        /* don't show a space in column 1.  Helps with Hawkwind. */
        if (buffer[i] == ' ' && c->col == 0) {
            continue;
        }
#endif
        screenShowChar(buffer[i], TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
        c->col++;
    }
    screenSetCursorPos(TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    screenShowCursor();
    screenNeedPrompt = 1;
} // screenMessage

const std::vector<std::string> &screenGetFilterNames()
{
    return filterNames;
}

const std::vector<std::string> &screenGetGemLayoutNames()
{
    return gemLayoutNames;
}

const std::vector<std::string> &screenGetLineOfSightStyles()
{
    return lineOfSightStyles;
}

static void screenLoadGraphicsFromConf()
{
    const Config *config = Config::getInstance();
    const std::vector<ConfigElement> graphicsConf =
#ifdef RASB_PI
        config->getElement("graphicsPi").getChildren();
#else
        config->getElement("graphics").getChildren();
#endif
    for (const auto &conf: graphicsConf) {
        if (conf.getName() == "layout") {
            layouts.push_back(screenLoadLayoutFromConf(conf));
        } else if (conf.getName() == "tileanimset") {
            tileAnimSets.push_back(new TileAnimSet(conf));
        }
    }
    gemLayoutNames.clear();

    for (const auto *layout: layouts) {
        if (layout->type == LAYOUT_GEM) {
            gemLayoutNames.push_back(layout->name);
        }
    }
    /*
     * Find gem layout to use.
     */
    for (auto *layout: layouts) {
        if (layout->type == LAYOUT_GEM
            && layout->name == settings.gemLayout) {
            gem_layout = layout;
            break;
        }
    }
    if (!gem_layout) {
        errorFatal(
            "no gem layout named %s found!\n", settings.gemLayout.c_str()
        );
    }
} // screenLoadGraphicsFromConf

static Layout *screenLoadLayoutFromConf(const ConfigElement &conf)
{
    static const char *typeEnumStrings[] = {
        "standard",
        "gem",
        "dungeon_gem",
        nullptr
    };
    auto *layout = new Layout;
    layout->name = conf.getString("name");
    layout->type = static_cast<LayoutType>(
        conf.getEnum("type", typeEnumStrings)
    );
    const std::vector<ConfigElement> children = conf.getChildren();
    for (const auto &child: children) {
        if (child.getName() == "tileshape") {
            layout->tile_shape.width = child.getInt("width");
            layout->tile_shape.height = child.getInt("height");
        } else if (child.getName() == "viewport") {
            layout->viewport.x = child.getInt("x");
            layout->viewport.y = child.getInt("y");
            layout->viewport.width = child.getInt("width");
            layout->viewport.height = child.getInt("height");
        }
    }
    return layout;
} // screenLoadLayoutFromConf

static std::vector<MapTile> screenViewportTile(
    const int width, const int height, const int x, const int y, bool &focus
)
{
    MapCoords center = c->location->coords;
    static MapTile grass =
        c->location->map->tileset->getByName("grass")->getId();

    if (c->location->map->width <= width
        && c->location->map->height <= height) {
        center.x = c->location->map->width / 2;
        center.y = c->location->map->height / 2;
    }
    MapCoords tc = center;
    tc.x += x - width / 2;
    tc.y += y - height / 2;
    /* Wrap the location if we can */
    tc.wrap(c->location->map);
    /* off the edge of the map: pad with grass tiles */
    if (MAP_IS_OOB(c->location->map, tc)) {
        focus = false;
        std::vector<MapTile> resultGrass(1, grass);
        return resultGrass;
    }
    return c->location->tilesAt(tc, focus);
} // screenViewportTile

static MapTile screenViewportTileGem(
    const int width, const int height, const int x, const int y, bool &focus
)
{
    MapCoords center(
        static_cast<int>(((c->location->coords.active_x + 1) << 4u) & 0xFF),
        static_cast<int>(((c->location->coords.active_y + 1) << 4u) & 0xFF),
        c->location->coords.z
    );
    static MapTile grass =
        c->location->map->tileset->getByName("grass")->getId();

    if (c->location->map->width <= width
        && c->location->map->height <= height) {
        center.x = c->location->map->width / 2;
        center.y = c->location->map->height / 2;
    }
    MapCoords tc = center;
    tc.x += x - width / 2;
    tc.y += y - height / 2;
    /* Wrap the location if we can */
    tc.wrap(c->location->map);
    /* off the edge of the map: pad with grass tiles */
    if (MAP_IS_OOB(c->location->map, tc)) {
        focus = false;
        return grass;
    }
    return c->location->tilesAt(tc, focus).front();
} // screenViewportTileGem

bool screenTileUpdate(TileView *view, const Coords &coords, const bool redraw)
{
    if (c->location->map->flags & FIRST_PERSON) {
        return false;
    }
    // Get the tiles
    bool focus;
    MapCoords mc(coords);
    mc.wrap(c->location->map);
    const std::vector<MapTile> tiles = c->location->tilesAt(mc, focus);
    // Get the screen coordinates
    int x = coords.x;
    int y = coords.y;
    const int width = c->location->map->width;
    const int height = c->location->map->height;
    if (width > VIEWPORT_WIDTH || height > VIEWPORT_HEIGHT) {
        // Center the coordinates to the viewport if you're on centered-view
        // map.
        // Then wrap so that cannon fire works across edge of main map.
        x -= c->location->coords.x;
        if (c->location->map->border_behavior == Map::BORDER_WRAP) {
            while (x < -(width / 2)) {
                x += width;
            }
            while (x > width / 2) {
                x -= width;
            }
        }
        x += VIEWPORT_WIDTH / 2;

        y -= c->location->coords.y;
        if (c->location->map->border_behavior == Map::BORDER_WRAP) {
            while (y < -(height / 2)) {
                y += height;
            }
            while (y > height / 2) {
                y -= height;
            }
        }
        y += VIEWPORT_HEIGHT / 2;
    }
    // Draw if it is on screen
    if (x >= 0
        && y >= 0
        && x < VIEWPORT_WIDTH
        && y < VIEWPORT_HEIGHT
        && screenLos[x][y]) {
        view->drawTile(tiles, focus, x, y);
        if (redraw) {
            screenRedrawMapArea();
        }
        return true;
    }
    return false;
} // screenTileUpdate


/**
 * Redraw the screen.  If show_map is set, the normal map is drawn in
 * the map area.  If blackout is set, the map area is blacked out. If
 * neither is set, the map area is left untouched.
 */
void screenUpdate(TileView *view, const bool show_map, const bool blackout)
{
    U4ASSERT(c != nullptr, "context has not yet been initialized");
    screenLock();
    if (blackout) {
        screenEraseMapArea();
    } else if (show_map && c->location->map->flags & FIRST_PERSON) {
        DungeonViewer.display(c, view);
        screenRedrawMapArea();
    } else if (show_map) {
        static MapTile black =
            c->location->map->tileset->getByName("black")->getId();
#if 0
        static MapTile avatar =
            c->location->map->tileset->getByName("avatar")->getId();
#endif
        int x, y;
        std::vector<MapTile> viewportTiles[VIEWPORT_WIDTH][VIEWPORT_HEIGHT];
        bool viewportFocus[VIEWPORT_WIDTH][VIEWPORT_HEIGHT];
        for (y = 0; y < VIEWPORT_HEIGHT; y++) {
            for (x = 0; x < VIEWPORT_WIDTH; x++) {
                viewportTiles[x][y] = screenViewportTile(
                    VIEWPORT_WIDTH, VIEWPORT_HEIGHT, x, y, viewportFocus[x][y]
                );
            }
        }
        screenFindLineOfSight(viewportTiles);
        for (y = 0; y < VIEWPORT_HEIGHT; y++) {
            for (x = 0; x < VIEWPORT_WIDTH; x++) {
                if (screenLos[x][y]) {
                    view->drawTile(
                        viewportTiles[x][y], viewportFocus[x][y], x, y
                    );
                } else {
                    view->drawTile(black, false, x, y);
                }
            }
        }
        screenRedrawMapArea();
    }
    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
    screenUnlock();
} // screenUpdate


/**
 * Draw an image or subimage on the screen.
 */
void screenDrawImage(const std::string &name, const int x, const int y)
{
    const ImageInfo *info = imageMgr->get(name);
    if (info) {
        info->image->alphaOn();
        info->image->draw(x, y);
        return;
    }
    const SubImage *subimage = imageMgr->getSubImage(name);
    if (subimage) {
        info = imageMgr->get(subimage->srcImageName);
        if (info) {
            info->image->alphaOn();
            info->image->drawSubRect(
                x,
                y,
                subimage->x * (settings.scale / info->prescale),
                subimage->y * (settings.scale / info->prescale),
                subimage->width * (settings.scale / info->prescale),
                subimage->height * (settings.scale / info->prescale)
            );
            return;
        }
    }
    errorFatal(
        "ERROR 1006: Unable to load the image \"%s\".\t\n\n"
        "Is %s installed?\n\nVisit the XU4 website for additional "
        "information.\n\thttp://xu4.sourceforge.net/",
        name.c_str(),
        settings.game.c_str()
    );
}

void screenDrawImageInMapArea(const std::string &name)
{
    const ImageInfo *info = imageMgr->get(name);
    if (!info) {
        errorFatal(
            "ERROR 1004: Unable to load data files.\t\n\nIs %s installed?\n\n"
            "Visit the XU4 website for additional information.\n"
            "\thttp://xu4.sourceforge.net/",
            settings.game.c_str()
        );
    }
    info->image->drawSubRect(
        BORDER_WIDTH * settings.scale,
        (BORDER_HEIGHT + 4) * settings.scale,
        BORDER_WIDTH * settings.scale,
        BORDER_HEIGHT * settings.scale,
        VIEWPORT_WIDTH * TILE_WIDTH * settings.scale,
        VIEWPORT_HEIGHT * TILE_HEIGHT * settings.scale
    );
}


/**
 * Change the current text color
 */
void screenTextColor(int color)
{
    if (charsetInfo == nullptr) {
        charsetInfo = imageMgr->get(BKGD_CHARSET);
        if (!charsetInfo) {
            errorFatal(
                "ERROR 1003: Unable to load the \"%s\" data file.\t\n\n"
                "Is %s installed?\n\nVisit the XU4 website for additional "
                "information.\n\thttp://xu4.sourceforge.net/",
                BKGD_CHARSET,
                settings.game.c_str()
            );
        }
    }
    if (!settings.enhancementsOptions.textColorization) {
        return;
    }
    switch (color) {
    case FG_GREY:
    case FG_BLUE:
    case FG_PURPLE:
    case FG_GREEN:
    case FG_RED:
    case FG_YELLOW:
    case FG_WHITE:
        charsetInfo->image->setFontColorFG(static_cast<ColorFG>(color));
        break;
    default:
        errorFatal("bad color number");
    }
}


/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(const int chr, const int x, const int y)
{
    if (charsetInfo == nullptr) {
        charsetInfo = imageMgr->get(BKGD_CHARSET);
        if (!charsetInfo) {
            errorFatal(
                "ERROR 1001: Unable to load the \"%s\" data file.\t\n\n"
                "Is %s installed?\n\nVisit the XU4 website for additional "
                "information.\n\thttp://xu4.sourceforge.net/",
                BKGD_CHARSET,
                settings.game.c_str()
            );
        }
        charsetInfo->image->alphaOff();
    }
    charsetInfo->image->drawSubRect(
        x * charsetInfo->image->width(),
        y * (CHAR_HEIGHT * settings.scale) + 4 * settings.scale,
        0,
        chr * (CHAR_HEIGHT * settings.scale),
        charsetInfo->image->width(),
        CHAR_HEIGHT * settings.scale
    );
}
/**
 * Scroll the text in the message area up one position.
 */
void screenScrollMessageArea()
{
    U4ASSERT(
        charsetInfo != nullptr && charsetInfo->image != nullptr,
        "charset not initialized!"
    );
    const Image *screen = imageMgr->get("screen")->image;
    screenLock();
    screen->drawSubRectOn(
        screen,
        TEXT_AREA_X * charsetInfo->image->width(),
        TEXT_AREA_Y * CHAR_HEIGHT * settings.scale + 4 * settings.scale,
        TEXT_AREA_X * charsetInfo->image->width(),
        (TEXT_AREA_Y + 1) * CHAR_HEIGHT * settings.scale + 4 * settings.scale,
        TEXT_AREA_W * charsetInfo->image->width(),
        (TEXT_AREA_H - 1) * CHAR_HEIGHT * settings.scale
    );
    screen->fillRect(
        TEXT_AREA_X * charsetInfo->image->width(),
        TEXT_AREA_Y * CHAR_HEIGHT * settings.scale
        + (TEXT_AREA_H - 1) * CHAR_HEIGHT * settings.scale
        + 4 * settings.scale,
        TEXT_AREA_W * charsetInfo->image->width(),
        CHAR_HEIGHT * settings.scale,
        0,
        0,
        0
    );
    screenUnlock();
    screenRedrawScreen();
}

void screenCycle()
{
    if (++screenCurrentCycle >= SCR_CYCLE_MAX) {
        screenCurrentCycle = 0;
    }
    screenRedrawScreen();
}

void screenUpdateCursor()
{
    const int phase =
        screenCurrentCycle * SCR_CYCLE_PER_SECOND / SCR_CYCLE_MAX;
    U4ASSERT(
        phase >= 0 && phase < 4,
        "derived an invalid cursor phase: %d",
        phase
    );
    if (screenCursorStatus) {
        screenShowChar(31 - phase, screenCursorX, screenCursorY);
        screenRedrawTextArea(screenCursorX, screenCursorY, 1, 1);
    }
}

void screenUpdateMoons()
{
    /* show "L?" for the dungeon level */
    if (c->location->context == CTX_DUNGEON) {
        screenShowChar('E', 11, 0);
        screenShowChar('1' + c->location->coords.z, 12, 0);
    }
    /* show the current moons (non-combat) */
    else if ((c->location->context & CTX_NON_COMBAT)
             == c->location->context) {
        const int trammelChar = c->saveGame->trammel_phase == 0
                              ? ' '
                              : MOON_CHAR + c->saveGame->trammel_phase - 1;
        const int feluccaChar = c->saveGame->felucca_phase == 0
                              ? ' '
                              : MOON_CHAR + c->saveGame->felucca_phase - 1;
        screenShowChar(trammelChar, 11, 0);
        screenShowChar(feluccaChar, 12, 0);
    }
    screenRedrawTextArea(11, 0, 2, 1);
}

void screenUpdateWind()
{
    /* show the direction we're facing in the dungeon */
    if (c->location->context == CTX_DUNGEON) {
        screenEraseTextArea(
            WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H
        );
        screenTextAt(
            WIND_AREA_X,
            WIND_AREA_Y,
            "BLICK %4s",
            uppercase(
                getDirectionName(
                    static_cast<Direction>(c->saveGame->orientation)
                )
            ).c_str()
        );
    }
    /* show the wind direction */
    else if ((c->location->context & CTX_NON_COMBAT)
             == c->location->context) {
        screenEraseTextArea(
            WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H
        );
        screenTextAt(
            WIND_AREA_X,
            WIND_AREA_Y,
            "WIND: %4s",
            uppercase(
                getDirectionName(static_cast<Direction>(c->windDirection))
            ).c_str()
        );
    }
    screenRedrawTextArea(
        WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H
    );
}

void screenShowCursor()
{
    if (!screenCursorStatus && screenCursorEnabled) {
        screenCursorStatus = 1;
        screenUpdateCursor();
    }
}

void screenHideCursor()
{
    if (screenCursorStatus) {
        screenEraseTextArea(screenCursorX, screenCursorY, 1, 1);
        screenRedrawTextArea(screenCursorX, screenCursorY, 1, 1);
    }
    screenCursorStatus = 0;
}

void screenEnableCursor()
{
    screenCursorEnabled = 1;
}

void screenDisableCursor()
{
    screenHideCursor();
    screenCursorEnabled = 0;
}

void screenSetCursorPos(const int x, const int y)
{
    screenCursorX = x;
    screenCursorY = y;
}


/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle. (original DOS algorithm)
 */
static void screenFindLineOfSight(
    std::vector<MapTile> viewportTiles[VIEWPORT_WIDTH][VIEWPORT_HEIGHT]
)
{
    int x, y;
    if (!c) {
        return;
    }
    /*
     * if the map has the no line of sight flag, all is visible
     */
    if (c->location->map->flags & NO_LINE_OF_SIGHT) {
        for (y = 0; y < VIEWPORT_HEIGHT; y++) {
            for (x = 0; x < VIEWPORT_WIDTH; x++) {
                screenLos[x][y] = 1;
            }
        }
        return;
    }
    /*
     * otherwise calculate it from the map data
     */
    for (y = 0; y < VIEWPORT_HEIGHT; y++) {
        for (x = 0; x < VIEWPORT_WIDTH; x++) {
            screenLos[x][y] = 0;
        }
    }
    if (settings.lineOfSight == "DOS") {
        screenFindLineOfSightDOS(viewportTiles);
    } else if (settings.lineOfSight == "Enhanced") {
        screenFindLineOfSightEnhanced(viewportTiles);
    } else {
        errorFatal(
            "unknown line of sight style %s!\n", settings.lineOfSight.c_str()
        );
    }
} // screenFindLineOfSight


/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle. (original DOS algorithm)
 */
static void screenFindLineOfSightDOS(
    std::vector<MapTile> viewportTiles[VIEWPORT_WIDTH][VIEWPORT_HEIGHT]
)
{
    screenLos[VIEWPORT_WIDTH / 2][VIEWPORT_HEIGHT / 2] = 1;
    for (int x = VIEWPORT_WIDTH / 2 - 1; x >= 0; x--) {
        if (screenLos[x + 1][VIEWPORT_HEIGHT / 2]
            && !viewportTiles[x + 1][VIEWPORT_HEIGHT / 2].front().getTileType()
            ->isOpaque()) {
            screenLos[x][VIEWPORT_HEIGHT / 2] = 1;
        }
    }
    for (int x = VIEWPORT_WIDTH / 2 + 1; x < VIEWPORT_WIDTH; x++) {
        if (screenLos[x - 1][VIEWPORT_HEIGHT / 2]
            && !viewportTiles[x - 1][VIEWPORT_HEIGHT / 2].front().getTileType()
            ->isOpaque()) {
            screenLos[x][VIEWPORT_HEIGHT / 2] = 1;
        }
    }
    for (int y = VIEWPORT_HEIGHT / 2 - 1; y >= 0; y--) {
        if (screenLos[VIEWPORT_WIDTH / 2][y + 1]
            && !viewportTiles[VIEWPORT_WIDTH / 2][y + 1].front().getTileType()
            ->isOpaque()) {
            screenLos[VIEWPORT_WIDTH / 2][y] = 1;
        }
    }
    for (int y = VIEWPORT_HEIGHT / 2 + 1; y < VIEWPORT_HEIGHT; y++) {
        if (screenLos[VIEWPORT_WIDTH / 2][y - 1]
            && !viewportTiles[VIEWPORT_WIDTH / 2][y - 1].front().getTileType()
            ->isOpaque()) {
            screenLos[VIEWPORT_WIDTH / 2][y] = 1;
        }
    }
    for (int y = VIEWPORT_HEIGHT / 2 - 1; y >= 0; y--) {
        for (int x = VIEWPORT_WIDTH / 2 - 1; x >= 0; x--) {
            if (
                (screenLos[x][y + 1]
                && !viewportTiles[x][y + 1].front().getTileType()
                ->isOpaque())
                || (screenLos[x + 1][y]
                       && !viewportTiles[x + 1][y].front().getTileType()
                       ->isOpaque())
                || (screenLos[x + 1][y + 1]
                       && !viewportTiles[x + 1][y + 1].front().getTileType()
                       ->isOpaque())
                ) {
                    screenLos[x][y] = 1;
            }
        }
        for (int x = VIEWPORT_WIDTH / 2 + 1; x < VIEWPORT_WIDTH; x++) {
            if (
                (screenLos[x][y + 1]
                && !viewportTiles[x][y + 1].front().getTileType()
                ->isOpaque())
                || (screenLos[x - 1][y]
                       && !viewportTiles[x - 1][y].front().getTileType()
                       ->isOpaque())
                || (screenLos[x - 1][y + 1]
                       && !viewportTiles[x - 1][y + 1].front().getTileType()
                       ->isOpaque())
                ) {
                    screenLos[x][y] = 1;
            }
        }
    }
    for (int y = VIEWPORT_HEIGHT / 2 + 1; y < VIEWPORT_HEIGHT; y++) {
        for (int x = VIEWPORT_WIDTH / 2 - 1; x >= 0; x--) {
            if (
                (screenLos[x][y - 1]
                && !viewportTiles[x][y - 1].front().getTileType()
                ->isOpaque())
                 || (screenLos[x + 1][y]
                       && !viewportTiles[x + 1][y].front().getTileType()
                       ->isOpaque())
                 || (screenLos[x + 1][y - 1]
                       && !viewportTiles[x + 1][y - 1].front().getTileType()
                       ->isOpaque())
            ) {
                screenLos[x][y] = 1;
            }
        }
        for (int x = VIEWPORT_WIDTH / 2 + 1; x < VIEWPORT_WIDTH; x++) {
            if (
                (screenLos[x][y - 1]
                    && !viewportTiles[x][y - 1].front().getTileType()
                    ->isOpaque())
                || (screenLos[x - 1][y]
                       && !viewportTiles[x - 1][y].front().getTileType()
                       ->isOpaque())
                || (screenLos[x - 1][y - 1]
                       && !viewportTiles[x - 1][y - 1].front().getTileType()
                       ->isOpaque())
                ) {
                    screenLos[x][y] = 1;
            }
        }
    }
} // screenFindLineOfSightDOS


/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle.
 *
 * A new, more accurate LOS function
 *
 * Based somewhat off Andy McFadden's 1994 article,
 *   "Improvements to a Fast Algorithm for Calculating Shading
 *   and Visibility in a Two-Dimensional Field"
 *   -----
 *   http://www.fadden.com/techmisc/fast-los.html
 *
 * This function uses a lookup table to get the correct shadow map,
 * therefore, the table will need to be updated if the viewport
 * dimensions increase. Also, the function assumes that the
 * viewport width and height are odd values and that the player
 * is always at the center of the screen.
 */
static void screenFindLineOfSightEnhanced(
    std::vector<MapTile> viewportTiles[VIEWPORT_WIDTH][VIEWPORT_HEIGHT]
)
{
    /*
     * the shadow rasters for each viewport octant
     *
     * shadowRaster[0][0]    // number of raster segments in this shadow
     * shadowRaster[0][1]    // #1 shadow bitmask value (low three bits)
     *                       // + "newline" flag (high bit)
     * shadowRaster[0][2]    // #1 length
     * shadowRaster[0][3]    // #2 shadow bitmask value
     * shadowRaster[0][4]    // #2 length
     * shadowRaster[0][5]    // #3 shadow bitmask value
     * shadowRaster[0][6]    // #3 length
     * ...etc...
     */
    /*
     * As each viewport tile is processed, it will store the bitmask for
     * the shadow it casts. Later, after processing all octants, the
     * entire viewport will be marked visible except for those tiles that
     * have the __VCH bitmask.
     */
    constexpr int OCTANTS = 8;
    int
        xSign = 1,
        ySign = 1,
        reflect = false,
        xTile,
        yTile;

    for (int octant = 0; octant < OCTANTS; octant++) {
        constexpr int NUM_RASTERS_COLS = 4;
        switch (octant) {
        case 0:
            xSign = 1;
            ySign = 1;
            reflect = false;
            break; // lower-right
        case 1:
            xSign = 1;
            ySign = 1;
            reflect = true;
            break;
        case 2:
            xSign = 1;
            ySign = -1;
            reflect = true;
            break; // lower-left
        case 3:
            xSign = -1;
            ySign = 1;
            reflect = false;
            break;
        case 4:
            xSign = -1;
            ySign = -1;
            reflect = false;
            break; // upper-left
        case 5:
            xSign = -1;
            ySign = -1;
            reflect = true;
            break;
        case 6:
            xSign = -1;
            ySign = 1;
            reflect = true;
            break; // upper-right
        case 7:
            xSign = 1;
            ySign = -1;
            reflect = false;
            break;
        default:
            errorFatal("BUG: wrong octant");
        } // switch
        // determine the origin point for the current LOS octant
        constexpr int xOrigin = VIEWPORT_WIDTH / 2;
        constexpr int yOrigin = VIEWPORT_HEIGHT / 2;
        // make sure the segment doesn't reach out of bounds
        int maxWidth = xOrigin;
        int maxHeight = yOrigin;
        int currentRaster;
        // just in case the viewport isn't square, swap the
        // width and height
        if (reflect) {
            // swap height and width for later use
            std::swap(maxWidth, maxHeight);
        }
        // check the visibility of each tile
        for (int currentCol = 1;
             currentCol <= NUM_RASTERS_COLS;
             currentCol++) {
            for (int currentRow = 0;
                 currentRow <= currentCol;
                 currentRow++) {
                // swap X and Y to reflect the octant rasters
                if (reflect) {
                    xTile = xOrigin + currentRow * ySign;
                    yTile = yOrigin + currentCol * xSign;
                } else {
                    xTile = xOrigin + currentCol * xSign;
                    yTile = yOrigin + currentRow * ySign;
                }
                if (viewportTiles[xTile][yTile].front().getTileType()
                    ->isOpaque()) {
                    constexpr int shadowRaster[14][13] = {
                        {
                            6,
                            xxVCH,
                            4,
                            xNxCH,
                            1,
                            xxVCH,
                            3,
                            xNxxx,
                            1,
                            xxxCH,
                            1,
                            xxVCH,
                            1
                        }, // raster_1_0
                        {
                            6,
                            xxVCx,
                            1,
                            xNVCH,
                            2,
                            xxVCx,
                            1,
                            xNVCH,
                            3,
                            xNVCH,
                            2,
                            xNVCH,
                            1
                        }, // raster_1_1
                        //
                        {
                            4,
                            xxVCH,
                            3,
                            xNxxH,
                            1,
                            xxxCH,
                            1,
                            xxVCH,
                            1,
                            0,
                            0,
                            0,
                            0
                        }, // raster_2_0
                        {
                            6,
                            xxVCx,
                            2,
                            xNxCH,
                            1,
                            xxVCH,
                            2,
                            xNxCH,
                            1,
                            xxVCH,
                            1,
                            xNxxH,
                            1
                        }, // raster_2_1
                        {
                            6,
                            xxVxx,
                            1,
                            xNVCH,
                            1,
                            xxVCx,
                            1,
                            xNVCH,
                            1,
                            xxVCx,
                            1,
                            xNVCH,
                            1
                        }, // raster_2_2
                        //
                        {
                            2,
                            xxVCH,
                            2,
                            xNxxH,
                            2,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_3_0
                        {
                            3,
                            xxVCx,
                            2,
                            xNxCH,
                            1,
                            xxVCH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_3_1
                        {
                            3,
                            xxVCx,
                            1,
                            xNVCH,
                            2,
                            xNxCH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_3_2
                        {
                            3,
                            xNVCH,
                            1,
                            xxVxx,
                            1,
                            xNVCH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_3_3
                        //
                        {
                            2,
                            xxVCH,
                            1,
                            xNxxH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_4_0
                        {
                            2,
                            xxVCx,
                            1,
                            xNxxH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_4_1
                        {
                            2,
                            xxVCx,
                            1,
                            xNxCH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_4_2
                        {
                            2,
                            xxVxx,
                            1,
                            xNVCH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        }, // raster_4_3
                        {
                            2,
                            xxVxx,
                            1,
                            xNVCH,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        } // raster_4_4
                    };
                    // a wall was detected, so go through
                    // the raster for this wall segment
                    // and mark everything behind it with
                    // the appropriate shadow bitmask.
                    //
                    // first, get the correct raster
                    //
                    if (currentCol == 1 && currentRow == 0) {
                        currentRaster = 0;
                    } else if (currentCol == 1 && currentRow == 1) {
                        currentRaster = 1;
                    } else if (currentCol == 2 && currentRow == 0) {
                        currentRaster = 2;
                    } else if (currentCol == 2 && currentRow == 1) {
                        currentRaster = 3;
                    } else if (currentCol == 2 && currentRow == 2) {
                        currentRaster = 4;
                    } else if (currentCol == 3 && currentRow == 0) {
                        currentRaster = 5;
                    } else if (currentCol == 3 && currentRow == 1) {
                        currentRaster = 6;
                    } else if (currentCol == 3 && currentRow == 2) {
                        currentRaster = 7;
                    } else if (currentCol == 3 && currentRow == 3) {
                        currentRaster = 8;
                    } else if (currentCol == 4 && currentRow == 0) {
                        currentRaster = 9;
                    } else if (currentCol == 4  && currentRow == 1) {
                        currentRaster = 10;
                    } else if (currentCol == 4 && currentRow == 2) {
                        currentRaster = 11;
                    } else if (currentCol == 4 && currentRow == 3) {
                        currentRaster = 12;
                    } else {
                        // currentCol and currentRow must equal 4
                        currentRaster = 13;
                    }
                    int xTileOffset = 0;
                    int yTileOffset = 0;
                    // =================================
                    for (int currentSegment = 0;
                         currentSegment < shadowRaster[currentRaster][0];
                         currentSegment++) {
                        // each shadow segment is 2 bytes
                        int shadowType = shadowRaster[currentRaster][
                            currentSegment * 2 + 1
                        ];
                        int shadowLength = shadowRaster[currentRaster][
                            currentSegment * 2 + 2
                        ];
                        // update the raster length to make sure it fits in
                        // the viewport
                        shadowLength =
                            shadowLength + 1 + yTileOffset > maxWidth
                            ?  maxWidth
                            : shadowLength;
                        // check to see if we should move up a row
                        if (shadowType & 0x80) {
                            // remove the flag from the shadowType
                            shadowType ^= xNxxx;
                            // if (currentRow + yTileOffset >= maxHeight) {
                            if (currentRow + yTileOffset > maxHeight) {
                                break;
                            }
                            xTileOffset = yTileOffset;
                            yTileOffset++;
                        }
                        /* it is seemingly unnecessary to swap the edges for
                         * shadow tiles, because we only care about shadow
                         * tiles that have all three parts (V, C, and H)
                         * flagged.  if a tile has fewer than three, it is
                         * ignored during the draw phase, so vertical and
                         * horizontal shadow edge accuracy isn't important
                         */
                        // if reflecting the octant, swap the edges
                        // if (reflect) {
                        // int shadowTemp = 0;
                        //// swap the vertical and horizontal shadow edges
                        // if (shadowType & __V__) { shadowTemp |= ____H; }
                        // if (shadowType & ___C_) { shadowTemp |= ___C_; }
                        // if (shadowType & ____H) { shadowTemp |= __V__; }
                        // shadowType = shadowTemp;
                        // }
                        for (int currentShadow = 1;
                             currentShadow <= shadowLength;
                             currentShadow++) {
                            // apply the shadow to the shadowMap
                            if (reflect) {
                                screenLos[
                                    xTile + yTileOffset * ySign
                                ][
                                    yTile
                                    + (currentShadow + xTileOffset) * xSign
                                ] |= shadowType;
                            } else {
                                screenLos[
                                    xTile
                                    + (currentShadow + xTileOffset) * xSign
                                ][
                                    yTile + yTileOffset * ySign
                                ] |= shadowType;
                            }
                        }
                        xTileOffset += shadowLength;
                    }
                }
            }
        }
    }
    // go through all tiles on the viewable area and set the appropriate
    // visibility
    for (int y = 0; y < VIEWPORT_HEIGHT; y++) {
        for (auto &screenLo: screenLos) {
            // if the shadow flags equal __VCH, hide it, otherwise
            // it's fully visible
            //
            if ((screenLo[y] & xxVCH) == xxVCH) {
                screenLo[y] = 0;
            } else {
                screenLo[y] = 1;
            }
        }
    }
} // screenFindLineOfSightEnhanced


/**
 * Generates terms a and b for equation "ax + b = y" that defines the
 * line containing the two given points.  Vertical lines are special
 * cased to return DBL_MAX for a and the x coordinate as b since they
 * cannot be represented with the above formula.
 */
static void screenGetLineTerms(
    const int x1,
    const int y1,
    const int x2,
    const int y2,
    double *a,
    double *b
)
{
    if (x2 - x1 == 0) {
        *a = DBL_MAX;
        *b = x1;
    } else {
        *a = static_cast<double>(y2 - y1) / static_cast<double>(x2 - x1);
        *b = y1 - *a * x1;
    }
}


/**
 * Determine if two points are on the same side of a line (or both on
 * the line).  The line is defined by the terms a and b of the
 * equation "ax + b = y".
 */
static bool screenPointsOnSameSideOfLine(
    const int x1,
    const int y1,
    const int x2,
    const int y2,
    const double a,
    const double b
)
{
    double p1, p2;
    if (a == DBL_MAX) {
        p1 = x1 - b;
        p2 = x2 - b;
    } else {
        p1 = x1 * a + b - y1;
        p2 = x2 * a + b - y2;
    }
    if ((p1 > 0.0 && p2 > 0.0)
        || (p1 < 0.0 && p2 < 0.0)
        || (p1 == 0.0 && p2 == 0.0)) {
        return true;
    }
    return false;
}

static bool screenPointInTriangle(
    const int x,
    const int y,
    const int tx1,
    const int ty1,
    const int tx2,
    const int ty2,
    const int tx3,
    const int ty3
)
{
    double a[3], b[3];
    screenGetLineTerms(tx1, ty1, tx2, ty2, &a[0], &b[0]);
    screenGetLineTerms(tx2, ty2, tx3, ty3, &a[1], &b[1]);
    screenGetLineTerms(tx3, ty3, tx1, ty1, &a[2], &b[2]);
    if (!screenPointsOnSameSideOfLine(x, y, tx3, ty3, a[0], b[0])) {
        return false;
    }
    if (!screenPointsOnSameSideOfLine(x, y, tx1, ty1, a[1], b[1])) {
        return false;
    }
    if (!screenPointsOnSameSideOfLine(x, y, tx2, ty2, a[2], b[2])) {
        return false;
    }
    return true;
}


/**
 * Determine if the given point is within a mouse area.
 */
bool screenPointInMouseArea(const int x, const int y, const MouseArea *area)
{
    U4ASSERT(
        area->n_points == 2 || area->n_points == 3,
        "unsupported number of points in area: %d",
        area->n_points
    );
    /* two points define a rectangle */
    if (area->n_points == 2) {
        if (x >= area->point[0].x * settings.scale
            && y >= area->point[0].y * settings.scale
            && x < area->point[1].x * settings.scale
            && y < area->point[1].y * settings.scale) {
            return true;
        }
    }
    /* three points define a triangle */
    else if (area->n_points == 3) {
        return screenPointInTriangle(
            x,
            y,
            area->point[0].x * settings.scale,
            area->point[0].y * settings.scale,
            area->point[1].x * settings.scale,
            area->point[1].y * settings.scale,
            area->point[2].x * settings.scale,
            area->point[2].y * settings.scale
        );
    }
    return false;
}

void screenRedrawMapArea()
{
    game->mapArea.update();
}

void screenEraseMapArea()
{
    const Image *screen = imageMgr->get("screen")->image;
    screen->fillRect(
        BORDER_WIDTH * settings.scale,
        BORDER_HEIGHT * settings.scale + 4 * settings.scale,
        VIEWPORT_WIDTH * TILE_WIDTH * settings.scale,
        VIEWPORT_HEIGHT * TILE_HEIGHT * settings.scale,
        0,
        0,
        0
    );
}

void screenEraseTextArea(
    const int x, const int y, const int width, const int height
)
{
    const Image *screen = imageMgr->get("screen")->image;
    screen->fillRect(
        x * CHAR_WIDTH * settings.scale,
        y * CHAR_HEIGHT * settings.scale + 4 * settings.scale,
        width * CHAR_WIDTH * settings.scale,
        height * CHAR_HEIGHT * settings.scale,
        0,
        0,
        0
    );
}


/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(const int iterations)
{
    // the MSVC8 binary was generating a Access Violation when using
    // drawSubRectOn() or drawOn() to draw the screen surface on top
    // of itself.  Occurred on settings.scale 2 and 4 only.
    // Therefore, a temporary Image buffer is used to store the area
    // that gets clipped at the bottom.
    if (settings.screenShakes) {
        // specify the size of the offset, and create a buffer
        // to store the offset row plus 1
        constexpr int shakeOffset = 2;
        const Image *screen = imageMgr->get("screen")->image;
        const Image *bottom = Image::create(
            SCALED(320), SCALED(shakeOffset + 1), false, Image::SOFTWARE
        );
        bottom->alphaOff();
        // disable all screen updates except our own
        // otherwise image will get messed up because things aren't in their
        // usual places
        screenMoving = false;
        // do the actual shaking
        for (int i = 0; i < iterations; i++) {
            // store the bottom row
            screen->drawOn(bottom, 0, SCALED(shakeOffset + 1 - 200));
            // shift the screen down and make the top row black
            screen->drawSubRectOn(
                screen,
                0,
                SCALED(shakeOffset),
                0,
                0,
                SCALED(320),
                SCALED(200 - (shakeOffset + 1)),
                true
            );
            bottom->drawOn(screen, 0, SCALED(200 - shakeOffset), true);
            screen->fillRect(
                0,
                0,
                SCALED(320),
                SCALED(shakeOffset),
                0,
                0,
                0,
                IM_OPAQUE,
                true
            );
            screenRedrawScreen();
            EventHandler::sleep(settings.shakeInterval);
            // shift the screen back up, and replace the bottom row
            screen->drawOn(screen, 0, 0 - SCALED(shakeOffset), true);
            bottom->drawOn(screen, 0, SCALED(200 - (shakeOffset + 1)), true);
            screenRedrawScreen();
            EventHandler::sleep(settings.shakeInterval);
        }
        // free the bottom row image
        delete bottom;
        // re-enable screen updates
        screenMoving = true;
        screenRedrawScreen();
        EventHandler::sleep(settings.shakeInterval);
    }
} // screenShake


/**
 * Draw a tile graphic on the screen.
 */
static void screenShowGemTile(
    const Layout *layout,
    const Map *map,
    MapTile t,
    bool,
    const int x,
    const int y
)
{
    // Make sure we account for tiles that look like other tiles
    // (dungeon tiles, mainly)
    const std::string &looks_like = t.getTileType()->getLooksLike();
    if (!looks_like.empty()) {
        t = map->tileset->getByName(looks_like)->getId();
    }
    if (map->isDungeonMap()) {
        U4ASSERT(charsetInfo, "charset not initialized");
        const auto charIndex =
            dungeonTileChars.find(t.getTileType()->getName());
        if (charIndex != dungeonTileChars.end()) {
            charsetInfo->image->drawSubRect(
                (layout->viewport.x + x * layout->tile_shape.width)
                * settings.scale,
                (layout->viewport.y + y * layout->tile_shape.height)
                * settings.scale + 4 * settings.scale,
                0,
                charIndex->second * layout->tile_shape.height * settings.scale,
                layout->tile_shape.width * settings.scale,
                layout->tile_shape.height * settings.scale
            );
        }
    } else {
        if (gemTilesInfo == nullptr) {
            gemTilesInfo = imageMgr->get(BKGD_GEM_TILES);
            if (!gemTilesInfo) {
                errorFatal(
                    "ERROR 1002: Unable to load the \"%s\" data file.\t\n\n"
                    "Is %s installed?\n\nVisit the XU4 website for additional "
                    "information.\n\thttp://xu4.sourceforge.net/",
                    BKGD_GEM_TILES,
                    settings.game.c_str()
                );
            }
        }
        const int tile = map->translateToRawTile(t);
        if (tile < 128) {
            gemTilesInfo->image->drawSubRect(
                (layout->viewport.x + x * layout->tile_shape.width)
                * settings.scale,
                (layout->viewport.y + y * layout->tile_shape.height)
                * settings.scale + 4 * settings.scale,
                0,
                tile * layout->tile_shape.height * settings.scale,
                layout->tile_shape.width * settings.scale,
                layout->tile_shape.height * settings.scale
            );
        } else {
            const Image *screen = imageMgr->get("screen")->image;
            screen->fillRect(
                (layout->viewport.x + x * layout->tile_shape.width)
                * settings.scale,
                (layout->viewport.y + y * layout->tile_shape.height)
                * settings.scale + 4 * settings.scale,
                layout->tile_shape.width * settings.scale,
                layout->tile_shape.height * settings.scale,
                0,
                0,
                0
            );
        }
    }
} // screenShowGemTile

static Layout *screenGetGemLayout(const Map *map)
{
    if (map->isDungeonMap()) {
        for (auto *layout: layouts) {
            if (layout->type == LAYOUT_DUNGEON_GEM) {
                return layout;
            }
        }
        errorFatal("no dungeon gem layout found!\n");
    }
    return gem_layout;
}

void screenGemUpdate()
{
    MapTile tile;
    int x, y;
    const Image *screen = imageMgr->get("screen")->image;
    screen->fillRect(
        BORDER_WIDTH * settings.scale,
        BORDER_HEIGHT * settings.scale + 4 * settings.scale,
        VIEWPORT_WIDTH * TILE_WIDTH * settings.scale,
        VIEWPORT_HEIGHT * TILE_HEIGHT * settings.scale,
        0,
        0,
        0
    );
    const Layout *layout = screenGetGemLayout(c->location->map);
    // TODO, move the code responsible for determining 'peer' visibility
    // to a non SDL specific part of the code.
    if (c->location->map->isDungeonMap()) {
        // DO THE SPECIAL DUNGEON MAP TRAVERSAL
        std::vector<std::vector<int> > drawnTiles(
            layout->viewport.width,
            std::vector<int>(layout->viewport.height, 0)
        );
        std::list<std::pair<int, int> > coordStack;
        // Put the avatar's position on the stack
        int center_x = layout->viewport.width / 2 - 1;
        int center_y = layout->viewport.height / 2 - 1;
        const int avatar_x = c->location->coords.x - 1;
        const int avatar_y = c->location->coords.y - 1;
        coordStack.emplace_back(center_x, center_y);
        bool weAreDrawingTheAvatarTile = true;
        // And draw each tile on the growing stack until it is empty
        while (!coordStack.empty()) {
            const std::pair<int, int> currentXY = coordStack.back();
            coordStack.pop_back();
            x = currentXY.first;
            y = currentXY.second;
            if (x < 0
                || x >= layout->viewport.width
                || y < 0
                || y >= layout->viewport.height) {
                continue; // Skip out of range tiles
            }
            if (drawnTiles[x][y]) {
                continue; // Skip already considered tiles
            }
            drawnTiles[x][y] = 1;
            // DRAW THE ACTUAL TILE
            bool focus;
            std::vector<MapTile> tiles = screenViewportTile(
                layout->viewport.width,
                layout->viewport.height,
                x - center_x + avatar_x,
                y - center_y + avatar_y,
                focus
            );
            tile = tiles.front();
            const TileId avatarTileId =
                c->location->map->tileset->getByName("avatar")->getId();
            if (!weAreDrawingTheAvatarTile) {
                // Hack to avoid showing the avatar tile multiple times
                if (tile.getId() == avatarTileId) {
                    tile = c->location->map->tileAt(
                        c->location->coords, WITHOUT_OBJECTS
                    ).getId();
                }
            }
            screenShowGemTile(layout, c->location->map, tile, focus, x, y);
            if (!tile.getTileType()->isOpaque()
                || tile.getTileType()->isWalkable()
                || weAreDrawingTheAvatarTile) {
                // Continue the search so we can see through all walkable
                // objects, non-opaque objects (like creatures) or the avatar
                // position in those rare circumstances where he is stuck in a
                // wall by adding all relative adjacency combinations to the
                // stack for drawing
                coordStack.emplace_back(x + 1, y - 1);
                coordStack.emplace_back(x + 1, y);
                coordStack.emplace_back(x + 1, y + 1);
                coordStack.emplace_back(x, y - 1);
                coordStack.emplace_back(x, y + 1);
                coordStack.emplace_back(x - 1, y - 1);
                coordStack.emplace_back(x - 1, y);
                coordStack.emplace_back(x - 1, y + 1);
                // We only draw the avatar tile once, it is the first
                // tile drawn
                weAreDrawingTheAvatarTile = false;
            }
        }
    } else {
        // DO THE REGULAR EVERYTHING-IS-VISIBLE MAP TRAVERSAL
        for (x = 0; x < layout->viewport.width; x++) {
            for (y = 0; y < layout->viewport.height; y++) {
                bool focus;
                tile = screenViewportTileGem(
                    layout->viewport.width,
                    layout->viewport.height,
                    x,
                    y,
                    focus
                );
                screenShowGemTile(layout, c->location->map, tile, focus, x, y);
            }
        }
    }
    screenRedrawMapArea();
    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
} // screenGemUpdate
