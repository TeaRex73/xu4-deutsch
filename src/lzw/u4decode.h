#ifndef U4DECODE_H
#define U4DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

long decompress_u4_file(FILE *in, long filesize, unsigned char **out);
long getFilesize(FILE *input_file);
unsigned char mightBeValidCompressedFile(FILE *compressed_file);
long decompress_u4_memory(
    const unsigned char *in, long inlen, unsigned char **out
);

#ifdef __cplusplus
}
#endif

#endif
