/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "debug.h"
#include "error.h"
#include "image.h"
#include "imagemgr.h"
#include "imageview.h"
#include "settings.h"

ImageView::ImageView(int x, int y, int width, int height)
    :View(x, y, width, height)
{
}

ImageView::~ImageView()
{
}


/**
 * Draw the image at the optionally specified offset.
 */
void ImageView::draw(const std::string &imageName, int x, int y) const
{
    ImageInfo *info = imageMgr->get(imageName);
    if (info) {
        info->image->draw(SCALED(this->x + x), SCALED(this->y + y));
        return;
    }
    SubImage *subimage = imageMgr->getSubImage(imageName);
    if (subimage) {
        info = imageMgr->get(subimage->srcImageName);
        if (info) {
            info->image->drawSubRect(
                SCALED(this->x + x),
                SCALED(this->y + y),
                SCALED(subimage->x) / info->prescale,
                SCALED(subimage->y) / info->prescale,
                SCALED(subimage->width) / info->prescale,
                SCALED(subimage->height) / info->prescale
            );
            return;
        }
    }
    errorFatal(
        "ERROR 1005: Unable to load the image \"%s\".\t\n\n"
        "Is %s installed?\n\n"
        "Visit the XU4 website for additional information.\n"
        "\thttp://xu4.sourceforge.net/",
        imageName.c_str(),
        settings.game.c_str()
    );
}
