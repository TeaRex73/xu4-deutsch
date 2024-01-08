/*
 * $Id$
 */

#ifndef RLE_H
#define RLE_H

#ifdef __cplusplus
extern "C" {
#define STDFILE FILE
#else
#define STDFILE FILE
#endif

#define RLE_RUNSTART 02

long rleDecompressFile(STDFILE *in, long inlen, unsigned char **out);
long rleDecompressMemory(
    const unsigned char *in, long inlen, unsigned char **out
);
long rleGetDecompressedSize(const unsigned char *indata, long inlen);
long rleDecompress(
    const unsigned char *indata,
    long inlen,
    unsigned char *outdata,
    long outlen
);

#undef STDFILE
#ifdef __cplusplus
}
#endif

#endif
