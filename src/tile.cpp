/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "tile.h"

#include "config.h"
#include "context.h"
#include "creature.h"
#include "error.h"
#include "image.h"
#include "imagemgr.h"
#include "location.h"
#include "settings.h"
#include "tileanim.h"
#include "tilemap.h"
#include "tileset.h"
#include "utils.h"

TileId Tile::nextId = 0;

Tile::Tile(Tileset *tileset)
    :id(nextId++),
     name(),
     tileset(tileset),
     w(0),
     h(0),
     frames(0),
     scale(1),
     anim(nullptr),
     opaque(false),
     foreground(),
     waterForeground(),
     rule(nullptr),
     imageName(),
     looks_like(),
     image(nullptr),
     tiledInDungeon(false),
     directions(),
     animationRule("")
{
}


/**
 * Loads tile information.
 */
void Tile::loadProperties(const ConfigElement &conf)
{
    if (conf.getName() != "tile") {
        return;
    }
    /* get the name of the tile */
    name = conf.getString("name");
    /* get the animation for the tile, if one is specified */
    if (conf.exists("animation")) {
        animationRule = conf.getString("animation");
    }
    /* see if the tile is opaque */
    opaque = conf.getBool("opaque");
    foreground = conf.getBool("usesReplacementTileAsBackground");
    waterForeground =
        conf.getBool("usesWaterReplacementTileAsBackground");
    /* find the rule that applies to the current tile, if there is one.
       if there is no rule specified, it defaults to the "default" rule */
    if (conf.exists("rule")) {
        rule = TileRule::findByName(conf.getString("rule"));
        if (rule == nullptr) {
            rule = TileRule::findByName("default");
        }
    } else {
        rule = TileRule::findByName("default");
    }
    /* get the number of frames the tile has */
    frames = conf.getInt("frames", 1);
    /* get the name of the image that belongs to this tile */
    if (conf.exists("image")) {
        imageName = conf.getString("image");
    } else {
        imageName = std::string("tile_") + name;
    }
    tiledInDungeon = conf.getBool("tiledInDungeon");
    if (conf.exists("directions")) {
        std::string dirs = conf.getString("directions");
        if (dirs.length() != static_cast<std::size_t>(frames)) {
            errorFatal(
                "Error: %llu directions for tile but only %d frames",
                static_cast<unsigned long long>(dirs.length()),
                frames
            );
        }
        for (unsigned int i = 0; i < dirs.length(); i++) {
            if (dirs[i] == 'w') {
                directions.push_back(DIR_WEST);
            } else if (dirs[i] == 'n') {
                directions.push_back(DIR_NORTH);
            } else if (dirs[i] == 'e') {
                directions.push_back(DIR_EAST);
            } else if (dirs[i] == 's') {
                directions.push_back(DIR_SOUTH);
            } else {
                errorFatal(
                    "Error: unknown direction specified by %c", dirs[i]
                );
            }
        }
    }
} // Tile::loadProperties


Image *Tile::getImage()
{
    if (!image) {
        loadImage();
    }
    return image;
}


/**
 * Loads the tile image
 */
void Tile::loadImage()
{
    if (!image) {
        scale = settings.scale;
        const SubImage *subimage = nullptr;
        ImageInfo *info = imageMgr->get(imageName);
        if (!info) {
            subimage = imageMgr->getSubImage(imageName);
            if (subimage) {
                info = imageMgr->get(subimage->srcImageName);
            }
        }
        if (!info) { // IF still no info loaded
            errorWarning(
                "Error: couldn't load image for tile '%s'", name.c_str()
            );
            return;
        }
        /* FIXME: This is a hack to address the fact that there
           are 4 frames for the guard in VGA mode, but only 2 in
           EGA. Is there  a better way to handle this? */
        if (name == "guard") {
            if (settings.videoType == "EGA") {
                frames = 2;
            } else {
                frames = 4;
            }
        }
        if (info->image) {
            info->image->alphaOff();
        }

        w = subimage ?
            subimage->width * scale :
            info->width * scale / info->prescale;
        h = subimage ?
            (subimage->height * scale) / frames :
            (info->height * scale / info->prescale) / frames;
        image = Image::create(
            w, h * frames, false, Image::SOFTWARE
        );
        image->alphaOff();
        /* draw the tile from the image we found
           to our tile image */
        if (subimage) {
            const Image *tiles = info->image;
            tiles->drawSubRectOn(
                image,
                0,
                0,
                subimage->x * scale,
                subimage->y * scale,
                subimage->width * scale,
                subimage->height * scale
            );
        } else {
            info->image->drawOn(image, 0, 0);
        }

        info->image->alphaOff();
        if (animationRule.size() > 0) {
            extern TileAnimSet *tileanims;
            anim = nullptr;
            if (tileanims) {
                anim = tileanims->getByName(animationRule);
            }
            if (anim == nullptr) {
                errorWarning(
                    "Warning: animation style '%s' not found",
                    animationRule.c_str()
                );
            }
        }
        /* if we have animations, we always used 'animated'
           to draw from */
        // if (anim)
        image->alphaOff();
    }
} // Tile::loadImage

void Tile::deleteImage()
{
    delete image;
    image = nullptr;
    scale = settings.scale;
}


/**
 * MapTile Class Implementation
 */
Direction MapTile::getDirection() const
{
    return getTileType()->directionForFrame(frame);
}

bool MapTile::setDirection(Direction d)
{
    /* if we're already pointing the right direction, do nothing! */
    if (getDirection() == d) {
        return false;
    }
    const Tile *type = getTileType();
    int new_frame = type->frameForDirection(d);
    if (new_frame != -1) {
        frame = new_frame;
        return true;
    }
    return false;
}

bool Tile::isDungeonFloor() const
{
    const Tile *floor = tileset->getByName("brick_floor");
    if (id == floor->id) {
        return true;
    }
    return false;
}

bool Tile::isOpaque() const
{
    return c->opacity ? opaque : false;
}

/**
 * Is tile a foreground tile (i.e. has transparent parts).
 * Deprecated? Never used in XML. Other mechanisms exist,
 * though this could help?
 */
bool Tile::isForeground() const
{
    return rule->mask & MASK_FOREGROUND;
}

Direction Tile::directionForFrame(int frame) const
{
    if (static_cast<unsigned int>(frame) >= directions.size()) {
        return DIR_NONE;
    } else {
        return directions[frame];
    }
}

int Tile::frameForDirection(Direction d) const
{
    for (int i = 0;
         static_cast<unsigned int>(i) < directions.size() && i < frames;
         i++) {
        if (directions[i] == d) {
            return i;
        }
    }
    return -1;
}

const Tile *MapTile::getTileType() const
{
    return Tileset::findTileById(id);
}
