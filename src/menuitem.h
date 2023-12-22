/*
 * $Id$
 */

#ifndef MENUITEM_H
#define MENUITEM_H

#include <string>
#include <set>
#include <vector>



class MenuEvent;


/**
 * custom output types for with menu items that need
 * to perform special calculations before displaying
 * its associated value
 */
typedef enum {
    MENU_OUTPUT_INT,
    MENU_OUTPUT_GAMMA,
    MENU_OUTPUT_SHRINE,
    MENU_OUTPUT_SPELL,
    MENU_OUTPUT_VOLUME,
    MENU_OUTPUT_REAGENT
} menuOutputType;

class MenuItem {
public:
    MenuItem(const std::string &t, short xpos, short ypos, int sc = -1);

    MenuItem(const MenuItem &) = delete;
    MenuItem(MenuItem &&) = delete;
    MenuItem &operator=(const MenuItem &) = delete;
    MenuItem &operator=(MenuItem &&) = delete;
    virtual ~MenuItem()
    {
    }

    virtual void activate(MenuEvent &)
    {
    }

    int getId() const;
    short getX() const;
    short getY() const;
    int getScOffset() const;
    virtual std::string getText() const;
    bool isHighlighted() const;
    bool isSelected() const;
    bool isVisible() const;
    const std::set<int> &getShortcutKeys() const;
    bool getClosesMenu() const;
    void setId(int i);
    void setX(int xpos);
    void setY(int ypos);
    void setText(const std::string &t);
    void setHighlighted(bool h = true);
    void setSelected(bool s = true);
    void setVisible(bool v = true);
    void addShortcutKey(int sc);
    void setClosesMenu(bool closesMenu);

protected:
    int id;
    short x, y;
    std::string text;
    bool highlighted;
    bool selected;
    bool visible;
    int scOffset;
    std::set<int> shortcutKeys;
    bool closesMenu;
};


/**
 * A menu item that toggles a boolean value, and displays the current
 * setting as part of the text.
 */
class BoolMenuItem:public MenuItem {
public:
    BoolMenuItem(
        const std::string &text, short x, short y, int shortcutKey, bool *val
    );
    BoolMenuItem(const BoolMenuItem &) = delete;
    BoolMenuItem(BoolMenuItem &&) = delete;
    BoolMenuItem &operator=(const BoolMenuItem &) = delete;
    BoolMenuItem &operator=(BoolMenuItem &&) = delete;

    BoolMenuItem *setValueStrings(
        const std::string &onString, const std::string &offString
    );
    virtual void activate(MenuEvent &event) override;
    virtual std::string getText() const override;

protected:
    bool *val;
    std::string on, off;
};


/**
 * A menu item that cycles through a list of possible std::string values, and
 * displays the current setting as part of the text.
 */
class StringMenuItem:public MenuItem {
public:
    StringMenuItem(
        const std::string &text,
        short x,
        short y,
        int shortcutKey,
        std::string *val,
        const std::vector<std::string> &validSettings
    );
    StringMenuItem(const StringMenuItem &) = delete;
    StringMenuItem(StringMenuItem &&) = delete;
    StringMenuItem &operator=(const StringMenuItem &) = delete;
    StringMenuItem &operator=(StringMenuItem &&) = delete;

    virtual void activate(MenuEvent &event) override;
    virtual std::string getText() const override;

protected:
    std::string *val;
    std::vector<std::string> validSettings;
};


/**
 * A menu item that cycles through a list of possible integer values,
 * and displays the current setting as part of the text.
 */
class IntMenuItem:public MenuItem {
public:
    IntMenuItem(
        const std::string &text,
        short x,
        short y,
        int shortcutKey,
        int *val,
        int min,
        int max,
        int increment,
        menuOutputType output = MENU_OUTPUT_INT
    );
    IntMenuItem(const IntMenuItem &) = delete;
    IntMenuItem(IntMenuItem &&) = delete;
    IntMenuItem &operator=(const IntMenuItem &) = delete;
    IntMenuItem &operator=(IntMenuItem &&) = delete;

    virtual void activate(MenuEvent &event) override;
    virtual std::string getText() const override;

protected:
    int *val;
    int min, max, increment;
    menuOutputType output;
};

/**
 * A menu item that cycles through a list of possible integer values,
 * and displays the current setting as part of the text.
 */
class UnsignedShortMenuItem:public MenuItem {
public:
    UnsignedShortMenuItem(
        const std::string &text,
        short x,
        short y,
        int shortcutKey,
        unsigned short *val,
        unsigned short min,
        unsigned short max,
        unsigned short increment,
        menuOutputType output = MENU_OUTPUT_INT
    );
    UnsignedShortMenuItem(const UnsignedShortMenuItem &) = delete;
    UnsignedShortMenuItem(UnsignedShortMenuItem &&) = delete;
    UnsignedShortMenuItem &operator=(const UnsignedShortMenuItem &) = delete;
    UnsignedShortMenuItem &operator=(UnsignedShortMenuItem &&) = delete;

    virtual void activate(MenuEvent &event) override;
    virtual std::string getText() const override;

protected:
    unsigned short *val;
    unsigned short min, max, increment;
    menuOutputType output;
};

#endif // ifndef MENUITEM_H
