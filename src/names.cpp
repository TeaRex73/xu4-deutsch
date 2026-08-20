/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "names.h"

#include "direction.h"
#include "savegame.h"

const char *getClassNameEnglish(const ClassType klass)
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

const char *getClassNameTranslated(const ClassType klass, const SexType sex)
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

const char *getReagentName(const Reagent reagent)
{
    static constexpr const char *reagentNames[] = {
        "Schwelasche",
        "Ginseng",
        "Knoblauch",
        "Spinnweben",
        "Blutmoos",
        "Schwarzperl",
        "Schatten",
        "Alraune"
    };
    if (reagent < REAGENT_MAX) {
        return reagentNames[reagent - REAGENT_ASH];
    }
    return "???";
}

const char *getVirtueName(const Virtue virtue)
{
    static constexpr const char *virtueNames[] = {
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
        return virtueNames[virtue - VIRTUE_HONESTY];
    }
    return "???";
}

const char *getBaseVirtueName(const int virtueMask)
{
    if (virtueMask == VIRTUE_TRUTH) {
        return "Wahrheit";
    }
    if (virtueMask == VIRTUE_LOVE) {
        return "Liebe";
    }
    if (virtueMask == VIRTUE_COURAGE) {
        return "Mut";
    }
    if (virtueMask == (VIRTUE_TRUTH | VIRTUE_LOVE)) {
        return "Wahrheit und Liebe";
    }
    if (virtueMask == (VIRTUE_LOVE | VIRTUE_COURAGE)) {
        return "Liebe und Mut";
    }
    if (virtueMask == (VIRTUE_COURAGE | VIRTUE_TRUTH)) {
        return "Mut und Wahrheit";
    }
    if (virtueMask == (VIRTUE_TRUTH | VIRTUE_LOVE | VIRTUE_COURAGE)) {
        return "Wahrheit, Liebe und Mut";
    }
    return "???";
}

int getBaseVirtues(const Virtue virtue)
{
    switch (virtue) {
    case VIRTUE_HONESTY:
        return VIRTUE_TRUTH;
    case VIRTUE_COMPASSION:
        return VIRTUE_LOVE;
    case VIRTUE_VALOR:
        return VIRTUE_COURAGE;
    case VIRTUE_JUSTICE:
        return VIRTUE_TRUTH | VIRTUE_LOVE;
    case VIRTUE_SACRIFICE:
        return VIRTUE_LOVE | VIRTUE_COURAGE;
    case VIRTUE_HONOR:
        return VIRTUE_COURAGE | VIRTUE_TRUTH;
    case VIRTUE_SPIRITUALITY:
        return VIRTUE_TRUTH | VIRTUE_LOVE | VIRTUE_COURAGE;
    case VIRTUE_HUMILITY:
    default:
        return 0;
    }
}

const char *getVirtueAdjective(const Virtue virtue)
{
    static constexpr const char *virtueAdjectives[] = {
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
        return virtueAdjectives[virtue - VIRTUE_HONESTY];
    }
    return "???";
}

const char *getStoneName(const Virtue virtue)
{
    static constexpr const char *virtueNames[] = {
        "Blau",
        "Gelb",
        "Rot",
        "Gr}n",
        "Orange",
        "Violett",
        "Wei~",
        "Schwarz"
    };
    if (virtue < VIRTUE_MAX) {
        return virtueNames[virtue - VIRTUE_HONESTY];
    }
    return "???";
}

const char *getItemName(const Item item)
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

const char *getDirectionName(const Direction dir)
{
    static constexpr const char *directionNames[] = {
        "West",
        "Nord",
        "Ost",
        "S}d"
    };
    if (dir >= DIR_WEST && dir <= DIR_SOUTH) {
        return directionNames[dir - DIR_WEST];
    }
    return "???";
}
