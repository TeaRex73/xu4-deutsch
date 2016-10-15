/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>

#include "io.h"

int writeInt(unsigned int i, std::FILE *f)
{
    if ((std::fputc(i & 0xff, f) == EOF)
        || (std::fputc((i >> 8) & 0xff, f) == EOF)
        || (std::fputc((i >> 16) & 0xff, f) == EOF)
        || (std::fputc((i >> 24) & 0xff, f) == EOF)) {
        return 0;
    }
    return 1;
}

int writeShort(unsigned short s, std::FILE *f)
{
    if ((std::fputc(s & 0xff, f) == EOF)
        || (std::fputc((s >> 8) & 0xff, f) == EOF)) {
        return 0;
    }
    return 1;
}

int writeChar(unsigned char c, std::FILE *f)
{
    if (std::fputc(c, f) == EOF) {
        return 0;
    }
    return 1;
}

int readInt(unsigned int *i, std::FILE *f)
{
    *i = std::fgetc(f);
    *i |= (std::fgetc(f) << 8);
    *i |= (std::fgetc(f) << 16);
    *i |= (std::fgetc(f) << 24);
    return 1;
}

int readShort(unsigned short *s, std::FILE *f)
{
    *s = std::fgetc(f);
    *s |= (std::fgetc(f) << 8);
    return 1;
}

int readChar(unsigned char *c, std::FILE *f)
{
    *c = std::fgetc(f);
    return 1;
}
