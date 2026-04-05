/*
 * $Id$
 */

#ifndef NAMES_H
#define NAMES_H

#include "direction.h"

enum ClassType : unsigned char;
enum Item : unsigned short;
enum Reagent : unsigned char;
enum SexType : unsigned char;
enum Virtue : unsigned char;

/*
 * These routines convert the various enumerations for classes, reagents,
 * etc. into the textual representations used in the game.
 */
const char *getClassNameEnglish(ClassType klass);
const char *getClassNameTranslated(ClassType klass, SexType sex);
const char *getReagentName(Reagent reagent);
const char *getVirtueName(Virtue virtue);
const char *getBaseVirtueName(int virtueMask);
int getBaseVirtues(Virtue virtue);
const char *getVirtueAdjective(Virtue virtue);
const char *getStoneName(Virtue virtue);
const char *getItemName(Item item);
const char *getDirectionName(Direction dir);

#endif
