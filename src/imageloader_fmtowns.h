/*
 * imageloader_fmtowns.h
 *
 *  Created on: Jun 23, 2011
 *      Author: Darren
 */

#ifndef IMAGELOADER_FMTOWNS_H_
#define IMAGELOADER_FMTOWNS_H_

#include "imageloader.h"

class FMTOWNSImageLoader:public ImageLoader {
public:
    virtual Image *load(U4FILE *file, int width, int height, int bpp) override;

    explicit FMTOWNSImageLoader(int offset)
        :offset(offset)
    {
    }

protected:
    int offset;

private:
    static ImageLoader *instance_pic;
    static ImageLoader *instance_tif;
};

#endif /* IMAGELOADER_FMTOWNS_H_ */
