/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>
#include <cstdlib>

#include "rle.h"

/**
 * Decompress an RLE encoded file.
 */
long rleDecompressFile(std::FILE *in, const long in_len, unsigned char **out)
{
    /* input file should be longer than 0 bytes */
    if (in_len <= 0) {
        return -1;
    }
    /* load compressed file into memory */
    auto *in_data = static_cast<unsigned char *>(std::malloc(in_len));
    const long check = static_cast<long>(std::fread(in_data, 1, in_len, in));
    if (check != in_len) {
        std::perror("fread failed");
    }
    const long out_len = rleDecompressMemory(in_data, in_len, out);
    std::free(in_data);
    return out_len;
}

long rleDecompressMemory(
    const unsigned char *in, const long in_len, unsigned char **out
)
{
    /* input should be longer than 0 bytes */
    if (in_len <= 0) {
        return -1;
    }
    /* determine decompressed file size */
    const long out_len = rleGetDecompressedSize(in, in_len);
    if (out_len <= 0) {
        return -1;
    }
    /* decompress file from in_len to out_len */
    auto *out_data = static_cast<unsigned char *>(std::malloc(out_len));
    rleDecompress(in, in_len, out_data, out_len);
    *out = out_data;
    return out_len;
}


/**
 * Determine the uncompressed size of RLE compressed data.
 */
long rleGetDecompressedSize(const unsigned char *in_data, const long in_len)
{
    long len = 0;
    const unsigned char *p = in_data;
    while (p - in_data < in_len) {
        const unsigned char ch = *p++;
        if (ch == RLE_RUN_START) {
            const unsigned char count = *p++;
            p++;
            len += count;
        } else {
            len++;
        }
    }
    return len;
}


/**
 * Decompress a block of RLE encoded memory.
 */
long rleDecompress(
    const unsigned char *in_data,
    const long in_len,
    unsigned char *out_data,
    const long out_len
)
{
    const unsigned char *p = in_data;
    unsigned char *q = out_data;
    while (p - in_data < in_len) {
        const unsigned char ch = *p++;
        if (ch == RLE_RUN_START) {
            const unsigned char count = *p++;
            const unsigned char val = *p++;
            for (int i = 0; i < count; i++) {
                *q++ = val;
                if (q - out_data >= out_len) {
                    break;
                }
            }
        } else {
            *q++ = ch;
            if (q - out_data >= out_len) {
                break;
            }
        }
    }
    return q - out_data;
} // rleDecompress
