/*
 * context.cpp
 * Copyright (C) 2012 Daniel Santos
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "context.h"

Context::Context()
    :party(nullptr),
     saveGame(nullptr),
     location(nullptr),
     line(0),
     col(0),
     stats(nullptr),
     moonPhase(0),
     windDirection(0),
     windLock(false),
     aura(nullptr),
     horseSpeed(0),
     opacity(1),
     transportContext(),
     lastCommandTime(0),
     willPassTurn(false),
     lastShip(nullptr)
{
}

Context::~Context()
{
    delete saveGame;
    delete stats;
    delete aura;
    delete party;
    delete location;
}
