/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "progress_bar.h"

#include "image.h"
#include "settings.h"

ProgressBar::ProgressBar(
    int x, int y, int width, int height, int _min, int _max
)
    :View(x, y, width, height),
     min(_min),
     max(_max),
     current(_min),
     color(),
     bcolor(),
     bwidth(0)
{
}

ProgressBar &ProgressBar::operator++()
{
    current++;
    draw();
    return *this;
}

ProgressBar &ProgressBar::operator--()
{
    current--;
    draw();
    return *this;
}

void ProgressBar::draw()
{
    Image *bar = Image::create(
        SCALED(width), SCALED(height), false, Image::SOFTWARE
    );
    bar->alphaOff();
    int pos = static_cast<int>(
        (static_cast<double>(current - min) / static_cast<double>(max - min))
                * (width - (bwidth * 2))
    );
    // border color
    bar->fillRect(
        0, 0, SCALED(width), SCALED(height), bcolor.r, bcolor.g, bcolor.b
    );
    // color
    bar->fillRect(
        SCALED(bwidth),
        SCALED(bwidth),
        SCALED(pos),
        SCALED(height - (bwidth * 2)),
        color.r,
        color.g,
        color.b
    );
    bar->drawOn(screen, SCALED(x), SCALED(y));
    update();
    delete bar;
}

void ProgressBar::setBorderColor(int r, int g, int b, int a)
{
    bcolor.r = r;
    bcolor.g = g;
    bcolor.b = b;
    bcolor.a = a;
}

void ProgressBar::setBorderWidth(unsigned int width)
{
    bwidth = width;
}

void ProgressBar::setColor(int r, int g, int b, int a)
{
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
}
