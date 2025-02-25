/*
 *  u6decode.cpp - Command-line decompression utility for Ultima 4 (PC version)
 *
 *  Copyright (C) 2005  Marc Winterrowd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// Ultima 6 dempression utility
// Last updated on 18-February-2005

/*
 * In xu4, this is actually used to decompress Ultima 5 files (despite
 * the name), which happen to use the same compression algorithm as
 * ultima 6.
 */

#include "../vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>
#include <cstdlib>

#include "u6decode.h"
#include "u6stack.h"
#include "u6dict.h"

using namespace U6Decode;

Dict dict;

unsigned char U6Decode::read1(std::FILE *f)
{
    return std::fgetc(f);
}

long U6Decode::read4(std::FILE *f)
{
    unsigned char b0, b1, b2, b3;
    b0 = std::fgetc(f);
    b1 = std::fgetc(f);
    b2 = std::fgetc(f);
    b3 = std::fgetc(f);
    return b0 + (b1 << 8) + (b2 << 16) + (b3 << 24);
}

long U6Decode::get_filesize(std::FILE *input_file)
{
    long file_length;
    std::fseek(input_file, 0, SEEK_END);
    file_length = std::ftell(input_file);
    std::fseek(input_file, 0, SEEK_SET);
    return file_length;
}

// this function only checks a few *necessary* conditions
// returns "false" if the file doesn't satisfy these conditions
// return "true" otherwise
bool U6Decode::is_valid_lzw_file(std::FILE *input_file)
{
    // file must contain 4-byte size header and space for 9-bit value 0x100
    if (get_filesize(input_file) < 6) {
        return false;
    }
    // the last byte of the size header must be 0
    // (U6's files aren't *that* big)
    std::fseek(input_file, 3, SEEK_SET);
    unsigned char byte3 = std::fgetc(input_file);
    std::fseek(input_file, 0, SEEK_SET);
    if (byte3 != 0) {
        return false;
    }
    // the 9 bits after the size header must be 0x100
    std::fseek(input_file, 4, SEEK_SET);
    unsigned char b0 = std::fgetc(input_file);
    unsigned char b1 = std::fgetc(input_file);
    std::fseek(input_file, 0, SEEK_SET);
    if ((b0 != 0) || ((b1 & 1) != 1)) {
        return false;
    }
    return true;
}

long U6Decode::get_uncompressed_size(std::FILE *input_file)
{
    if (is_valid_lzw_file(input_file)) {
        std::fseek(input_file, 0, SEEK_SET);
        long uncompressed_file_length = read4(input_file);
        std::fseek(input_file, 0, SEEK_SET);
        return uncompressed_file_length;
    } else {
        return -1;
    }
}

// ----------------------------------------------
// Read the next code word from the source buffer
// ----------------------------------------------
int U6Decode::get_next_codeword(
    long &bits_read, const unsigned char *source, int codeword_size
)
{
    unsigned char b0, b1, b2;
    int codeword;
    b0 = source[bits_read/8];
    b1 = source[bits_read/8+1];
    b2 = source[bits_read/8+2];
    codeword = ((b2 << 16) + (b1 << 8) + b0);
    codeword = codeword >> (bits_read % 8);
    switch (codeword_size) {
    case 0x9:
        codeword = codeword & 0x1ff;
        break;
    case 0xa:
        codeword = codeword & 0x3ff;
        break;
    case 0xb:
        codeword = codeword & 0x7ff;
        break;
    case 0xc:
        codeword = codeword & 0xfff;
        break;
    default:
        printf("Error: weird codeword size!\n");
        break;
    }
    bits_read += codeword_size;
    return codeword;
}

void U6Decode::output_root(
    unsigned char root, unsigned char *destination, long &position
)
{
    destination[position] = root;
    position++;
}

void U6Decode::get_string(Stack &stack, int codeword)
{
    int current_codeword = codeword;
    while (current_codeword > 0xff) {
        unsigned char root = dict.get_root(current_codeword);
        current_codeword = dict.get_codeword(current_codeword);
        stack.push(root);
    }
    // push the root at the leaf
    stack.push(static_cast<unsigned char>(current_codeword));
}


// ---------------------------------------------------------------------------
// LZW-decompress from buffer to buffer.
// The parameters "source_length" and "destination_length" are currently
// unused. They might be used to prevent reading/writing outside the buffers.
// ---------------------------------------------------------------------------
int U6Decode::lzw_decompress(
    const unsigned char *source,
    long /* source_length */,
    unsigned char *destination,
    long /* destination_length */
)
{
    const int max_codeword_length = 12;
    bool end_marker_reached = false;
    int codeword_size = 9;
    long bits_read = 0;
    int next_free_codeword = 0x102;
    int dictionary_size = 0x200;
    long bytes_written = 0;
    int pW = 0;
    unsigned char C;
    while (!end_marker_reached) {
        int cW = get_next_codeword(bits_read, source, codeword_size);
        switch (cW) {
        case 0x100:
            // re-init the dictionary
            codeword_size = 9;
            next_free_codeword = 0x102;
            dictionary_size = 0x200;
            dict.init();
            cW = get_next_codeword(bits_read, source, codeword_size);
            output_root(
                static_cast<unsigned char>(cW), destination, bytes_written
            );
            break;
        case 0x101:
            // end of compressed file has been reached
            end_marker_reached = true;
            break;
        default:
            // (cW <> 0x100) && (cW <> 0x101)
            // codeword is already in the dictionary
            if (cW < next_free_codeword) {
                Stack stack;
                // create the string associated with cW (on the stack)
                get_string(stack, cW);
                C = stack.gettop();
                // output the string represented by cW
                while (!stack.is_empty()) {
                    output_root(stack.pop(), destination, bytes_written);
                }
                // add pW+C to the dictionary
                dict.add(C, pW);
                next_free_codeword++;
                if (next_free_codeword >= dictionary_size) {
                    if (codeword_size < max_codeword_length) {
                        codeword_size += 1;
                        dictionary_size *= 2;
                    }
                }
            } else { // codeword is not yet defined
                Stack stack;
                // create the string associated with pW (on the stack)
                get_string(stack, pW);
                C = stack.gettop();
                // output the string represented by pW
                while (!stack.is_empty()) {
                    output_root(stack.pop(), destination, bytes_written);
                }
                // output the char C
                output_root(C, destination, bytes_written);
                // the new dictionary entry must correspond to cW
                // if it doesn't, something is wrong with the lzw-compressed
                // data.
                if (cW != next_free_codeword) {
                    printf("cW != next_free_codeword!\n");
                    return EXIT_FAILURE;
                }
                // add pW+C to the dictionary
                dict.add(C, pW);
                next_free_codeword++;
                if (next_free_codeword >= dictionary_size) {
                    if (codeword_size < max_codeword_length) {
                        codeword_size += 1;
                        dictionary_size *= 2;
                    }
                }
            }
            break;
        }
        // shift roles - the current cW becomes the new pW
        pW = cW;
    }
    return EXIT_SUCCESS;
}


// -----------------
// from file to file
// -----------------
int U6Decode::lzw_decompress(std::FILE *input_file, std::FILE *output_file)
{
    if (is_valid_lzw_file(input_file)) {
        // determine the buffer sizes
        long source_buffer_size = get_filesize(input_file) - 4;
        long destination_buffer_size = get_uncompressed_size(input_file);
        // create the buffer
        unsigned char *source_buffer =
            new unsigned char[source_buffer_size];
        unsigned char *destination_buffer =
            new unsigned char[destination_buffer_size];
        // read the input file into the source buffer
        std::fseek(input_file, 4, SEEK_SET);
        if (std::fread(source_buffer, 1, source_buffer_size, input_file)
            != static_cast<size_t>(source_buffer_size)) {
            perror("std::fread failed");
        }
        // decompress the input file
        int error_code = lzw_decompress(
            source_buffer,
            source_buffer_size,
            destination_buffer,
            destination_buffer_size
        );
        if (error_code != EXIT_SUCCESS) {
            delete[] source_buffer;
            delete[] destination_buffer;
            return error_code;
        }
        // write the destination buffer to the output file
        std::fwrite(
            destination_buffer, 1, destination_buffer_size, output_file
        );
        // destroy the buffers
        delete[] source_buffer;
        delete[] destination_buffer;
        return EXIT_SUCCESS;
    } else
        return EXIT_FAILURE;
}


#ifdef STANDALONE

// ----------------------------------------------------------
// called if the program is run with 1 command line parameter
// display uncompressed file size
// ----------------------------------------------------------
int one_argument(const char *file_name)
{
    std::FILE *compressed_file = std::fopen(file_name,"rb");
    if (compressed_file == nullptr) {
        printf("Couldn't open the file.\n");
        return EXIT_FAILURE;
    } else {
        long uncompressed_size = get_uncompressed_size(compressed_file);
        if (uncompressed_size == -1) {
            printf("The input file is not a valid LZW-compressed file.\n");
            return EXIT_FAILURE;
        } else {
            printf(
                "The uncompressed file '%s' would be %ld bytes long.\n",
                file_name,
                get_uncompressed_size(compressed_file)
            );
            std::fclose(compressed_file);
            return EXIT_SUCCESS;
        }
    }
}


// -----------------------------------------------------------
// called if the program is run with 2 command line parameters
// decompress arg1 into arg2
// -----------------------------------------------------------
int two_arguments(
    const char *source_file_name, const char *destination_file_name
)
{
    std::FILE *source;
    std::FILE *destination;
    if (strcmp(source_file_name, destination_file_name) == 0) {
        printf("Source and destination must not be identical.\n");
        return EXIT_FAILURE;
    } else {
        if (!(source=std::fopen(source_file_name,"rb"))
            || !(destination=std::fopen(destination_file_name,"wb"))) {
            printf(
                "Couldn't open '%s' or '%s' or both.\n.",
                source_file_name,
                destination_file_name
            );
            return EXIT_FAILURE;
        } else {
            if (is_valid_lzw_file(source))
                return lzw_decompress(source, destination);
            else {
                printf("The input file is not a valid LZW-compressed file.\n");
                return EXIT_FAILURE;
            }
        }
    }
}

int main(int argc, char *argv[])
{

    // 0 args => print help message
    // 1 arg ==> display uncompressed file size, but don't decompress
    // 2 args => decompress arg1 into arg2
    switch (argc) {
    case 1:
        printf("Usage:\n");
        printf("0 parameters - this message.\n");
        printf("1 parameter  - display uncompressed size.\n");
        printf("2 parameters - extract arg1 into arg2.\n");
        return EXIT_SUCCESS;
        break;
    case 2:
        return one_argument(argv[1]);
        break;
    case 3:
        return two_arguments(argv[1], argv[2]);
        break;
    default:
        printf("Too many command line parameters.\n");
        return EXIT_FAILURE;
    }
}

#endif
