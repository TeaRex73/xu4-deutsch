#ifndef U4DECODE_H
#define U4DECODE_H

long decompress_u4_file(std::FILE *in, long filesize, unsigned char **out);
long getFilesize(std::FILE *input_file);
unsigned char mightBeValidCompressedFile(std::FILE *compressed_file);
long decompress_u4_memory(
    const unsigned char *in, long inlen, unsigned char **out
);

#endif
