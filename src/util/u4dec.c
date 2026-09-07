/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzw/lzw.h"
#include "rle.h"
#include "util/pngconv.h"

static int isPowerOfTwo(int n);


/**
 * A simple command line interface to the U4 RLE and LZW decompressors.
 */
int main(const int argc, const char *argv[])
{
    FILE *infile;
    unsigned char *in_data, *out_data;
    long in_len, out_len;
    const char *alg, *in_file_name, *out_file_name;
    int width, height;
    int cond1, cond2;
    if (argc != 4 && argc != 6) {
        fprintf(
            stderr, "usage: u4dec rle|lzw|raw infile outfile [width height]\n"
        );
        exit(EXIT_FAILURE);
    }
    alg = argv[1];
    in_file_name = argv[2];
    out_file_name = argv[3];
    if (argc > 4) {
        width = (int)strtoul(argv[4], NULL, 0);
        height = (int)strtoul(argv[5], NULL, 0);
    } else {
        width = 320;
        height = 200;
    }
    printf("decoding %s image of size %dx%d\n", alg, width, height);
    infile = fopen(in_file_name, "rb");
    if (!infile) {
        perror(in_file_name);
        exit(EXIT_FAILURE);
    }
    if (fseek(infile, 0L, SEEK_END)) {
        perror(in_file_name);
        exit(EXIT_FAILURE);
    }
    in_len = ftell(infile);
    fseek(infile, 0L, SEEK_SET);
    in_data = (unsigned char *)malloc(in_len);
    if (!in_data) {
        perror("out of memory");
        exit(EXIT_FAILURE);
    }
    if ((long)fread(in_data, 1, in_len, infile) != in_len) {
        perror("fread failed");
    }
    fclose(infile);
    if (strcmp(alg, "lzw") == 0) {
        out_len = lzwGetDecompressedSize(in_data, in_len);
        out_data = (unsigned char *)malloc(out_len);
        if (!out_data) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        lzwDecompress(in_data, out_data, in_len);
    } else if (strcmp(alg, "rle") == 0) {
        out_len = rleGetDecompressedSize(in_data, in_len);
        cond1 = out_len*8 % (width*height) == 0;
        cond2 = isPowerOfTwo((int)(out_len*8 / (width*height)));
        if (!cond1 || !cond2) {
            printf("Invalid width or height.\n");
            exit(EXIT_FAILURE);
        }
        out_data = (unsigned char *)malloc(out_len);
        if (!out_data) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        rleDecompress(in_data, in_len, out_data, out_len);
    } else if (strcmp(alg, "raw") == 0) {
        out_len = in_len;
        out_data = in_data;
    } else {
        fprintf(stderr, "unknown algorithm %s\n", alg);
        exit(EXIT_FAILURE);
    }
    writePngFromEga(
        out_data,
        height,
        width,
        (int)(out_len * 8 / (height * width)),
        out_file_name
    );
    return EXIT_SUCCESS;
}

int isPowerOfTwo(const int n)
{
    if (n <= 0) {
        return 0;
    }
    int tmp = n;
    while (tmp % 2 == 0) {
        tmp = tmp >> 1;
    }
    if (tmp == 1) {
        return 1;
    }
    return 0;
}
