/*
 * $Id$
 */

#ifndef SPELL_H
#define SPELL_H

#include <string>

#include "context.h"
#include "location.h"
#include "savegame.h"
#include "sound.h"


enum SpellCastError {
    CAST_ERROR_NO_ERROR, /* success */
    CAST_ERROR_NO_MIXTURE, /* no mixture available */
    CAST_ERROR_MP_TOO_LOW, /* caster doesn't have enough mp */
    CAST_ERROR_FAILED, /* the spell failed */
    CAST_ERROR_WRONG_CONTEXT, /* generic 'wrong-context' error
                             (generally finds the correct
                             context error message on its own) */
    CAST_ERROR_COMBAT_ONLY, /* spell must be cast in combat */
    CAST_ERROR_DUNGEON_ONLY, /* spell must be cast in dungeons */
    CAST_ERROR_WORLD_MAP_ONLY /* spell must be cast on the world map */
};

/* Field types for the Energy field spell */
enum EnergyFieldType {
    ENERGY_FIELD_NONE,
    ENERGY_FIELD_FIRE,
    ENERGY_FIELD_LIGHTNING,
    ENERGY_FIELD_POISON,
    ENERGY_FIELD_SLEEP
};


/**
 * The ingredients for a spell mixture.
 */
class Ingredients {
public:
    Ingredients();
    bool addReagent(Reagent reagent);
    bool removeReagent(Reagent reagent);
    int getReagent(Reagent reagent) const;
    void revert();
    bool checkMultiple(int batches) const;
    void multiply(int batches);

private:
    unsigned short reagents[REAGENT_MAX];
};

struct Spell {
    typedef enum {
        PARAM_NONE, /* none */
        PARAM_PLAYER, /* number of a player required */
        PARAM_DIR, /* direction required */
        PARAM_TYPE_DIR, /* field type and direction required (energy field) */
        PARAM_PHASE, /* phase required (gate) */
        PARAM_FROM_DIR /* direction from required (winds) */
    } Param;

    typedef enum {
        SFX_NONE, /* none */
        SFX_INVERT, /* invert the screen (moongates, most normal spells) */
        SFX_TREMOR /* tremor spell */
    } SpecialEffects;

    const char *name;
    int components;
    LocationContext context;
    TransportContext transportContext;
    bool (*spellFunc)(int);
    Param paramType;
    int mp;
};

typedef void (*SpellEffectCallback)(unsigned int spell, int player, Sound sound);
extern SpellEffectCallback spellEffectCallback;
void spellSetEffectCallback(SpellEffectCallback callback);
const char *spellGetName(unsigned int spell);
int spellGetRequiredMP(unsigned int spell);
LocationContext spellGetContext(unsigned int spell);
TransportContext spellGetTransportContext(unsigned int spell);
std::string spellGetErrorMessage(unsigned int spell, SpellCastError error);
bool spellMix(unsigned int spell, const Ingredients *ingredients);
Spell::Param spellGetParamType(unsigned int spell);
SpellCastError spellCheckPrerequisites(unsigned int spell, int character);
bool spellCast(
    unsigned int spell,
    int character,
    int param,
    SpellCastError *error,
    bool spellEffect
);
const Spell *getSpell(int i);

#endif // SPELL_H
