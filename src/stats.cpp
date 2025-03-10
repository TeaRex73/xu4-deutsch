/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "u4.h"

#include "stats.h"

#include <cstring>

#include "armor.h"
#include "context.h"
#include "debug.h"
#include "menu.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "spell.h"
#include "tile.h"
#include "weapon.h"
#include "utils.h"

extern bool verbose;
/**
 * StatsArea class implementation
 */
StatsArea::StatsArea()
    :title(
        STATS_AREA_X * CHAR_WIDTH,
        0 * CHAR_HEIGHT,
        STATS_AREA_WIDTH,
        1
    ),
     mainArea(
         STATS_AREA_X * CHAR_WIDTH,
         STATS_AREA_Y * CHAR_HEIGHT,
         STATS_AREA_WIDTH,
         STATS_AREA_HEIGHT
     ),
     summary(
         STATS_AREA_X * CHAR_WIDTH,
         (STATS_AREA_Y + STATS_AREA_HEIGHT + 1) * CHAR_HEIGHT,
         STATS_AREA_WIDTH,
         1
     ),
     view(STATS_PARTY_OVERVIEW),
     reagentsMixMenu()
{
    // Generate a formatted std::string for each menu item,
    // and then add the item to the menu.  The Y value
    // for each menu item will be filled in later.
    for (int count = 0; count < 8; count++) {
        char outputBuffer[16];
        std::snprintf(
            outputBuffer,
            sizeof(outputBuffer),
            "-%-11s%%s",
            uppercase(getReagentName(static_cast<Reagent>(count))).c_str()
        );
        reagentsMixMenu.add(
            count,
            new UnsignedShortMenuItem(
                outputBuffer,
                1,
                0,
                -1,
                c->party->getReagentPtr(static_cast<Reagent>(count)),
                0,
                99,
                1,
                MENU_OUTPUT_REAGENT
            )
        );
    }
    reagentsMixMenu.addObserver(this);
}

void StatsArea::setView(StatsView view)
{
    this->view = view;
    update();
}


/**
 * Sets the stats item to the previous in sequence.
 */
void StatsArea::prevItem()
{
    view = static_cast<StatsView>(view - 1);
    if (view < STATS_CHAR1) {
        view = STATS_MIXTURES;
    }
    if ((view <= STATS_CHAR8)
        && ((view - STATS_CHAR1 + 1) > c->party->size())) {
        view = static_cast<StatsView>(STATS_CHAR1 - 1 + c->party->size());
    }
    update();
}


/**
 * Sets the stats item to the next in sequence.
 */
void StatsArea::nextItem()
{
    view = static_cast<StatsView>(view + 1);
    if (view > STATS_MIXTURES) {
        view = STATS_CHAR1;
    }
    if ((view <= STATS_CHAR8)
        && ((view - STATS_CHAR1 + 1) > c->party->size())) {
        view = STATS_WEAPONS;
    }
    update();
}


/**
 * Update the stats (ztats) box on the upper right of the screen.
 */
void StatsArea::update()
{
    clear();
    /*
     * update the upper stats box
     */
    switch (view) {
    case STATS_PARTY_OVERVIEW:
        showPartyView(false);
        break;
    case STATS_PARTY_AVATARONLY:
        showPartyView(true);
        break;
    case STATS_CHAR1:
    case STATS_CHAR2:
    case STATS_CHAR3:
    case STATS_CHAR4:
    case STATS_CHAR5:
    case STATS_CHAR6:
    case STATS_CHAR7:
    case STATS_CHAR8:
        showPlayerDetails();
        break;
    case STATS_WEAPONS:
        showWeapons();
        break;
    case STATS_ARMOR:
        showArmor();
        break;
    case STATS_EQUIPMENT:
        showEquipment();
        break;
    case STATS_ITEMS:
        showItems();
        break;
    case STATS_REAGENTS:
        showReagents();
        break;
    case STATS_MIXTURES:
        showMixtures();
        break;
    case MIX_REAGENTS:
        showReagents(true);
        break;
    }
    /*
     * update the lower stats box (food, gold, etc.)
     */
    if (c->transportContext == TRANSPORT_SHIP) {
        summary.textAt(
            0,
            0,
            "L:%04d   SCH:%02d",
            c->saveGame->food / 100,
            c->saveGame->shiphull
        );
    } else {
        summary.textAt(
            0,
            0,
            "L:%04d   G:%04d",
            c->saveGame->food / 100,
            c->saveGame->gold
        );
    }
    update(c->aura);
    redraw();
} // StatsArea::update

void StatsArea::update(Aura *aura)
{
    unsigned char mask = 0xff;
    for (int i = 0; i < VIRT_MAX; i++) {
        if (c->saveGame->karma[i] == 0) {
            mask &= ~(1 << i);
        }
    }
    switch (aura->getType()) {
    case Aura::NONE:
        summary.drawCharMasked(0, STATS_AREA_WIDTH / 2, 0, mask);
        break;
    case Aura::HORN:
        summary.drawChar(CHARSET_REDDOT, STATS_AREA_WIDTH / 2, 0);
        break;
    case Aura::JINX:
        summary.drawChar('J', STATS_AREA_WIDTH / 2, 0);
        break;
    case Aura::NEGATE:
        summary.drawChar('N', STATS_AREA_WIDTH / 2, 0);
        break;
    case Aura::PROTECTION:
        summary.drawChar('P', STATS_AREA_WIDTH / 2, 0);
        break;
    case Aura::QUICKNESS:
        summary.drawChar('Q', STATS_AREA_WIDTH / 2, 0);
        break;
    }
    summary.update();
} // StatsArea::update

void StatsArea::highlightPlayer(int player)
{
    U4ASSERT(
        player < c->party->size(), "player number out of range: %d", player
    );
    mainArea.highlight(
        0, player * CHAR_HEIGHT, STATS_AREA_WIDTH * CHAR_WIDTH, CHAR_HEIGHT
    );
}

void StatsArea::clear()
{
    for (int i = 0; i < STATS_AREA_WIDTH; i++) {
        title.drawChar(CHARSET_HORIZBAR, i, 0);
    }
    mainArea.clear();
    summary.clear();
}


/**
 * Redraws the entire stats area
 */
void StatsArea::redraw()
{
    title.update();
    mainArea.update();
    summary.update();
}


/**
 * Sets the title of the stats area.
 */
void StatsArea::setTitle(const std::string &s)
{
    int titleStart = (STATS_AREA_WIDTH / 2) - ((s.length() + 2) / 2);
    title.textAt(titleStart, 0, "%c%s%c", 16, s.c_str(), 17);
}


/**
 * The basic party view.
 */
void StatsArea::showPartyView(bool avatarOnly)
{
    const char *format = "%d%c%-9.8s%03d%s";
    PartyMember *p = nullptr;
    int activePlayer = c->party->getActivePlayer();
    U4ASSERT(
        c->party->size() <= 8,
        "party members out of range: %d",
        c->party->size()
    );
    if (!avatarOnly) {
        for (int i = 0; i < c->party->size(); i++) {
            p = c->party->member(i);
            mainArea.textAt(
                0,
                i,
                format,
                i + 1,
                (i == activePlayer) ? CHARSET_BULLET : '-',
                uppercase(p->getName()).c_str(),
                p->getHp(),
                mainArea.colorizeStatus(p->getStatus()).c_str()
            );
        }
    } else {
        p = c->party->member(0);
        mainArea.textAt(
            0,
            0,
            format,
            1,
            (activePlayer == 0) ? CHARSET_BULLET : '-',
            uppercase(p->getName()).c_str(), p->getHp(),
            mainArea.colorizeStatus(p->getStatus()).c_str()
        );
    }
}


/**
 * The individual character view.
 */
void StatsArea::showPlayerDetails()
{
    int player = view - STATS_CHAR1;
    U4ASSERT(player < 8, "character number out of range: %d", player);
    PartyMember *p = c->party->member(player);
    char titleText[16];
    std::sprintf(titleText, "SPL-%d", player + 1);
    setTitle(titleText);
    std::string nameStr = uppercase(p->getName());
    int nameStart = (STATS_AREA_WIDTH - nameStr.length()) / 2;
    mainArea.textAt(0, 1, "%c             %c", p->getSex(), p->getStatus());
    mainArea.textAt(nameStart, 0, "%s", nameStr.c_str());
    std::string classStr =
        uppercase(getClassNameTranslated(p->getClass(), p->getSex()));
    int classStart = (STATS_AREA_WIDTH - classStr.length()) / 2;
    mainArea.textAt(classStart, 1, "%s", classStr.c_str());
    mainArea.textAt(0, 2, " MP:%02d  ST:%d", p->getMp(), p->getRealLevel());
    mainArea.textAt(0, 3, "STR:%02d  TP:%04d", p->getStr(), p->getHp());
    mainArea.textAt(0, 4, "GES:%02d  TM:%04d", p->getDex(), p->getMaxHp());
    mainArea.textAt(0, 5, "INT:%02d  EP:%04d", p->getInt(), p->getExp());
    mainArea.textAt(
        0,
        6,
        "W:%s",
        uppercase(p->getWeapon()->getName()).c_str()
    );
    mainArea.textAt(
        0,
        7,
        "R:%s",
        uppercase(p->getArmor()->getName()).c_str()
    );
}


/**
 * Weapons in inventory.
 */
void StatsArea::showWeapons()
{
    setTitle("WAFFEN");
    int line = 0;
    int col = 0;
    mainArea.textAt(0, line++, "A-H[NDE");
    for (int w = WEAP_HANDS + 1; w < WEAP_MAX; w++) {
        int n = c->saveGame->weapons[w];
        if (n >= 100) {
            n = 99;
        }
        if (n >= 1) {
            const char *format = (n >= 10) ? "%c%d-%s" : "%c-%d-%s";
            mainArea.textAt(
                col,
                line++,
                format,
                w - WEAP_HANDS + 'A',
                n,
                uppercase(
                    Weapon::get(static_cast<WeaponType>(w))->getAbbrev()
                ).c_str()
            );
            if (line >= (STATS_AREA_HEIGHT)) {
                line = 0;
                col += 8;
            }
        }
    }
}


/**
 * Armor in inventory.
 */
void StatsArea::showArmor()
{
    setTitle("R]STUNG");
    int line = 0;
    mainArea.textAt(0, line++, "A-KEINE");
    for (int a = ARMR_NONE + 1; a < ARMR_MAX; a++) {
        if (c->saveGame->armor[a] > 0) {
            const char *format =
                (c->saveGame->armor[a] >= 10) ? "%c%d-%s" : "%c-%d-%s";
            mainArea.textAt(
                0,
                line++,
                format,
                a - ARMR_NONE + 'A',
                c->saveGame->armor[a],
                uppercase(
                    Armor::get(static_cast<ArmorType>(a))->getName()
                ).c_str()
            );
        }
    }
}


/**
 * Equipment: touches, gems, keys, and sextants.
 */
void StatsArea::showEquipment()
{
    setTitle("WERKZEUG");
    int line = 0;
    mainArea.textAt(0, line++, "%02d-FACKELN", c->saveGame->torches);
    mainArea.textAt(0, line++, "%02d-JUWELEN", c->saveGame->gems);
    mainArea.textAt(0, line++, "%02d-DIETRICHE", c->saveGame->keys);
    if (c->saveGame->sextants > 0) {
        mainArea.textAt(0, line++, "%02d-SEXTANTEN", c->saveGame->sextants);
    }
}


/**
 * Items: runes, stones, and other miscellaneous quest items.
 */
void StatsArea::showItems()
{
    int i, j;
    char buffer[255];
    setTitle("GEGENST[NDE");
    int line = 0;
    if (c->saveGame->stones != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->stones & (1 << i)) {
                buffer[j++] = getStoneName(static_cast<Virtue>(i))[0];
            }
        }
        buffer[j] = '\0';
        mainArea.textAt(0, line++, "STEINE:%s", buffer);
    }
    if (c->saveGame->runes != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->runes & (1 << i)) {
                buffer[j++] = getVirtueName(static_cast<Virtue>(i))[0];
            }
        }
        buffer[j] = '\0';
        mainArea.textAt(0, line++, "RUNEN:%s", buffer);
    }
    if (c->saveGame->items & (ITEM_CANDLE | ITEM_BOOK | ITEM_BELL)) {
        buffer[0] = '\0';
        if (c->saveGame->items & ITEM_BELL) {
            std::strcat(buffer, getItemName(ITEM_BELL));
            std::strcat(buffer, " ");
        }
        if (c->saveGame->items & ITEM_BOOK) {
            std::strcat(buffer, getItemName(ITEM_BOOK));
            std::strcat(buffer, " ");
        }
        if (c->saveGame->items & ITEM_CANDLE) {
            std::strcat(buffer, getItemName(ITEM_CANDLE));
            // all three together are too long in German
            if (std::strcmp(buffer, "GLOCKE BUCH KERZE") == 0) {
                buffer[0] = '\0';
                std::strcat(buffer, "GLOCK BUCH KERZ");
                buffer[15] = '\0';
            }
        }
        mainArea.textAt(0, line++, "%s", buffer);
    }
    if (c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        j = 0;
        if (c->saveGame->items & ITEM_KEY_T) {
            buffer[j++] = getItemName(ITEM_KEY_T)[0];
        }
        if (c->saveGame->items & ITEM_KEY_L) {
            buffer[j++] = getItemName(ITEM_KEY_L)[0];
        }
        if (c->saveGame->items & ITEM_KEY_C) {
            buffer[j++] = getItemName(ITEM_KEY_C)[0];
        }
        buffer[j] = '\0';
        mainArea.textAt(0, line++, "SCHL]SSEL:%s", buffer);
    }
    if (c->saveGame->items & ITEM_HORN) {
        mainArea.textAt(0, line++, "%s", getItemName(ITEM_HORN));
    }
    if (c->saveGame->items & ITEM_WHEEL) {
        mainArea.textAt(0, line++, "%s", getItemName(ITEM_WHEEL));
    }
    if (c->saveGame->items & ITEM_SKULL) {
        mainArea.textAt(0, line++, "%s", getItemName(ITEM_SKULL));
    }
} // StatsArea::showItems


/**
 * Unmixed reagents in inventory.
 */
void StatsArea::showReagents(bool active)
{
    setTitle("REAGENZIEN");
    Menu::MenuItemList::const_iterator i;
    int line = 0, r = REAG_ASH;
    std::string shortcut("A");
    reagentsMixMenu.show(&mainArea);
    for (i = reagentsMixMenu.begin(); i != reagentsMixMenu.end(); ++i, ++r) {
        if ((*i)->isVisible()) {
            // Insert the reagent menu item shortcut character
            shortcut[0] = 'A' + r;
            if (active) {
                mainArea.textAt(
                    0,
                    line++,
                    "%s",
                    mainArea.colorizeString(shortcut, FG_YELLOW, 0, 1).c_str()
                );
            } else {
                mainArea.textAt(0, line++, "%s", shortcut.c_str());
            }
        }
    }
}


/**
 * Mixed reagents in inventory.
 */
void StatsArea::showMixtures()
{
    setTitle("MIXTUREN");
    int line = 0;
    int col = 0;
    for (int s = 0; s < SPELL_MAX; s++) {
        int n = c->saveGame->mixtures[s];
        if (n >= 100) {
            n = 99;
        }
        if (n >= 1) {
            mainArea.textAt(col, line++, "%c-%02d", s + 'A', n);
            if (line >= (STATS_AREA_HEIGHT)) {
                if (col >= 10) {
                    break;
                }
                line = 0;
                col += 5;
            }
        }
    }
} // StatsArea::showMixtures

void StatsArea::resetReagentsMenu()
{
    Menu::MenuItemList::const_iterator current;
    int i = 0, row = 0;
    for (current = reagentsMixMenu.begin();
         current != reagentsMixMenu.end();
         ++current) {
        if (c->saveGame->reagents[i++] > 0) {
            (*current)->setVisible(true);
            (*current)->setY(row++);
        } else {
            (*current)->setVisible(false);
        }
    }
    reagentsMixMenu.reset(false);
}


/**
 * Handles spell mixing for the Ultima V-style menu-system
 */
bool ReagentsMenuController::keyPressed(int key)
{
    if ((key >= 'A') && (key <= ']')) {
        key = xu4_tolower(key);
    }
    switch (key) {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    {
        /* select the corresponding reagent (if visible) */
        Menu::MenuItemList::iterator mi = menu->getById(key - 'a');
        if ((*mi)->isVisible()) {
            menu->setCurrent(menu->getById(key - 'a'));
            keyPressed(U4_SPACE);
        }
        break;
    }
    case U4_LEFT:
    case U4_RIGHT:
    case U4_SPACE:
        if (menu->isVisible()) {
            MenuItem *item = *menu->getCurrent();
            /* change whether or not it's selected */
            item->setSelected(!item->isSelected());
            if (item->isSelected()) {
                ingredients->addReagent(
                    static_cast<Reagent>(item->getId())
                );
            } else {
                ingredients->removeReagent(
                    static_cast<Reagent>(item->getId())
                );
            }
        }
        break;
    case U4_ENTER:
        eventHandler->setControllerDone();
        break;
    case U4_ESC:
        ingredients->revert();
        eventHandler->setControllerDone();
        break;
    default:
        return MenuController::keyPressed(key);
    } // switch
    return true;
} // ReagentsMenuController::keyPressed
