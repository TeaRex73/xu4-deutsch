/*
 *  hash.c - hash functions for Ultima 4 LZW implementation
 *
 *  Copyright (C) 2002  Marc Winterrowd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../vc6.h" /* Fixes things if you're using VC6,
                       does nothing otherwise */

#include "hash.h"

int probe1(unsigned char root, int codeword)
{
    int newHashCode = ((root << 4) ^ codeword) & 0xfff;
    return newHashCode;
}

/* The secondary probe uses some assembler instructions that aren't
   easily translated to C. */
int probe2(unsigned char root, int codeword)
{
    /* registers[0] == AX, registers[1] == DX */
    long registers[2], temp;
    long carry, oldCarry;
    int i,j;
    /* the pre-mul part */
    registers[1] = 0;
    registers[0] = ((root << 1) + codeword) | 0x800;
    /* the mul part (simulated mul instruction) */
    /* DX:AX = AX * AX                          */
    temp = (registers[0] & 0xff) * (registers[0] & 0xff);
    temp += 2 * (registers[0] & 0xff) * (registers[0] >> 8) * 0x100;
    registers[1] = (temp >> 16) + (registers[0] >> 8) * (registers[0] >> 8);
    registers[0] = temp & 0xffff;
    /* if DX != 0, the mul instruction sets the carry flag */
    if (registers[1] == 00) {
        carry = 0;
    } else {
        carry = 1;
    }
    /* the rcl part */
    for (i = 0; i < 2; i++) { /* 2 rcl's */
        for (j = 0; j < 2; j++) { /* rotate through 2 registers */
            oldCarry = carry;
            carry = (registers[j] >> 15) & 1;
            registers[j] = (registers[j] << 1) | oldCarry;
            /* make sure register stays 16 bit */
            registers[j] = registers[j] & 0xffff;
        }
    }
    /* final touches */
    registers[0] = ((registers[0] >> 8) | (registers[1] << 8)) & 0xfff;
    return (int)registers[0];
}

int probe3(int hashCode)
{
    const long probeOffset = 0x1fd;   /* 0x1fd (decimal 509) is prime */
    long newHashCode = (hashCode + probeOffset) & 0xfff;
    return (int)newHashCode;
}
