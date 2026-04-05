#ifndef U4DECODE_H
#define U4DECODE_H

#ifdef __cplusplus
#include <cstdio>
#define STDFILE std::FILE
extern "C" {
#else
#include <stdio.h>
#define STDFILE FILE
#endif

long decompress_u4_file(STDFILE *in, long filesize, unsigned char **out);
long getFilesize(STDFILE *input_file);
unsigned char mightBeValidCompressedFile(STDFILE *compressed_file);
long decompress_u4_memory(
    const unsigned char *in, long inlen, unsigned char **out
);

#undef STDFILE

#ifdef __cplusplus
}
#endif

#endif
