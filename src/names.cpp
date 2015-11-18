/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "names.h"

const char *getClassNameEnglish(ClassType klass)
{
    switch (klass) {
    case CLASS_MAGE:
        return "Mage";
    case CLASS_BARD:
        return "Bard";
    case CLASS_FIGHTER:
        return "Fighter";
    case CLASS_DRUID:
        return "Druid";
    case CLASS_TINKER:
        return "Tinker";
    case CLASS_PALADIN:
        return "Paladin";
    case CLASS_RANGER:
        return "Ranger";
    case CLASS_SHEPHERD:
        return "Shepherd";
    default:
        return "???";
    }
}

const char *getClassNameTranslated(ClassType klass, SexType sex)
{
    switch (klass) {
    case CLASS_MAGE:
        switch (sex) {
        case SEX_MALE:
            return "Magier";
        case SEX_FEMALE:
            return "Magierin";
        default:
            return "???";
        }
    case CLASS_BARD:
        switch (sex) {
        case SEX_MALE:
            return "Barde";
        case SEX_FEMALE:
            return "Bardin";
        default:
            return "???";
        }
    case CLASS_FIGHTER:
        switch (sex) {
        case SEX_MALE:
            return "K{mpfer";
        case SEX_FEMALE:
            return "K{mpferin";
        default:
            return "???";
        }
    case CLASS_DRUID:
        switch (sex) {
        case SEX_MALE:
            return "Druide";
        case SEX_FEMALE:
            return "Druidin";
        default:
            return "???";
        }
    case CLASS_TINKER:
        switch (sex) {
        case SEX_MALE:
            return "Zinker";
        case SEX_FEMALE:
            return "Zinkerin";
        default:
            return "???";
        }
    case CLASS_PALADIN:
        switch (sex) {
        case SEX_MALE:
            return "Paladin";
        case SEX_FEMALE:
            return "Paladinin";
        default:
            return "???";
        }
    case CLASS_RANGER:
        switch (sex) {
        case SEX_MALE:
            return "Waldl{ufer";
        case SEX_FEMALE:
            return "Waldl{uferin";
        default:
            return "???";
        }
    case CLASS_SHEPHERD:
        switch (sex) {
        case SEX_MALE:
            return "Hirte";
        case SEX_FEMALE:
            return "Hirtin";
        default:
            return "???";
        }
    default:
        return "???";
    } // switch
} // getClassNameTranslated

const char *getReagentName(Reagent reagent)
{
    static const char *const reagentNames[] = {
        "Schwelasche",
        "Ginseng",
        "Knoblauch",
        "Spinnweben",
        "Blutmoos",
        "Schwarzperl",
        "Schatten",
        "Alraune"
    };
    if (reagent < REAG_MAX) {
        return reagentNames[reagent - REAG_ASH];
    } else {
        return "???";
    }
}

const char *getVirtueName(Virtue virtue)
{
    static const char *const virtueNames[] = {
        "Ehrlichkeit",
        "Mitgef}hl",
        "Tapferkeit",
        "Gerechtigkeit",
        "Verzicht",
        "Ehre",
        "Spiritualit{t",
        "Demut"
    };
    if (virtue < 8) {
        return virtueNames[virtue - VIRT_HONESTY];
    } else {
        return "???";
    }
}

const char *getBaseVirtueName(int virtueMask)
{
    if (virtueMask == VIRT_TRUTH) {
        return "Wahrheit";
    } else if (virtueMask == VIRT_LOVE) {
        return "Liebe";
    } else if (virtueMask == VIRT_COURAGE) {
        return "Mut";
    } else if (virtueMask == (VIRT_TRUTH | VIRT_LOVE)) {
        return "Wahrheit und Liebe";
    } else if (virtueMask == (VIRT_LOVE | VIRT_COURAGE)) {
        return "Liebe und Mut";
    } else if (virtueMask == (VIRT_COURAGE | VIRT_TRUTH)) {
        return "Mut und Wahrheit";
    } else if (virtueMask == (VIRT_TRUTH | VIRT_LOVE | VIRT_COURAGE)) {
        return "Wahrheit, Liebe und Mut";
    } else {
        return "???";
    }
}

int getBaseVirtues(Virtue virtue)
{
    switch (virtue) {
    case VIRT_HONESTY:
        return VIRT_TRUTH;
    case VIRT_COMPASSION:
        return VIRT_LOVE;
    case VIRT_VALOR:
        return VIRT_COURAGE;
    case VIRT_JUSTICE:
        return VIRT_TRUTH | VIRT_LOVE;
    case VIRT_SACRIFICE:
        return VIRT_LOVE | VIRT_COURAGE;
    case VIRT_HONOR:
        return VIRT_COURAGE | VIRT_TRUTH;
    case VIRT_SPIRITUALITY:
        return VIRT_TRUTH | VIRT_LOVE | VIRT_COURAGE;
    case VIRT_HUMILITY:
        return 0;
    default:
        return 0;
    }
}

const char *getVirtueAdjective(Virtue virtue)
{
    static const char *const virtueAdjectives[] = {
        "ehrlich",
        "mitf}hlend",
        "tapfer",
        "gerecht",
        "verzichtend",
        "ehrenvoll",
        "spirituell",
        "dem}tig"
    };
    if (virtue < 8) {
        return virtueAdjectives[virtue - VIRT_HONESTY];
    } else {
        return "???";
    }
}

const char *getStoneName(Virtue virtue)
{
    static const char *const virtueNames[] = {
        "Blau",
        "Gelb",
        "Rot",
        "Gr}n",
        "Orange",
        "Violett",
        "Wei~",
        "Schwarz"
    };
    if (virtue < VIRT_MAX) {
        return virtueNames[virtue - VIRT_HONESTY];
    } else {
        return "???";
    }
}

const char *getItemName(Item item)
{
    switch (item) {
    case ITEM_SKULL:
        return "SCH[DEL";
    case ITEM_CANDLE:
        return "KERZE";
    case ITEM_BOOK:
        return "BUCH";
    case ITEM_BELL:
        return "GLOCKE";
    case ITEM_KEY_C:
        return "MUT";
    case ITEM_KEY_L:
        return "LIEBE";
    case ITEM_KEY_T:
        return "WAHRHEIT";
    case ITEM_HORN:
        return "HORN";
    case ITEM_WHEEL:
        return "STEUER";
    default:
        return "???";
    }
}

const char *getDirectionName(Direction dir)
{
    static const char *const directionNames[] = {
        "West",
        "Nord",
        "Ost",
        "S}d"
    };
    if ((dir >= DIR_WEST) && (dir <= DIR_SOUTH)) {
        return directionNames[dir - DIR_WEST];
    } else {
        return "???";
    }
}
