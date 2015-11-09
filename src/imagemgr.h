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

using std::string;

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
	~ImageInfo();
	string name;
	string filename;
	int width, height, depth;
	int prescale;
	string filetype;
	int tiles; /**< used to scale the without bleeding
		      colors between adjacent tiles */
	bool introOnly; /**< whether can be freed after the intro */
	int transparentIndex; /**< color index to consider transparent */
	bool xu4Graphic; /**< an original xu4 graphic not
			    part of u4dos or the VGA upgrade */
	ImageFixup fixup; /**< a routine to do miscellaneous
			     fixes to the image */
	Image *image; /**< the image we're describing */
	std::map<string, SubImage *> subImages;
	bool hasBlackBackground();
};


/**
 * The image manager singleton that keeps track of all the images.
 */
class ImageMgr:Observer<Settings *> {
public:
	static ImageMgr *getInstance();
	static void destroy();
	ImageInfo *get(const string &name, bool returnUnscaled = false);
	SubImage *getSubImage(const string &name);
	void freeIntroBackgrounds();
	const std::vector<string> &getSetNames();
	U4FILE *getImageFile(ImageInfo *info);
	bool imageExists(ImageInfo *info);

private:
	ImageMgr();
	~ImageMgr();
	void init();
	ImageSet *loadImageSetFromConf(const ConfigElement &conf);
	ImageInfo *loadImageInfoFromConf(const ConfigElement &conf);
	SubImage *loadSubImageFromConf(const ImageInfo *info,
				       const ConfigElement &conf);
	ImageSet *getSet(const string &setname);
	ImageInfo *getInfo(const string &name);
	ImageInfo *getInfoFromSet(const string &name, ImageSet *set);
	string guessFileType(const string &filename);
	void fixupIntro(Image *im, int prescale);
	void fixupAbyssVision(Image *im, int prescale);
	void fixupAbacus(Image *im, int prescale);
	void fixupDungNS(Image *im, int prescale);
	void fixupFMTowns(Image *im, int prescale);
	void update(Settings *newSettings);
	static ImageMgr *instance;
	std::map<string, ImageSet *> imageSets;
	std::vector<string> imageSetNames;
	ImageSet *baseSet;
	Debug *logger;
};

#define imageMgr (ImageMgr::getInstance())

#endif /* IMAGEMGR_H */
