#ifndef U4DECODE_H
#define U4DECODE_H

#ifdef __cplusplus
#include <cstdio>
#define STD_FILE std::FILE
extern "C" {
#else
#include <stdio.h>
#define STDFILE FILE
#endif

long decompress_u4_file(STD_FILE *in, long filesize, unsigned char **out);
long getFilesize(STD_FILE *input_file);
unsigned char mightBeValidCompressedFile(STD_FILE *compressed_file);
long decompress_u4_memory(
    const unsigned char *in, long inlen, unsigned char **out
);

#undef STD_FILE

#ifdef __cplusplus
}
#endif

#endif
