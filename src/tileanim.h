/*
 * $id: tileanim.h 3019 2012-03-18 11:31:13Z daniel_santos $
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include <map>
#include <string>
#include <vector>

#include "direction.h"

class ConfigElement;
class Image;
class MapTile;
class Tile;
class RGBA;


/**
 * The interface for tile animation transformations.
 */
class TileAnimTransform {
public:
    TileAnimTransform()
        :random(0)
    {
    }

    TileAnimTransform(const TileAnimTransform &) = delete;
    TileAnimTransform(TileAnimTransform &&) = delete;
    TileAnimTransform &operator=(const TileAnimTransform &) = delete;
    TileAnimTransform &operator=(TileAnimTransform &&) = delete;

    virtual ~TileAnimTransform() = default;

    static TileAnimTransform *create(const ConfigElement &conf);
    static RGBA *loadColorFromConf(const ConfigElement &conf);
    virtual void draw(Image *dest, Tile *tile, MapTile mapTile) = 0;

    virtual bool drawsTile() const = 0;
    int random;
};


/**
 * A tile animation transformation that turns a piece of the tile
 * upside down.  Used for animating the flags on building and ships.
 */
class TileAnimInvertTransform:public TileAnimTransform {
public:
    TileAnimInvertTransform(int x, int y, int w, int h);
    void draw(Image *dest, Tile *tile, MapTile mapTile) override;
    bool drawsTile() const override;

private:
    int x, y, w, h;
};


/**
 * A tile animation transformation that changes a single pixels to a
 * random color selected from a list.  Used for animating the
 * campfire in EGA mode.
 */
class TileAnimPixelTransform:public TileAnimTransform {
public:
    TileAnimPixelTransform(int x, int y);
    ~TileAnimPixelTransform() override;
    void draw(Image *dest, Tile *tile, MapTile mapTile) override;
    bool drawsTile() const override;
    int x, y;
    std::vector<RGBA *> colors;
};


/**
 * A tile animation transformation that scrolls the tile's contents
 * vertically within the tile's boundaries.
 */
class TileAnimScrollTransform:public TileAnimTransform {
public:
    explicit TileAnimScrollTransform(int i);
    void draw(Image *dest, Tile *tile, MapTile mapTile) override;
    bool drawsTile() const override;

private:
    int increment, current, lastOffset;
};


/**
 * A tile animation transformation that randomizes the tile's contents
 * vertically within the tile's boundaries.
 */
class TileAnimScrambleTransform:public TileAnimTransform {
public:
    TileAnimScrambleTransform() = default;

    void draw(Image *dest, Tile *tile, MapTile mapTile) override;
    bool drawsTile() const override;
};


/**
 * A tile animation transformation that advances the tile's frame
 * by 1.
 */
class TileAnimFrameTransform:public TileAnimTransform {
public:
    TileAnimFrameTransform()
        :currentFrame(0)
    {
    }

    void draw(Image *dest, Tile *tile, MapTile mapTile) override;
    bool drawsTile() const override;

protected:
    int currentFrame;
};


/**
 * A tile animation transformation that changes pixels with colors
 * that fall in a given range to another color.  Used to animate
 * the campfire in VGA mode.
 */
class TileAnimPixelColorTransform:public TileAnimTransform {
public:
    TileAnimPixelColorTransform(int x, int y, int w, int h);

    TileAnimPixelColorTransform(const TileAnimPixelColorTransform &) = delete;
    TileAnimPixelColorTransform(TileAnimPixelColorTransform &&) = delete;
    TileAnimPixelColorTransform &operator=(
        const TileAnimPixelColorTransform &
    ) = delete;
    TileAnimPixelColorTransform &operator=(
        TileAnimPixelColorTransform &&
    ) = delete;

    ~TileAnimPixelColorTransform() override;
    void draw(Image *dest, Tile *tile, MapTile mapTile) override;
    bool drawsTile() const override;
    int x, y, w, h;
    RGBA *start, *end;
};


/**
 * A context in which to perform the animation
 */
class TileAnimContext {
public:
    typedef std::vector<TileAnimTransform *> TileAnimTransformVector;

    typedef enum {
        FRAME,
        DIR
    } Type;

    TileAnimContext() = default;

    static TileAnimContext *create(const ConfigElement &conf);
    void add(TileAnimTransform *);
    virtual bool isInContext(
        const Tile *t, MapTile mapTile, Direction d
    ) const = 0;

    const TileAnimTransformVector &getTransforms() const
    {
        return animTransforms;   /**< Returns a list of
                                    transformations under the
                                    context. */
    }

    virtual ~TileAnimContext();

private:
    TileAnimTransformVector animTransforms;
};


/**
 * An animation context which changes the animation based on the tile's
 * current frame
 */
class TileAnimFrameContext:public TileAnimContext {
public:
    explicit TileAnimFrameContext(int f);
    bool isInContext(
        const Tile *t, MapTile mapTile, Direction d
    ) const override;

private:
    int frame;
};


/**
 * An animation context which changes the animation based on the player's
 * current facing direction
 */
class TileAnimPlayerDirContext:public TileAnimContext {
public:
    explicit TileAnimPlayerDirContext(Direction d);
    bool isInContext(
        const Tile *t, MapTile mapTile, Direction d
    ) const override;

private:
    Direction dir;
};


/**
 * Instructions for animating a tile.  Each tile animation is made up
 * of a list of transformations which are applied to the tile after it
 * is drawn.
 */
class TileAnim {
public:
    explicit TileAnim(const ConfigElement &conf);
    ~TileAnim();
    std::string name;
    std::vector<TileAnimTransform *> transforms;
    std::vector<TileAnimContext *> contexts;
    /* returns the frame to set the mapTile to (only relevant
       if persistent) */
    void draw(Image *dest, Tile *tile, MapTile mapTile, Direction dir) const;
    int random; /* true if the tile animation occurs randomly */
};


/**
 * A set of tile animations.  Tile animations are associated with a
 * specific image set which shares the same name.
 */
class TileAnimSet {
    typedef std::map<std::string, TileAnim *> TileAnimMap;

public:
    explicit TileAnimSet(const ConfigElement &conf);
    ~TileAnimSet();
    TileAnim *getByName(const std::string &nameToFind);
    std::string name;
    TileAnimMap tileAnimations;
};

#endif // TILEANIM_H
