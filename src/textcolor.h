/*
 * $Id$
 */

#ifndef TEXTCOLOR_H
#define TEXTCOLOR_H

#define TEXT_BG_INDEX 0
#define TEXT_FG_PRIMARY_INDEX 15
#define TEXT_FG_SECONDARY_INDEX 7
#define TEXT_FG_SHADOW_INDEX 80


//
// text foreground colors
//
enum ColorFG {
    FG_GREY = '\023',
    FG_BLUE = '\024',
    FG_PURPLE = '\025',
    FG_GREEN = '\026',
    FG_RED = '\027',
    FG_YELLOW = '\030',
    FG_WHITE = '\031'
};


//
// text background colors
//
enum ColorBG {
    BG_NORMAL = '\032',
    BG_BRIGHT = '\033'
};

#endif /* TEXTCOLOR_H */
