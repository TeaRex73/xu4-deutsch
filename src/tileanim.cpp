/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <vector>

#include "tileanim.h"

#include "config.h"
#include "debug.h"
#include "direction.h"
#include "error.h"
#include "image.h"
#include "screen.h"
#include "tile.h"
#include "types.h"
#include "utils.h"


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
    const int type = conf.getEnum("type", transformTypeEnumStrings);
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
        const std::vector<ConfigElement> children = conf.getChildren();
        for (const auto &child: children) {
            if (child.getName() == "color") {
                RGBA *rgba = loadColorFromConf(child);
                auto *pixel_transform =
                    dynamic_cast<TileAnimPixelTransform *>(transform);
                U4ASSERT(
                    pixel_transform, "color element on non-pixel transform"
                );
                pixel_transform->colors.push_back(rgba);
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
        for (auto child = children.cbegin();
            child != children.cend();
            ++child) {
            if (child->getName() == "color") {
                RGBA *rgba = loadColorFromConf(*child);
                auto *pixel_color_tansform =
                    dynamic_cast<TileAnimPixelColorTransform *>(transform);
                U4ASSERT(
                    pixel_color_tansform,
                    "color element on non-pixel-color transform"
                );
                if (child == children.begin()) {
                    pixel_color_tansform->start = rgba;
                } else {
                    pixel_color_tansform->end = rgba;
                }
            }
        }
        break;
    }
    case 5:
        transform = new TileAnimScrambleTransform();
        break;
    default:
        errorFatal("BUG: wrong type in TileAnimTransform");
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
    auto *rgba = new RGBA;
    rgba->r = conf.getInt("red");
    rgba->g = conf.getInt("green");
    rgba->b = conf.getInt("blue");
    rgba->a = IM_OPAQUE;
    return rgba;
}

TileAnimInvertTransform::TileAnimInvertTransform(
    const int x, const int y, const int w, const int h
)
    :x(x), y(y), w(w), h(h)
{
}

bool TileAnimInvertTransform::drawsTile() const
{
    return false;
}

void TileAnimInvertTransform::draw(
    Image *dest, Tile *tile, const MapTile mapTile
)
{
    const int scale = tile->getScale();
    tile->getImage()->drawSubRectInvertedOn(
        dest,
        x * scale,
        y * scale,
        x * scale,
        tile->getHeight() * mapTile.getFrame() + y * scale,
        w * scale, h * scale
    );
}

TileAnimPixelTransform::TileAnimPixelTransform(const int x, const int y)
    :x(x), y(y)
{
}

TileAnimPixelTransform::~TileAnimPixelTransform()
{
    for (const auto *color: colors) {
        delete color;
    }
    colors.clear();
}

bool TileAnimPixelTransform::drawsTile() const
{
    return false;
}

void TileAnimPixelTransform::draw(Image *dest, Tile *tile, MapTile )
{
    const RGBA *color = colors[xu4_random(static_cast<int>(colors.size()))];
    const int scale = tile->getScale();
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

TileAnimScrollTransform::TileAnimScrollTransform(const int i)
    :increment(i), current(0), lastOffset(0)
{
}

void TileAnimScrollTransform::draw(
    Image *dest, Tile *tile, const MapTile mapTile
)
{
    if (increment == 0) {
        increment = tile->getScale();
    }
    const int offset = screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND
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
            tile->getHeight() * mapTile.getFrame()
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
    Image *dest, Tile *tile, const MapTile mapTile
)
{
    const int scale = tile->getScale();
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
    const int x, const int y, const int w, const int h
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
    Image *dest, Tile *tile, const MapTile mapTile
)
{
    RGBA diff = *end;
    const int scale = tile->getScale();
    diff.r -= start->r;
    diff.g -= start->g;
    diff.b -= start->b;
    const Image *tileImage = tile->getImage();
    for (int j = y * scale; j < y * scale + h * scale; j++) {
        for (int i = x * scale; i < x * scale + w * scale; i++) {
            RGBA pixelAt;
            tileImage->getPixel(
                i,
                j + mapTile.getFrame() * tile->getHeight(),
                pixelAt.r,
                pixelAt.g,
                pixelAt.b,
                pixelAt.a
            );
            if (pixelAt.r >= start->r
                && pixelAt.r <= end->r
                && pixelAt.g >= start->g
                && pixelAt.g <= end->g
                && pixelAt.b >= start->b
                && pixelAt.b <= end->b) {
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
    const auto type = static_cast<Type>(
        conf.getEnum("type", contextTypeEnumStrings)
    );
    switch (type) {
    case FRAME:
        context = new TileAnimFrameContext(conf.getInt("frame"));
        break;
    case DIR:
        context = new TileAnimPlayerDirContext(
            static_cast<Direction>(conf.getEnum("dir", dirEnumStrings))
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
        const std::vector<ConfigElement> children = conf.getChildren();
        for (const auto &child: children) {
            if (child.getName() == "transform") {
                TileAnimTransform *transform =
                    TileAnimTransform::create(child);
                context->add(transform);
            }
        }
    }
    return context;
} // TileAnimContext::create


TileAnimContext::~TileAnimContext()
{
    for (auto &animTransform: animTransforms) {
        delete animTransform;
        animTransform = nullptr;
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
TileAnimFrameContext::TileAnimFrameContext(const int f)
    :frame(f)
{
}

bool TileAnimFrameContext::isInContext(
    const Tile *, const MapTile mapTile, Direction
) const
{
    return mapTile.getFrame() == frame;
}


/**
 * An animation context which changes the animation based on the player's
 * current facing direction
 */
TileAnimPlayerDirContext::TileAnimPlayerDirContext(const Direction d)
    :dir(d)
{
}

bool TileAnimPlayerDirContext::isInContext(
    const Tile *, MapTile , const Direction d
) const
{
    return d == dir;
}


/**
 * TileAnimSet
 */
TileAnimSet::TileAnimSet(const ConfigElement &conf)
    :name(conf.getString("name"))

{
    const std::vector<ConfigElement> children = conf.getChildren();
    for (const auto &child: children) {
        if (child.getName() == "tileanim") {
            auto *anim = new TileAnim(child);
            tileAnimations[anim->name] = anim;
        }
    }
}

TileAnimSet::~TileAnimSet()
{
    for (const auto &tileAnimation: tileAnimations) {
        delete tileAnimation.second;
    }
}


/**
 * Returns the tile animation with the given name from the current set
 */
TileAnim *TileAnimSet::getByName(const std::string &nameToFind)
{
    const auto tileAnimation = tileAnimations.find(nameToFind);
    if (tileAnimation == tileAnimations.end()) {
        return nullptr;
    }
    return tileAnimation->second;
}

TileAnim::TileAnim(const ConfigElement &conf)
    :name(conf.getString("name")),
     random(conf.exists("random") ? conf.getInt("random") : 0)
{
    const std::vector<ConfigElement> children = conf.getChildren();
    for (const auto &child: children) {
        if (child.getName() == "transform") {
            TileAnimTransform *transform = TileAnimTransform::create(child);
            transforms.push_back(transform);
        } else if (child.getName() == "context") {
            TileAnimContext *context = TileAnimContext::create(child);
            contexts.push_back(context);
        }
    }
}

TileAnim::~TileAnim()
{
    for(const auto *transform: transforms) {
        delete transform;
    }
    for(const auto *context: contexts) {
        delete context;
    }
}

void TileAnim::draw(
    Image *dest, Tile *tile, MapTile mapTile, const Direction dir
) const
{
    std::vector<TileAnimTransform *>::const_iterator t;
    bool drawn = false;
    /* nothing to do, draw the tile and return! */
    if ((random && xu4_random(100) > random)
        || (transforms.empty() && contexts.empty())
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
            xu4_random(100) < transform->random) {
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
    for (const auto *context: contexts) {
        if (context->isInContext(tile, mapTile, dir)) {
            const TileAnimContext::TileAnimTransformVector
                &ctx_transforms = context->getTransforms();
            for (t = ctx_transforms.cbegin();
                 t != ctx_transforms.cend();
                 ++t) {
                TileAnimTransform *transform = *t;
                if (!transform->random
                    || xu4_random(100) < transform->random) {
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
