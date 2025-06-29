/*
 * $Id$
 */

#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include <map>
#include <string>
#include <vector>

#include "image.h"
#include "observer.h"


class ConfigElement;
class Debug;
class ImageSet;
class Settings;


/*
 * The image manager is responsible for loading and keeping track of
 * the various images.
 */
#define BKGD_SHAPES "tiles"
#define BKGD_CHARSET "charset"
#define BKGD_BORDERS "borders"
#define BKGD_INTRO "title"
#define BKGD_OPTIONS_TOP "options_top"
#define BKGD_OPTIONS_BTM "options_btm"
#define BKGD_TREE "tree"
#define BKGD_PORTAL "portal"
#define BKGD_OUTSIDE "outside"
#define BKGD_INSIDE "inside"
#define BKGD_WAGON "wagon"
#define BKGD_GYPSY "gypsy"
#define BKGD_ABACUS "abacus"
#define BKGD_HONECARD "honecard"
#define BKGD_COMPCARD "compcard"
#define BKGD_VALOCARD "valocard"
#define BKGD_JUSTCARD "justcard"
#define BKGD_SACRCARD "sacrcard"
#define BKGD_SPIRCARD "spircard"
#define BKGD_HUMICARD "humicard"
#define BKGD_HONCOM "honcom"
#define BKGD_VALJUS "valjus"
#define BKGD_SACHONOR "sachonor"
#define BKGD_SPIRHUM "spirhum"
#define BKGD_ANIMATE "beasties"
#define BKGD_KEY "key"
#define BKGD_HONESTY "honesty"
#define BKGD_COMPASSN "compassn"
#define BKGD_VALOR "valor"
#define BKGD_JUSTICE "justice"
#define BKGD_SACRIFIC "sacrific"
#define BKGD_HONOR "honor"
#define BKGD_SPIRIT "spirit"
#define BKGD_HUMILITY "humility"
#define BKGD_TRUTH "truth"
#define BKGD_LOVE "love"
#define BKGD_COURAGE "courage"
#define BKGD_STONCRCL "stoncrcl"
#define BKGD_RUNE_INF "rune0"
#define BKGD_SHRINE_HON "rune1"
#define BKGD_SHRINE_COM "rune2"
#define BKGD_SHRINE_VAL "rune3"
#define BKGD_SHRINE_JUS "rune4"
#define BKGD_SHRINE_SAC "rune5"
#define BKGD_SHRINE_HNR "rune6"
#define BKGD_SHRINE_SPI "rune7"
#define BKGD_SHRINE_HUM "rune8"
#define BKGD_GEMTILES "gemtiles"

enum ImageFixup {
    FIXUP_NONE,
    FIXUP_INTRO,
    FIXUP_ABYSS,
    FIXUP_ABACUS,
    FIXUP_DUNGNS,
    FIXUP_BLACKTRANSPARENCYHACK,
    FIXUP_FMTOWNSSCREEN
};


/**
 * Image meta info.
 */
class ImageInfo {
public:
    ImageInfo()
        :name(),
         filename(),
         width(0),
         height(0),
         depth(0),
         prescale(0),
         filetype(),
         tiles(0),
         introOnly(false),
         transparentIndex(0),
         xu4Graphic(false),
         fixup(FIXUP_NONE),
         image(nullptr),
         subImages()
    {
    }

    ~ImageInfo();
    ImageInfo(const ImageInfo &) = delete;
    ImageInfo(ImageInfo &&) = delete;
    ImageInfo &operator=(const ImageInfo &) = delete;
    ImageInfo &operator=(ImageInfo &&) = delete;
    std::string name;
    std::string filename;
    int width, height, depth;
    int prescale;
    std::string filetype;
    int tiles; /**< used to scale the without bleeding
                  colors between adjacent tiles */
    bool introOnly; /**< whether can be freed after the intro */
    int transparentIndex; /**< color index to consider transparent */
    bool xu4Graphic; /**< an original xu4 graphic not
                        part of u4dos or the VGA upgrade */
    ImageFixup fixup; /**< a routine to do miscellaneous
                         fixes to the image */
    Image *image; /**< the image we're describing */
    std::map<std::string, SubImage *> subImages;
    bool hasBlackBackground() const;
};


/**
 * The image manager singleton that keeps track of all the images.
 */
class ImageMgr:Observer<Settings *> {
public:
    static ImageMgr *getInstance();
    static void destroy();
    ImageInfo *get(const std::string &name, bool returnUnscaled = false);
    SubImage *getSubImage(const std::string &name);
    void freeIntroBackgrounds();
    const std::vector<std::string> &getSetNames() const;
    U4FILE *getImageFile(const ImageInfo *info);
    bool imageExists(const ImageInfo *info);

private:
    ImageMgr();
    ImageMgr(const ImageMgr &) = delete;
    ImageMgr(ImageMgr &&) = delete;
    ImageMgr &operator=(const ImageMgr &) = delete;
    ImageMgr &operator=(ImageMgr &&) = delete;
    ~ImageMgr();
    void init();
    ImageSet *loadImageSetFromConf(const ConfigElement &conf);
    static ImageInfo *loadImageInfoFromConf(const ConfigElement &conf);
    static SubImage *loadSubImageFromConf(
        const ImageInfo *info, const ConfigElement &conf
    );
    ImageSet *getSet(const std::string &setname);
    ImageInfo *getInfo(const std::string &name);
    ImageInfo *getInfoFromSet(const std::string &name, ImageSet *imageset);
    static std::string guessFileType(const std::string &filename);
    static void fixupIntro(Image *im, int prescale);
    static void fixupAbyssVision(const Image *im, int prescale);
    static void fixupAbacus(Image *im, int prescale);
    static void fixupDungNS(const Image *im, int prescale);
    static void fixupFMTowns(const Image *im, int prescale);
    void update(Settings *newSettings) override;
    static ImageMgr *instance;
    static ImageInfo *screenInfo;
    std::map<std::string, ImageSet *> imageSets;
    std::vector<std::string> imageSetNames;
    ImageSet *baseSet;
    Debug *logger;
};

#define imageMgr (ImageMgr::getInstance())

#endif /* IMAGEMGR_H */
