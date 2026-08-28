/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <algorithm>
#include <set>
#include <string>

#include "menu.h"

#include "error.h"
#include "event.h"
#include "menuitem.h"
#include "textcolor.h"
#include "textview.h"

Menu::Menu()
    : closed(false),
      titleX(0),
      titleY(0)
{
}

Menu::~Menu()
{
    for (const auto *item: items) {
        delete item;
    }
}

void Menu::removeAll()
{
    for (const auto *item: items) {
        delete item;
    }
    items.clear();
}


/**
 * Adds an item to the menu list and returns the menu
 */
void Menu::add(
    const int id,
    const std::string &text,
    const short x,
    const short y,
    const int sc
)
{
    auto *item = new MenuItem(text, x, y, sc);
    item->setId(id);
    items.push_back(item);
}

MenuItem *Menu::add(const int id, MenuItem *item)
{
    item->setId(id);
    items.push_back(item);
    return item;
}

void Menu::addShortcutKey(const int id, const int shortcutKey) const
{
    const auto item = std::find_if(
        items.cbegin(),
        items.cend(),
        [&](const MenuItem *v) -> bool {
            return v->getId() == id;
        }
    );
    if (item != items.cend()) {
        (*item)->addShortcutKey(shortcutKey);
    }
}

void Menu::setClosesMenu(const int id) const
{
    const auto item = std::find_if(
        items.cbegin(),
        items.cend(),
        [&](const MenuItem *v) -> bool {
            return v->getId() == id;
        }
    );
    if (item != items.cend()) {
        (*item)->setClosesMenu(true);
    }
}


/**
 * Returns the menu item that is currently selected/highlighted
 */
Menu::MenuItemList::iterator Menu::getCurrent() const
{
    return selected;
}


/**
 * Sets the current menu item to the one indicated by the iterator
 */
void Menu::setCurrent(const MenuItemList::iterator i)
{
    selected = i;
    highlight(*selected);
    MenuEvent event(this, MenuEvent::SELECT);
    setChanged();
    notifyObservers(event);
}

void Menu::setCurrent(const int id)
{
    setCurrent(getById(id));
}

void Menu::show(TextView *view)
{
    if (!title.empty()) {
        view->textAt(titleX, titleY, "%s", title.c_str());
    }
    for (current = items.begin(); current != items.end(); ++current) {
        const MenuItem *mi = *current;
        if (mi->isVisible()) {
            std::string text(mi->getText());
            if (mi->isSelected()) {
                text[0] = '\010';
            }
            if (mi->isHighlighted()) {
                view->textSelectedAt(
                    mi->getX(),
                    mi->getY(),
                    TextView::colorizeString(
                        text, FG_YELLOW, mi->getScOffset(), 1
                    ).c_str()
                );
                // hack for the custom U5 mix reagents menu
                // places cursor 1 column over, rather than 2.
                view->setCursorPos(
                    mi->getX() - (view->getWidth() == 15 ? 1 : 2),
                    mi->getY(),
                    true
                );
                view->enableCursor();
            } else {
                view->textAt(
                    mi->getX(),
                    mi->getY(),
                    "%s",
                    TextView::colorizeString(
                        text, FG_YELLOW, mi->getScOffset(), 1
                    ).c_str()
                );
            }
        }
    }
} // Menu::show


/**
 * Checks the menu to ensure that there is at least 1 visible
 * item in the list.  Returns true if there is at least 1 visible
 * item, false if nothing is visible.
 */
bool Menu::isVisible()
{
    bool visible = false;
    for (current = items.begin(); current != items.end(); ++current) {
        if ((*current)->isVisible()) {
            visible = true;
        }
    }
    return visible;
}


/**
 * Sets the selected iterator to the next visible menu item and highlights it
 */
void Menu::next()
{
    auto item = selected;
    if (isVisible()) {
        if (++item == items.end()) {
            item = items.begin();
        }
        while (!(*item)->isVisible()) {
            if (++item == items.end()) {
                item = items.begin();
            }
        }
    }
    setCurrent(item);
}


/**
 * Sets the selected iterator to the previous visible menu item and
 * highlights it
 */
void Menu::prev()
{
    auto i = selected;
    if (isVisible()) {
        if (i == items.begin()) {
            i = items.end();
        }
        --i;
        while (!(*i)->isVisible()) {
            if (i == items.begin()) {
                i = items.end();
            }
            --i;
        }
    }
    setCurrent(i);
}


/**
 * Highlights a single menu item, un-highlighting any others
 */
void Menu::highlight(MenuItem *item)
{
    // unhighlight all menu items first
    for (current = items.begin(); current != items.end(); ++current) {
        (*current)->setHighlighted(false);
    }
    if (item) {
        item->setHighlighted(true);
    }
}


/**
 * Returns an iterator pointing to the first menu item
 */
Menu::MenuItemList::iterator Menu::begin()
{
    return items.begin();
}


/**
 * Returns an iterator pointing just past the last menu item
 */
Menu::MenuItemList::iterator Menu::end()
{
    return items.end();
}
/**
 * Returns an iterator pointing to the first visible menu item
 */
Menu::MenuItemList::iterator Menu::begin_visible()
{
    if (!isVisible()) {
        return items.end();
    }
    current = items.begin();
    while (current != items.end() && !(*current)->isVisible()) {
        ++current;
    }
    return current;
}


/**
 * 'Resets' the menu.  This does the following:
 *      - un-highlights all menu items
 *      - highlights the first menu item
 *      - selects the first visible menu item
 */
void Menu::reset(const bool highlightFirst)
{
    closed = false;
    /* get the first visible menu item */
    selected = begin_visible();
    /* un-highlight and deselect each menu item */
    for (current = items.begin(); current != items.end(); ++current) {
        (*current)->setHighlighted(false);
        (*current)->setSelected(false);
    }
    /* highlight the first visible menu item */
    if (highlightFirst) {
        highlight(*selected);
    }
    MenuEvent event(this, MenuEvent::RESET);
    setChanged();
    notifyObservers(event);
}


/**
 * Returns an iterator pointing to the item associated with the given 'id'
 */
Menu::MenuItemList::iterator Menu::getById(const int id)
{
    if (id == -1) {
        return getCurrent();
    }
    current = std::find_if(
        items.begin(),
        items.end(),
        [&](const MenuItem *v) -> bool {
            return v->getId() == id;
        }
    );
    return current;
}


/**
 * Returns the menu item associated with the given 'id'
 */
MenuItem *Menu::getItemById(const int id)
{
    current = getById(id);
    if (current != items.end()) {
        return *current;
    }
    return nullptr;
}


/**
 * Activates the menu item given by 'id', using 'action' to
 * activate it.  If the menu item cannot be activated using
 * 'action', then it is not activated.  This also un-highlights
 * the menu item given by 'menu' and highlights the new menu
 * item that was found for 'id'.
 */
void Menu::activateItem(const int id, const MenuEvent::Type action)
{
    MenuItem *mi;
    /* find the given menu item by id */
    if (id >= 0) {
        mi = getItemById(id);
    }
    /* or use the current item */
    else {
        mi = *getCurrent();
    }
    if (!mi) {
        errorFatal(
            "Error: Unable to find menu item with id '%d'", id
        );
    }
    /* make sure the action given will activate the menu item */
    if (mi->getClosesMenu()) {
        setClosed(true);
    }
    MenuEvent event(this, action, mi);
    mi->activate(event);
    setChanged();
    notifyObservers(event);
} // Menu::activateItem


/**
 * Activates a menu item by its shortcut key.  True is returned if a
 * menu item get activated, false otherwise.
 */
bool Menu::activateItemByShortcut(const int key, const MenuEvent::Type action)
{
    return std::any_of(
        items.cbegin(),
        items.cend(),
        [&] (const MenuItem *item) -> bool {
            const std::set<int> &shortcuts = item->getShortcutKeys();
            if (shortcuts.find(key) != shortcuts.end()) {
                activateItem(item->getId(), action);
                // if the selection doesn't close the menu,
                // highlight the selection
                if (!item->getClosesMenu()) {
                    setCurrent(item->getId());
                }
                return true;
            }
            return false;
        }
    );
}


/**
 * Returns true if the menu has been closed.
 */
bool Menu::getClosed() const
{
    return closed;
}


/**
 * Update whether the menu has been closed.
 */
void Menu::setClosed(const bool isClosed)
{
    closed = isClosed;
}

void Menu::setTitle(const std::string &text, const int x, const int y)
{
    title = text;
    titleX = x;
    titleY = y;
}

MenuController::MenuController(Menu *menu, TextView *view)
    :menu(menu), view(view)
{
}

bool MenuController::keyPressed(const int key)
{
    bool handled = true;
    const bool cursorOn = view->getCursorEnabled();
    if (cursorOn) {
        view->disableCursor();
    }
    switch (key) {
    case U4_UP:
        menu->prev();
        break;
    case U4_DOWN:
        menu->next();
        break;
    case U4_LEFT:
    case U4_RIGHT:
    case U4_ENTER:
    {
        MenuEvent::Type action = MenuEvent::ACTIVATE;
        if (key == U4_LEFT) {
            action = MenuEvent::DECREMENT;
        } else if (key == U4_RIGHT) {
            action = MenuEvent::INCREMENT;
        }
        menu->activateItem(-1, action);
        break;
    }
    default:
        handled = menu->activateItemByShortcut(key, MenuEvent::ACTIVATE);
    }
    menu->show(view);
    if (cursorOn) {
        view->enableCursor();
    }
    view->update();
    if (menu->getClosed()) {
        doneWaiting();
    }
    return handled;
} // MenuController::keyPressed
