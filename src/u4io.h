/*
 * $Id$
 */

#ifndef IO_H
#define IO_H


/*
 * These are endian-independent routines for reading and writing
 * 4-byte (int), 2-byte (short), and 1-byte (char) values to and from
 * the ultima 4 data files.  If sizeof(int) != 4, all bets are off.
 */
bool writeInt(unsigned int i, std::FILE *f);
bool writeShort(unsigned short s, std::FILE *f);
bool writeChar(unsigned char c, std::FILE *f);
bool readInt(unsigned int *i, std::FILE *f);
bool readShort(unsigned short *s, std::FILE *f);
bool readChar(unsigned char *c, std::FILE *f);

#endif
