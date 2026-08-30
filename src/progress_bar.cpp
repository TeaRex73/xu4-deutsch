/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "progress_bar.h"

#include "image.h"
#include "view.h"

ProgressBar::ProgressBar(
    const int x,
    const int y,
    const int width,
    const int height,
    const int _min,
    const int _max
)
    :View(x, y, width, height),
     min(_min),
     max(_max),
     current(_min),
     border_width(0)
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
    const Image *bar = Image::create(
        SCALED(width), SCALED(height), false, Image::SOFTWARE
    );
    bar->alphaOff();
    const int pos = static_cast<int>(
        static_cast<double>(current - min) / static_cast<double>(max - min)
                * (width - border_width * 2)
    );
    // border color
    bar->fillRect(
        0, 0, SCALED(width), SCALED(height), border_color.r, border_color.g, border_color.b
    );
    // color
    bar->fillRect(
        SCALED(border_width),
        SCALED(border_width),
        SCALED(pos),
        SCALED(height - border_width * 2),
        color.r,
        color.g,
        color.b
    );
    bar->drawOn(screen, SCALED(x), SCALED(y));
    update();
    delete bar;
}

void ProgressBar::setBorderColor(
    const int r, const int g, const int b, const int a
)
{
    border_color.r = r;
    border_color.g = g;
    border_color.b = b;
    border_color.a = a;
}

void ProgressBar::setBorderWidth(const int width)
{
    border_width = width;
}

void ProgressBar::setColor(const int r, const int g, const int b, const int a)
{
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
}
