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
long rleDecompressFile(std::FILE *in, long inlen, unsigned char **out)
{
    long check;
    unsigned char *indata;
    long outlen;
    /* input file should be longer than 0 bytes */
    if (inlen <= 0) {
        return -1;
    }
    /* load compressed file into memory */
    indata = static_cast<unsigned char *>(std::malloc(inlen));
    check = std::fread(indata, 1, inlen, in);
    if (check != inlen) {
        std::perror("fread failed");
    }
    outlen = rleDecompressMemory(indata, inlen, out);
    std::free(indata);
    return outlen;
}

long rleDecompressMemory(unsigned char *in, long inlen, unsigned char **out)
{
    unsigned char *outdata;
    long outlen;
    /* input should be longer than 0 bytes */
    if (inlen <= 0) {
        return -1;
    }
    /* determine decompressed file size */
    outlen = rleGetDecompressedSize(in, inlen);
    if (outlen <= 0) {
        return -1;
    }
    /* decompress file from inlen to outlen */
    outdata = static_cast<unsigned char *>(std::malloc(outlen));
    rleDecompress(in, inlen, outdata, outlen);
    *out = outdata;
    return outlen;
}


/**
 * Determine the uncompressed size of RLE compressed data.
 */
long rleGetDecompressedSize(unsigned char *indata, long inlen)
{
    unsigned char *p;
    long len = 0;
    p = indata;
    while ((p - indata) < inlen) {
        unsigned char ch;
        ch = *p++;
        if (ch == RLE_RUNSTART) {
            unsigned char count;
            count = *p++;
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
    unsigned char *indata,
    long inlen,
    unsigned char *outdata,
    long outlen
)
{
    int i;
    unsigned char *p, *q;
    p = indata;
    q = outdata;
    while ((p - indata) < inlen) {
        unsigned char ch;
        ch = *p++;
        if (ch == RLE_RUNSTART) {
            unsigned char count, val;
            count = *p++;
            val = *p++;
            for (i = 0; i < count; i++) {
                *q++ = val;
                if ((q - outdata) >= outlen) {
                    break;
                }
            }
        } else {
            *q++ = ch;
            if ((q - outdata) >= outlen) {
                break;
            }
        }
    }
    return q - outdata;
} // rleDecompress
