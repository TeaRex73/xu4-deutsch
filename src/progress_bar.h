/*
 * $Id$
 */

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "image.h"
#include "view.h"

class ProgressBar:public View {
public:
    ProgressBar(int x, int y, int width, int height, int _min, int _max);
    virtual ProgressBar &operator++();
    virtual ProgressBar &operator--();
    virtual void draw();
    void setBorderColor(int r, int g, int b, int a = IM_OPAQUE);
    void setBorderWidth(int width);
    void setColor(int r, int g, int b, int a = IM_OPAQUE);

protected:
    int min, max;
    int current;
    RGBA color, border_color;
    int border_width;
};

#endif // PROGRESS_BAR_H
