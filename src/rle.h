/*
 * $Id$
 */

#ifndef RLE_H
#define RLE_H

#ifdef __cplusplus
extern "C" {
#include <cstdio>
#define STD_FILE std::FILE
#else
#include <stdio.h>
#define STD_FILE FILE
#endif

#define RLE_RUN_START 02

long rleDecompressFile(STD_FILE *in, long in_len, unsigned char **out);
long rleDecompressMemory(
    const unsigned char *in, long in_len, unsigned char **out
);
long rleGetDecompressedSize(const unsigned char *in_data, long in_len);
long rleDecompress(
    const unsigned char *in_data,
    long in_len,
    unsigned char *out_data,
    long out_len
);

#undef STD_FILE
#ifdef __cplusplus
}
#endif

#endif
