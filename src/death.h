/*
 * $Id$
 */

#ifndef DEATH_H
#define DEATH_H

#include <atomic>

extern std::atomic_bool deathSequenceRunning;
void deathStart(int delay);

#endif // DEATH_H
