/*
 *  lzw.c - LZW decompression from memory to memory
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

/*
 * A few files from Ultima 4 (PC version) have been compressed with the LZW
 * algorithm. There are two things that make the U4 implementation of the LZW
 * decoding algorithm special:
 * 1) It uses a fixed codeword length of 12 bits.
 * The advantages over variable-length codewords are faster decompression and
 * simpler code.
 * 2) The dictionary is implemented as a hash table.
 * While the dictionary is supposed to be implemented as a hash table in the
 * LZW *en*coder (to speed up string searches), there is no reason not to
 * implement it as a simple array in the decoder. But since U4 uses a hash
 * table in the decoder, this C version must do the same (or it won't be able
 * to decode the U4 files).
 * An article on LZW data (de)compression can be found here:
 * http://dogma.net/markn/articles/lzw/lzw.htm
 */
#include "../vc6.h" /* Fixes things if you're using VC6,
                       does nothing otherwise */

#include "lzw.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*WRITE_DECOMP)(
    unsigned char root, unsigned char *destination, long *position
);

typedef struct _lzwDictionaryEntry {
    unsigned char root;
    int codeword;
    unsigned char occupied;
} lzwDictionaryEntry;

long generalizedDecompress(
    WRITE_DECOMP outFunc,
    const unsigned char *compressedMem,
    unsigned char *decompressedMem,
    long compressedSize
);
int getNextCodeword(long *bitsRead, const unsigned char *compressedMem);
void discardRoot(
    unsigned char root, unsigned char *destination, long *position
);
void outputRoot(
    unsigned char root, unsigned char *destination, long *position
);

void getString(
    int codeword,
    lzwDictionaryEntry *dictionary,
    unsigned char *stack,
    int *elementsInStack
);
int getNewHashCode(
    unsigned char root, int codeword, lzwDictionaryEntry *dictionary
);
unsigned char hashPosFound(
    int hashCode,
    unsigned char root,
    int codeword,
    lzwDictionaryEntry *dictionary
);


/*
 * This function returns the decompressed size of a block of compressed data.
 * It doesn't decompress the data. Use this function if you want to decompress
 * a block of data, but don't know the decompressed size in advance.
 *
 * There is some error checking to detect if the compressed data is corrupt,
 * but it's only rudimentary.
 * Returns:
 * No errors: (long) decompressed size
 * Error: (long) -1
 */
long lzwGetDecompressedSize(
    const unsigned char *compressedMem,
    long compressedSize
)
{
    return generalizedDecompress(
        &discardRoot, compressedMem, NULL, compressedSize
    );
}


/*
 * Decompresses a block of compressed data from memory to memory.
 * Use this function if you already know the decompressed size.
 *
 * This function assumes that *decompressed_mem is already allocated, and that
 * the decompressed data will fit into *decompressed_mem.
 * There is some error checking to detect if the compressed data is corrupt,
 * but it's only rudimentary.
 * Returns:
 * No errors: (long) decompressed size
 * Error: (long) -1
 */
long lzwDecompress(
    const unsigned char *compressedMem,
    unsigned char *decompressedMem,
    long compressedSize
)
{
    return generalizedDecompress(
        &outputRoot, compressedMem, decompressedMem, compressedSize
    );
}


/* ------------------------------------------------------------------------
   Functions used only inside lzw.c
   ------------------------------------------------------------------------ */


/*
 * This function does the actual decompression work.
 * Parameters:
 * perform_decompression: FALSE ==> return decompressed size, but discard
 * decompressed data
 * compressed_mem: compressed data
 * decompressed_mem: this is where the compressed data will be decompressed to
 * compressed_size: size of the compressed data (in bytes)
 */
long generalizedDecompress(
    WRITE_DECOMP outFunc,
    const unsigned char *compressedMem,
    unsigned char *decompressedMem,
    long compressedSize
)
{
    int i;
    /* re-initialize the dictionary when there are more than 0xccc entries */
    const int maxDictEntries = 0xccc;
    const int lzwStackSize = 0x8000;
    const int lzwDictionarySize = 0x1000;
    long bitsRead = 0;
    long bytesWritten = 0;
    /* initialize the dictionary and the stack */
    lzwDictionaryEntry *lzwDictionary = (lzwDictionaryEntry *) malloc(
        sizeof(lzwDictionaryEntry) * lzwDictionarySize
    );
    if (!lzwDictionary) {
        perror("out of memory");
        exit(EXIT_FAILURE);
    }
    unsigned char *lzwStack =
        (unsigned char *) malloc(sizeof(unsigned char) * lzwStackSize);
    if (!lzwStack) {
        perror("out of memory");
        exit(EXIT_FAILURE);
    }
    int elementsInStack = 0;
    /* clear the dictionary */
    memset(lzwDictionary, 0, sizeof(lzwDictionaryEntry) * lzwDictionarySize);
    for (i = 0; i < 0x100; i++) {
        lzwDictionary[i].occupied = 1;
    }
    if (bitsRead + 12 <= compressedSize * 8) {
        int old_code;
        unsigned char character;
        /* unknownCodeword: is the current codeword in the dictionary? */
        unsigned char unknownCodeword;
        int codewordsInDictionary = 0;
        /* read OLD_CODE */
        old_code = getNextCodeword(&bitsRead, compressedMem);
        /* CHARACTER = OLD_CODE */
        character = (unsigned char)old_code;
        /* output OLD_CODE */
        outFunc(character, decompressedMem, &bytesWritten);
        /* WHILE there are still input characters DO */
        while (bitsRead + 12 <= compressedSize * 8) {
            int new_code;
            /* newpos: position in the dictionary where new codeword
               was added */
            /* must be equal to current codeword (if it isn't, the compressed
               data must be corrupt). */
            int newpos;
            /* read NEW_CODE */
            new_code = getNextCodeword(&bitsRead, compressedMem);
            /* is the codeword in the dictionary? */
            if (lzwDictionary[new_code].occupied) {
                /* codeword is present in the dictionary */
                /* it must either be a root or a non-root that has already
                   been added to the dicionary */
                unknownCodeword = 0;
                /* STRING = get translation of NEW_CODE */
                getString(new_code,lzwDictionary,lzwStack,&elementsInStack);
            } else {
                /* codeword is yet to be defined */
                unknownCodeword = 1;
                /* STRING = get translation of OLD_CODE */
                /* STRING = STRING+CHARACTER            */
                /* push character on the stack */
                lzwStack[elementsInStack] = character;
                elementsInStack++;
                getString(old_code,lzwDictionary,lzwStack,&elementsInStack);
            }
            /* CHARACTER = first character in STRING */
            /* element at top of stack */
            character = lzwStack[elementsInStack-1];
            /* output STRING */
            while (elementsInStack > 0) {
                outFunc(
                    lzwStack[elementsInStack-1],
                    decompressedMem,
                    &bytesWritten
                );
                elementsInStack--;
            }
            /* add OLD_CODE + CHARACTER to the translation table */
            newpos = getNewHashCode(character,old_code,lzwDictionary);
            lzwDictionary[newpos].root = character;
            lzwDictionary[newpos].codeword = old_code;
            lzwDictionary[newpos].occupied = 1;
            codewordsInDictionary++;
            /* check for errors */
            if (unknownCodeword && (newpos != new_code)) {
                /* clean up */
                free(lzwStack);
                free(lzwDictionary);
                return -1;
            }
            if (codewordsInDictionary > maxDictEntries) {
                /* wipe dictionary */
                codewordsInDictionary = 0;
                memset(
                    lzwDictionary,
                    0,
                    sizeof(lzwDictionaryEntry) * lzwDictionarySize
                );
                for (i = 0; i < 0x100; i++) {
                    lzwDictionary[i].occupied = 1;
                }
                if (bitsRead + 12 <= compressedSize * 8) {
                    new_code = getNextCodeword(&bitsRead, compressedMem);
                    character = (unsigned char)new_code;
                    outFunc(character, decompressedMem, &bytesWritten);
                } else {
                    /* clean up */
                    free(lzwStack);
                    free(lzwDictionary);
                    return bytesWritten;
                }
            }
            /* OLD_CODE = NEW_CODE */
            old_code = new_code;
        }
    }
    /* clean up */
    free(lzwStack);
    free(lzwDictionary);
    return bytesWritten;
}


/* read the next 12-bit codeword from the compressed data */
int getNextCodeword(long *bitsRead, const unsigned char *compressedMem)
{
    int codeword =
        (compressedMem[(*bitsRead)/8] << 8) + compressedMem[(*bitsRead)/8+1];
    codeword = codeword >> (4-((*bitsRead)%8));
    codeword = codeword & 0xfff;
    (*bitsRead) += 12;
    return codeword;
}


/* increment position pointer, but do not write root to memory */
void discardRoot(
    unsigned char root, unsigned char *destination, long *position
)
{
    (void) root;
    (void) destination;
    (*position)++;
}


/* output a root to memory */
void outputRoot(unsigned char root, unsigned char *destination, long *position)
{
    destination[*position] = root;
    (*position)++;
}


/* -----------------------------------------------------------------------
   Dictionary-related functions
   ------------------------------------------------------------------------ */


/* pushes the string associated with codeword onto the stack */
void getString(
    int codeword,
    lzwDictionaryEntry *dictionary,
    unsigned char *stack,
    int *elementsInStack
)
{
    int currentCodeword = codeword;
    while (currentCodeword > 0xff) {
        unsigned char root;
        root = dictionary[currentCodeword].root;
        currentCodeword = dictionary[currentCodeword].codeword;
        stack[*elementsInStack] = root;
        (*elementsInStack)++;
    }
    /* push the root at the leaf */
    stack[*elementsInStack] = (unsigned char)currentCodeword;
    (*elementsInStack)++;
}

int getNewHashCode(
    unsigned char root, int codeword, lzwDictionaryEntry *dictionary
)
{
    int hashCode;
    /* probe 1 */
    hashCode = probe1(root, codeword);
    if (hashPosFound(hashCode, root, codeword, dictionary))
        return hashCode;
    /* probe 2 */
    hashCode = probe2(root, codeword);
    if (hashPosFound(hashCode, root, codeword, dictionary))
        return hashCode;
    /* probe 3 */
    do {
        hashCode = probe3(hashCode);
    } while (! hashPosFound(hashCode, root, codeword, dictionary));
    return hashCode;
}

unsigned char hashPosFound(
    int hashCode,
    unsigned char root,
    int codeword,
    lzwDictionaryEntry *dictionary
)
{
    if (hashCode > 0xff) { /* hash codes must not be roots */
         if (dictionary[hashCode].occupied) {
             unsigned char c2, c3;
             /* hash table position is occupied */
            /* is our (root,codeword) pair already in the hash table? */
            c2 = dictionary[hashCode].root == root;
            c3 = dictionary[hashCode].codeword == codeword;
            return c2 && c3;
        } else {
            /* hash table position is free */
            return 1;
        }
    } else {
        return 0;
    }
}
