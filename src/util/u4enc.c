/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzw/hash.h"
#include "rle.h"
#include "util/pngconv.h"

#define MAX_DICT_CAPACITY 0xCCC
#define DICT_SIZE 0x1000

static int compressing = 1;
static int save = -1;
static int dict_size = 0;

static struct {
    int len;
    unsigned char *data;
    int occupied;
} lzw_dict[DICT_SIZE];

static void putc_12(int c, FILE *out);
static void flush_12(FILE *out);
static void init_dict(void);
static int get_code(unsigned char *str, int len);
static void add_code(unsigned char *str, int len);

/**
 * Outputs a 12 bit word.  If a half byte is left, it is saved until
 * the next time this is called.  flush_12 must be called to flush out
 * any saved data at the end of the stream.
 */
static void putc_12(const int c, FILE *out)
{
    if (save == -1) {
        putc(c >> 4, out);
        save = c & 0x0f;
    } else {
        putc((save << 4) | (c >> 8), out);
        putc(c & 0xff, out);
        save = -1;
    }
}


/**
 * Flushes the last half byte from putc_12.
 */
static void flush_12(FILE *out)
{
    if (save != -1) {
        putc(save << 4, out);
        save = -1;
    }
}


/**
 *  Initializes the LZW dictionary.
 */
static void init_dict(void)
{
    int i;
    dict_size = 0;
    for (i = 0; i < 256; i++) {
        lzw_dict[i].len = 1;
        lzw_dict[i].data = (unsigned char *)strdup("");
        if (!lzw_dict[i].data) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        lzw_dict[i].data[0] = i;
        lzw_dict[i].occupied = 1;
    }
    for (; i < DICT_SIZE; i++) {
        lzw_dict[i].len = 0;
        lzw_dict[i].data = NULL;
        lzw_dict[i].occupied = 0;
    }
}


/**
 * Gets the 12-bit LZW code for a given string.  -1 is returned if not
 * in the dictionary.
 */
static int get_code(unsigned char *str, const int len)
{
    int prefix_code;
    int hash_code;
    if (len == 1) {
        return str[0];
    }
    prefix_code = get_code(str, len - 1);
    hash_code = probe1(str[len - 1], prefix_code);
    if (lzw_dict[hash_code].occupied
        && lzw_dict[hash_code].len == len
        && memcmp(lzw_dict[hash_code].data, str, len) == 0) {
        return hash_code;
    }
    hash_code = probe2(str[len - 1], prefix_code);
    if (lzw_dict[hash_code].occupied
        && lzw_dict[hash_code].len == len
        && memcmp(lzw_dict[hash_code].data, str, len) == 0) {
        return hash_code;
    }
    do {
        hash_code = probe3(hash_code);
        if (lzw_dict[hash_code].occupied
            && lzw_dict[hash_code].len == len
            && memcmp(lzw_dict[hash_code].data, str, len) == 0) {
            return hash_code;
        }
    } while (lzw_dict[hash_code].occupied);
    return -1;
}


/**
 * Add a new word to the LZW dictionary.
 */
static void add_code(unsigned char *str, int len)
{
    int hashcode;
    if (!compressing) {
        return;
    }
    hashcode = probe1(str[len - 1], get_code(str, len - 1));
    if (lzw_dict[hashcode].occupied) {
        hashcode = probe2(str[len - 1], get_code(str, len - 1));
    }
    if (lzw_dict[hashcode].occupied) {
        do {
            hashcode = probe3(hashcode);
        } while (lzw_dict[hashcode].occupied);
    }
    lzw_dict[hashcode].len = len;
    lzw_dict[hashcode].data = (unsigned char *) malloc(len);
    if (!lzw_dict[hashcode].data) {
        perror("out of memory");
        exit(EXIT_FAILURE);
    }
    memcpy(lzw_dict[hashcode].data, str, len);
    lzw_dict[hashcode].occupied = 1;
    dict_size++;
}


/**
 * LZW encode a file.
 */
int main(const int argc, char *argv[])
{
    FILE *out;
    const char *alg, *in_file_name, *out_file_name;
    int bits;
    int height = 0, width = 0;
    int data_length, c;
    unsigned char *data;
    const unsigned char *p;
    if (argc != 4) {
        fprintf(stderr, "usage: u4enc rle|lzw|raw infile outfile\n");
        exit(EXIT_FAILURE);
    }
    alg = argv[1];
    in_file_name = argv[2];
    out_file_name = argv[3];
    out = fopen(out_file_name, "wb");
    if (!out) {
        perror(out_file_name);
        exit(EXIT_FAILURE);
    }
    readEgaFromPng(&data, &height, &width, &bits, in_file_name);
    data_length = width * height * bits / 8;
    fprintf(stderr, "image is %dx%d (%d bits)\n", width, height, bits);
    if (strcmp(alg, "lzw") == 0) {
        unsigned char str[4096];
        int idx;
        init_dict();
        p = data;
        idx = 0;
        c = *p++;
        str[idx++] = c;
        while (1) {
            c = *p++;
            if (p > data + data_length) {
                break;
            }
            str[idx++] = c;
            if (get_code(str, idx) == -1) {
                const int code = get_code(str, idx-1);
                putc_12(code, out);
                if (idx >= 4095) {
                    fprintf(stderr, "overflow in lzw encoding\n");
                    exit(EXIT_FAILURE);
                }
                add_code(str, idx);
                if (dict_size > MAX_DICT_CAPACITY) {
                    init_dict();
                    putc_12(c, out);
                    c = *p++;
                    if (p > data + data_length) {
                        break;
                    }
                }
                idx = 0;
                str[idx++] = c;
            }
        }
        if (idx != 0) {
            putc_12(get_code(str, idx), out);
        }
        flush_12(out);
    } else if (strcmp(alg, "rle") == 0) {
        int count, threshold, val, i;
        /*
         * The original, 4-bit graphics only start a run if the count is 5
         * or more; but the upgrade uses a run wherever it doesn't expand
         * the file (i.e. at a count of 3).  This value is adjusted to
         * re-encode the files exactly as they were, if they weren't
         * changed.  A threshold of 3 or 4 gives optimal compression; 5 is
         * slightly worse and it is not clear why the original files used
         * it.
         */
        if (bits == 4) {
            threshold = 5;
        } else {
            threshold = 3;
        }
        p = data;
        count = 0;
        val = -1;
        while (p < data + data_length) {
            c = *p++;
            switch (c) {
            case 0x01:
                c = 0x07;
                break;
            case 0x10:
                c = 0x70;
                break;
            case 0x11:
                c = 0x77;
                break;
            default:
                break;
            }
            if (c == val && count < 255) {
                count++;
            } else {
                if (count >= threshold || val == RLE_RUN_START) {
                    putc(RLE_RUN_START, out);
                    putc(count, out);
                    putc(val, out);
                } else {
                    for (i = 0; i < count; i++) {
                        putc(val, out);
                    }
                }
                val = c;
                count = 1;
            }
        }
        if (count >= threshold || val == RLE_RUN_START) {
            putc(RLE_RUN_START, out);
            putc(count, out);
            putc(val, out);
        } else {
            for (i = 0; i < count; i++) {
                putc(val, out);
            }
        }
    } else if (strcmp(alg, "raw") == 0) {
        fwrite(data, data_length, 1, out);
    } else {
        fprintf(stderr, "unknown algorithm %s\n", alg);
        exit(EXIT_FAILURE);
    }
    fclose(out);
    return EXIT_SUCCESS;
}
