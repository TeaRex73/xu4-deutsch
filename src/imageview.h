/*
 * $Id$
 */

#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include "view.h"
/**
 * A view for displaying bitmap images.
 */
class ImageView:public View {
public:
    explicit ImageView(
        int x = 0, int y = 0, int width = 320, int height = 200
    );
    virtual ~ImageView();
    void draw(const std::string &imageName, int x = 0, int y = 0) const;
};

#endif /* IMAGEVIEW_H */
