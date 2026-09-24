#include "vc6.h"

#include <cstdio>
#include <cstdlib>

#include "u4file.h"
#include "xordata.h"

/* these globals are needed to link u4file.o successfully */
// ReSharper disable CppUseInternalLinkage
bool verbose = false;
void print_trace(std::FILE *)
{
}
// ReSharper enable CppUseInternalLinkage

int main()
{    
    for (const auto &file: xorDataMap) {
        std::fprintf(stderr, "Recreating file %s\n", file.first.c_str());
        U4FILE *u4f = u4fopen(file.first);
        const auto len = static_cast<std::size_t>(u4flength(u4f));
        void *data = std::malloc(len);
        if (!data) {
            std::fputs("Out of memory!", stderr);
            return EXIT_FAILURE;
        }
        const std::size_t read_len = u4fread(data, 1, len, u4f);
        if (read_len != len) {
            std::fprintf(stderr, "Error reading file %s.\n", file.first.c_str());
            std::free(data);
            return EXIT_FAILURE;
        }
        u4fclose(u4f);
        std::FILE *disk_file = std::fopen(("out/" + file.first).c_str(), "wb");
        const std::size_t write_len = std::fwrite(data, 1, len, disk_file);
        if (write_len != len) {
            std::fprintf(stderr, "Error writing file out/%s.\n", file.first.c_str());
            std::free(data);
            return EXIT_FAILURE;
        }
        std::free(data);
        std::fflush(disk_file);
        fsync(fileno(disk_file)); // NOLINT(misc-include-cleaner)
        std::fclose(disk_file);
        sync(); // NOLINT(misc-include-cleaner)
    }
    return EXIT_SUCCESS;
}
