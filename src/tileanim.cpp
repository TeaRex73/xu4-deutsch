/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <vector>
#include <typeinfo>

#include "config.h"
#include "direction.h"
#include "image.h"
#include "screen.h"
#include "tileanim.h"
#include "u4.h"
#include "utils.h"
#include "tile.h"


TileAnimTransform *TileAnimTransform::create(const ConfigElement &conf)
{
    TileAnimTransform *transform = nullptr;
    static const char *transformTypeEnumStrings[] = {
        "invert",
        "pixel",
        "scroll",
        "frame",
        "pixel_color",
        "scramble",
        nullptr
    };
    int type = conf.getEnum("type", transformTypeEnumStrings);
    switch (type) {
    case 0:
        transform = new TileAnimInvertTransform(
            conf.getInt("x"),
            conf.getInt("y"),
            conf.getInt("width"),
            conf.getInt("height")
        );
        break;
    case 1:
    {
        transform = new TileAnimPixelTransform(
            conf.getInt("x"),
            conf.getInt("y")
        );
        std::vector<ConfigElement> children = conf.getChildren();
        for (std::vector<ConfigElement>::const_iterator i = children.cbegin();
             i != children.cend();
             ++i) {
            if (i->getName() == "color") {
                RGBA *rgba = loadColorFromConf(*i);
                static_cast<TileAnimPixelTransform *>(transform)
                    ->colors.push_back(rgba);
            }
        }
        break;
    }
    case 2:
        transform = new TileAnimScrollTransform(
            conf.getInt("increment")
        );
        break;
    case 3:
        transform = new TileAnimFrameTransform();
        break;
    case 4:
    {
        transform = new TileAnimPixelColorTransform(
            conf.getInt("x"),
            conf.getInt("y"),
            conf.getInt("width"),
            conf.getInt("height")
        );
        std::vector<ConfigElement> children = conf.getChildren();
        for (std::vector<ConfigElement>::const_iterator i = children.cbegin();
             i != children.cend();
             ++i) {
            if (i->getName() == "color") {
                RGBA *rgba = loadColorFromConf(*i);
                if (i == children.begin()) {
                    static_cast<TileAnimPixelColorTransform *>(transform)
                        ->start = rgba;
                } else {
                    static_cast<TileAnimPixelColorTransform *>(transform)
                        ->end = rgba;
                }
            }
        }
        break;
    }
    case 5:
        transform = new TileAnimScrambleTransform();
        break;
    } // switch
    /**
     * See if the transform is performed randomly
     */
    if (conf.exists("random")) {
        transform->random = conf.getInt("random");
    } else {
        transform->random = 0;
    }
    return transform;
} // TileAnimTransform::create

/**
 * Loads a color from a config element
 */
RGBA *TileAnimTransform::loadColorFromConf(const ConfigElement &conf)
{
    RGBA *rgba;
    rgba = new RGBA;
    rgba->r = conf.getInt("red");
    rgba->g = conf.getInt("green");
    rgba->b = conf.getInt("blue");
    rgba->a = IM_OPAQUE;
    return rgba;
}

TileAnimInvertTransform::TileAnimInvertTransform(int x, int y, int w, int h)
    :x(x), y(y), w(w), h(h)
{
}

bool TileAnimInvertTransform::drawsTile() const
{
    return false;
}

void TileAnimInvertTransform::draw(
    Image *dest, Tile *tile, MapTile mapTile
)
{
    int scale = tile->getScale();
    tile->getImage()->drawSubRectInvertedOn(
        dest,
        x * scale,
        y * scale,
        x * scale,
        (tile->getHeight() * mapTile.getFrame()) + (y * scale),
        w * scale, h * scale
    );
}

TileAnimPixelTransform::TileAnimPixelTransform(int x, int y)
    :x(x), y(y), colors()
{
}

TileAnimPixelTransform::~TileAnimPixelTransform()
{
    std::vector<RGBA *>::const_iterator i;
    for (i = colors.cbegin(); i != colors.cend(); ++i) {
        delete (*i);
    }
    colors.clear();
}

bool TileAnimPixelTransform::drawsTile() const
{
    return false;
}

void TileAnimPixelTransform::draw(Image *dest, Tile *tile, MapTile )
{
    const RGBA *color = colors[xu4_random(colors.size())];
    int scale = tile->getScale();
    dest->fillRect(
        x * scale,
        y * scale,
        scale,
        scale,
        color->r,
        color->g,
        color->b,
        color->a
    );
}

bool TileAnimScrollTransform::drawsTile() const
{
    return true;
}

TileAnimScrollTransform::TileAnimScrollTransform(int i)
    :increment(i), current(0), lastOffset(0)
{
}

void TileAnimScrollTransform::draw(
    Image *dest, Tile *tile, MapTile mapTile
)
{
    if (increment == 0) {
        increment = tile->getScale();
    }
    int offset = screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND
        * tile->getScale();
    if (lastOffset != offset) {
        lastOffset = offset;
        current += increment;
        if (current >= tile->getHeight()) {
            current = 0;
        }
    }
    tile->getImage()->drawSubRectOn(
        dest,
        0,
        current,
        0,
        tile->getHeight() * mapTile.getFrame(),
        tile->getWidth(),
        tile->getHeight() - current
    );
    if (current != 0) {
        tile->getImage()->drawSubRectOn(
            dest,
            0,
            0,
            0,
            (tile->getHeight() * mapTile.getFrame())
            + tile->getHeight() - current,
            tile->getWidth(),
            current
        );
    }
}

bool TileAnimScrambleTransform::drawsTile() const
{
    return true;
}

void TileAnimScrambleTransform::draw(
    Image *dest, Tile *tile, MapTile mapTile
)
{
    int scale = tile->getScale();
    for (int i = 0; i < tile->getHeight() / scale; i++) {
        for (int j = 0; j < tile->getWidth() / scale; j++) {
            tile->getImage()->drawSubRectOn(
                dest,
                j * scale,
                i * scale,
                xu4_random(tile->getWidth() / scale) * scale,
                (tile->getHeight() * mapTile.getFrame()
                 + xu4_random(tile->getHeight()))
                / scale * scale,
                scale,
                scale);
        }
    }
}


/**
 * Advance the frame by one and draw it!
 */
bool TileAnimFrameTransform::drawsTile() const
{
    return true;
}

void TileAnimFrameTransform::draw(Image *dest, Tile *tile, MapTile )
{
    if (++currentFrame >= tile->getFrames()) {
        currentFrame = 0;
    }
    tile->getImage()->drawSubRectOn(
        dest,
        0,
        0,
        0,
        currentFrame * tile->getHeight(),
        tile->getWidth(),
        tile->getHeight()
    );
}

TileAnimPixelColorTransform::TileAnimPixelColorTransform(
    int x, int y, int w, int h
)
    :x(x), y(y), w(w), h(h), start(nullptr), end(nullptr)
{
}

TileAnimPixelColorTransform::~TileAnimPixelColorTransform()
{
    delete start;
    delete end;
}

bool TileAnimPixelColorTransform::drawsTile() const
{
    return false;
}

void TileAnimPixelColorTransform::draw(
    Image *dest, Tile *tile, MapTile mapTile
)
{
    RGBA diff = *end;
    int scale = tile->getScale();
    diff.r -= start->r;
    diff.g -= start->g;
    diff.b -= start->b;
    const Image *tileImage = tile->getImage();
    for (int j = y * scale; j < (y * scale) + (h * scale); j++) {
        for (int i = x * scale; i < (x * scale) + (w * scale); i++) {
            RGBA pixelAt;
            tileImage->getPixel(
                i,
                j + (mapTile.getFrame() * tile->getHeight()),
                pixelAt.r,
                pixelAt.g,
                pixelAt.b,
                pixelAt.a
            );
            if ((pixelAt.r >= start->r)
                && (pixelAt.r <= end->r)
                && (pixelAt.g >= start->g)
                && (pixelAt.g <= end->g)
                && (pixelAt.b >= start->b)
                && (pixelAt.b <= end->b)) {
                dest->putPixel(
                    i,
                    j,
                    start->r + xu4_random(diff.r),
                    start->g + xu4_random(diff.g),
                    start->b + xu4_random(diff.b),
                    pixelAt.a
                );
            }
        }
    }
}


/**
 * Creates a new animation context which controls if animation transforms
 * are performed or not
 */
TileAnimContext *TileAnimContext::create(const ConfigElement &conf)
{
    TileAnimContext *context;
    static const char *contextTypeEnumStrings[] = {
        "frame",
        "dir",
        nullptr
    };
    static const char *dirEnumStrings[] = {
        "none",
        "west",
        "north",
        "east",
        "south",
        nullptr
    };
    TileAnimContext::Type type = static_cast<TileAnimContext::Type>(
        conf.getEnum("type", contextTypeEnumStrings)
    );
    switch (type) {
    case FRAME:
        context = new TileAnimFrameContext(conf.getInt("frame"));
        break;
    case DIR:
        context = new TileAnimPlayerDirContext(
            Direction(conf.getEnum("dir", dirEnumStrings))
        );
        break;
    default:
        context = nullptr;
        break;
    }
    /**
     * Add the transforms to the context
     */
    if (context) {
        std::vector<ConfigElement> children = conf.getChildren();
        for (std::vector<ConfigElement>::const_iterator i = children.cbegin();
             i != children.cend();
             ++i) {
            if (i->getName() == "transform") {
                TileAnimTransform *transform = TileAnimTransform::create(*i);
                context->add(transform);
            }
        }
    }
    return context;
} // TileAnimContext::create


TileAnimContext::~TileAnimContext()
{
    for (TileAnimTransformList::iterator i = animTransforms.begin();
         i != animTransforms.end();
         ++i) {
        delete (*i);
        (*i) = nullptr;
    }
}

/**
 * Adds a tile transform to the context
 */
void TileAnimContext::add(TileAnimTransform *transform)
{
    animTransforms.push_back(transform);
}


/**
 * A context which depends on the tile's current frame for animation
 */
TileAnimFrameContext::TileAnimFrameContext(int f)
    :frame(f)
{
}

bool TileAnimFrameContext::isInContext(
    Tile *, MapTile mapTile, Direction
)
{
    return mapTile.getFrame() == frame;
}


/**
 * An animation context which changes the animation based on the player's
 * current facing direction
 */
TileAnimPlayerDirContext::TileAnimPlayerDirContext(Direction d)
    :dir(d)
{
}

bool TileAnimPlayerDirContext::isInContext(
    Tile *, MapTile , Direction d
)
{
    return d == dir;
}


/**
 * TileAnimSet
 */
TileAnimSet::TileAnimSet(const ConfigElement &conf)
    :name(conf.getString("name")), tileanims()

{
    std::vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::const_iterator i = children.cbegin();
         i != children.cend();
         ++i) {
        if (i->getName() == "tileanim") {
            TileAnim *anim = new TileAnim(*i);
            tileanims[anim->name] = anim;
        }
    }
}

TileAnimSet::~TileAnimSet()
{
    TileAnimMap::const_iterator i;
    for (i = tileanims.cbegin(); i != tileanims.cend(); ++i) {
        delete (i->second);
    }
}


/**
 * Returns the tile animation with the given name from the current set
 */
TileAnim *TileAnimSet::getByName(const std::string &name)
{
    TileAnimMap::iterator i = tileanims.find(name);
    if (i == tileanims.end()) {
        return nullptr;
    }
    return i->second;
}

TileAnim::TileAnim(const ConfigElement &conf)
    :name(conf.getString("name")),
     transforms(),
     contexts(),
     random(conf.exists("random") ? conf.getInt("random") : 0)
{
    std::vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::const_iterator i = children.cbegin();
         i != children.cend();
         ++i) {
        if (i->getName() == "transform") {
            TileAnimTransform *transform = TileAnimTransform::create(*i);
            transforms.push_back(transform);
        } else if (i->getName() == "context") {
            TileAnimContext *context = TileAnimContext::create(*i);
            contexts.push_back(context);
        }
    }
}

TileAnim::~TileAnim()
{
    for(std::vector<TileAnimTransform *>::const_iterator i =
            transforms.cbegin();
        i != transforms.end();
        ++i) {
        delete (*i);
    }
    for(std::vector<TileAnimContext *>::const_iterator i = contexts.cbegin();
        i != contexts.cend();
        ++i) {
        delete (*i);
    }
}

void TileAnim::draw(
    Image *dest, Tile *tile, MapTile mapTile, Direction dir
) const
{
    std::vector<TileAnimTransform *>::const_iterator t;
    std::vector<TileAnimContext *>::const_iterator c;
    bool drawn = false;
    /* nothing to do, draw the tile and return! */
    if ((random && (xu4_random(100) > random))
        || (!transforms.size() && !contexts.size())
        || mapTile.getFreezeAnimation()) {
        tile->getImage()->drawSubRectOn(
            dest,
            0,
            0,
            0,
            mapTile.getFrame() * tile->getHeight(),
            tile->getWidth(),
            tile->getHeight()
        );
        return;
    }
    /**
     * Do global transforms
     */
    for (t = transforms.cbegin(); t != transforms.cend(); ++t) {
        TileAnimTransform *transform = *t;
        if (!transform->random ||
            (xu4_random(100) < transform->random)) {
            if (!transform->drawsTile() && !drawn) {
                tile->getImage()->drawSubRectOn(
                    dest,
                    0,
                    0,
                    0,
                    mapTile.getFrame() * tile->getHeight(),
                    tile->getWidth(), tile->getHeight()
                );
            }
            transform->draw(dest, tile, mapTile);
            drawn = true;
        }
    }
    /**
     * Do contextual transforms
     */
    for (c = contexts.cbegin(); c != contexts.cend(); ++c) {
        if ((*c)->isInContext(tile, mapTile, dir)) {
            const TileAnimContext::TileAnimTransformList
                &ctx_transforms = (*c)->getTransforms();
            for (t = ctx_transforms.cbegin();
                 t != ctx_transforms.cend();
                 ++t) {
                TileAnimTransform *transform = *t;
                if (!transform->random
                    || (xu4_random(100) < transform->random)) {
                    if (!transform->drawsTile() && !drawn) {
                        tile->getImage()->drawSubRectOn(
                            dest,
                            0,
                            0,
                            0,
                            mapTile.getFrame() * tile->getHeight(),
                            tile->getWidth(),
                            tile->getHeight()
                        );
                    }
                    transform->draw(dest, tile, mapTile);
                    drawn = true;
                }
            }
        }
    }
} // TileAnim::draw
