/*
 * $Id$
 */

#ifndef MENU_H
#define MENU_H

#include <list>
#include <string>
#include "event.h"
#include "menuitem.h"
#include "observable.h"
#include "types.h"



class Menu;
class TextView;

class MenuEvent {
public:
    enum Type {
        ACTIVATE,
        INCREMENT,
        DECREMENT,
        SELECT,
        RESET
    };

    MenuEvent(const Menu *menu, Type type, const MenuItem *item = nullptr)
        :menu(menu), type(type), item(item)
    {
    }

    const Menu *getMenu() const
    {
        return menu;
    }

    Type getType() const
    {
        return type;
    }

    const MenuItem *getMenuItem() const
    {
        return item;
    }

private:
    const Menu *menu;
    Type type;
    const MenuItem *item;
};


/**
 * Menu class definition
 */
class Menu:public Observable<Menu *, MenuEvent &> {
public:
    typedef std::list<MenuItem *> MenuItemList;
    Menu();
    ~Menu();
    void removeAll();
    void add(int id, const std::string &text, short x, short y, int sc = -1);
    MenuItem *add(int id, MenuItem *item);
    void addShortcutKey(int id, int shortcutKey) const;
    void setClosesMenu(int id) const;
    MenuItemList::iterator getCurrent() const;
    void setCurrent(MenuItemList::iterator i);
    void setCurrent(int id);
    void show(TextView *view);
    bool isVisible();
    void next();
    void prev();
    void highlight(MenuItem *item);
    MenuItemList::iterator begin();
    MenuItemList::iterator end();
    MenuItemList::iterator begin_visible();
    void reset(bool highlightFirst = true);
    MenuItemList::iterator getById(int id);
    MenuItem *getItemById(int id);
    void activateItem(int id, MenuEvent::Type action);
    bool activateItemByShortcut(int key, MenuEvent::Type action);
    bool getClosed() const;
    void setClosed(bool closed);
    void setTitle(const std::string &text, int x, int y);

private:
    MenuItemList items;
    MenuItemList::iterator current;
    MenuItemList::iterator selected;
    bool closed;
    std::string title;
    int titleX, titleY;
};


/**
 * This class controls a menu.  The value field of WaitableController
 * isn't used.
 */
class MenuController:public WaitableController<void *> {
public:
    MenuController(Menu *menu, TextView *view);
    MenuController(const MenuController &) = delete;
    MenuController(MenuController &&) = delete;
    MenuController &operator=(const MenuController &) = delete;
    MenuController &operator=(MenuController &&) = delete;
    bool keyPressed(int key) override;

protected:
    Menu *menu;
    TextView *view;
};

#endif // ifndef MENU_H
