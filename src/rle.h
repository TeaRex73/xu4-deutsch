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
long rleDecompressMemory(unsigned char *in, long inlen, unsigned char **out);
long rleGetDecompressedSize(unsigned char *indata, long inlen);
long rleDecompress(
    unsigned char *indata,
    long inlen,
    unsigned char *outdata,
    long outlen
);

#undef STDFILE
#ifdef __cplusplus
}
#endif

#endif
