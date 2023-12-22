/*
 * $Id$
 */

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#if __GNUC__
#define PRINTF_LIKE(x, y) __attribute__((format(printf, (x), (y))))
#else
#define PRINTF_LIKE(x, y)
#endif

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

#include "view.h"
#include "image.h"


/**
 * A view of a text area.  Keeps track of the cursor position.
 */
class TextView:public View {
public:
    TextView(int x, int y, int columns, int rows);
    virtual ~TextView();
    virtual void reinit() override;

    int getCursorX() const
    {
        return cursorX;
    }

    int getCursorY() const
    {
        return cursorY;
    }

    bool getCursorEnabled() const
    {
        return cursorEnabled;
    }

    int getWidth() const
    {
        return columns;
    }

    void drawChar(int chr, int x, int y) const;
    void drawCharMasked(int chr, int x, int y, unsigned char mask) const;
    void textAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(4, 5);
    void scroll();

    void setCursorFollowsText(bool follows)
    {
        cursorFollowsText = follows;
    }

    void setCursorPos(int x, int y, bool clearOld = true);
    void enableCursor();
    void disableCursor();
    void drawCursor();
    static void cursorTimer(void *data);
    // functions to modify the charset font palette
    static void setFontColor(ColorFG fg, ColorBG bg);
    static void setFontColorFG(ColorFG fg);
    static void setFontColorBG(ColorBG bg);
    // functions to add color to strings
    void textSelectedAt(int x, int y, const char *text);
    static std::string colorizeStatus(char statustype);
    static std::string colorizeString(
        std::string input,
        ColorFG color,
        unsigned int colorstart,
        unsigned int colorlength = 0
    );

protected:
    int columns, rows;   /**< size of the view in character cells  */
    bool cursorEnabled; /**< whether the cursor is enabled */
    bool cursorFollowsText; /**< whether cursor is moved past last char */
    int cursorX, cursorY; /**< current position of cursor */
    int cursorPhase; /**< the rotation state of the cursor */
    static Image *charset; /**< image containing font */
};

#endif /* TEXTVIEW_H */
