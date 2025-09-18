/*
 * $Id$
 */

#ifndef U6DECODE_H
#define U6DECODE_H

#include <cstdio>

namespace U6Decode
{
class Stack;
class Dict;

unsigned char read1(std::FILE *f);
long read4(std::FILE *f);
long get_filesize(std::FILE *input_file);
bool is_valid_lzw_file(std::FILE *input_file);
long get_uncompressed_size(std::FILE *input_file);
int get_next_codeword(
    long &bits_read, const unsigned char *source, int codeword_size
);
void output_root(
    unsigned char root, unsigned char *destination, long &position
);
void get_string(Stack &stack, int codeword);
int lzw_decompress(
    const unsigned char *source,
    long source_length,
    unsigned char *destination,
    long destination_length
);
int lzw_decompress(std::FILE *input_file, std::FILE *output_file);
};

#endif
