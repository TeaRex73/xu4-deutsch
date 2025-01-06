#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* XOR file in first arg with file given in second arg (used repeatedly
   if too short) */
/* based on code from stackoverflow, by Sato Katsura */

int main(int argc, const char *argv[])
{
    FILE *f, *kf;
    size_t ks, n, i, j;
    int first = 1;
    long pos;
    unsigned char *key, *buf;
    char *name, *keyname;
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file> <key>\a\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if ((f = fopen(argv[1], "rb")) == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    if ((kf = fopen(argv[2], "rb")) == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    if (fseek(kf, 0L, SEEK_END)) {
        perror("fseek");
        exit(EXIT_FAILURE);
    }
    if ((pos = ftell(kf)) < 0L) {
        perror("ftell");
        exit(EXIT_FAILURE);
    }
    ks = (size_t)pos;
    if (!ks) {
        fputs("key size zero is not allowed\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (fseek(kf, 0L, SEEK_SET)) {
        perror("fseek");
        exit(EXIT_FAILURE);
    }
    if ((key = (unsigned char *)malloc(ks)) == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if ((buf = (unsigned char *)malloc(ks)) == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (fread(key, 1, ks, kf) != ks) {
        perror("fread");
        exit(EXIT_FAILURE);
    }
    if (fclose(kf)) {
        perror("fclose");
        exit(EXIT_FAILURE);
    }
    name = strdup(argv[1]);
    if (!name) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < strlen(name); i++) {
        name[i] = tolower(name[i]);
    }
    keyname = strdup(argv[2]);
    if (!keyname) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < strlen(keyname); i++) {
        keyname[i] = tolower(keyname[i]);
    }
    printf("    {\n        \"%s\",\n", name);
    printf("        {\n            \"%s\",\n            {\n", keyname);
    j = 0;
    while ((n = fread(buf, 1, ks, f))) {
        for (i = 0; i < n; i++) buf[i] ^= key[i];
        for (i = 0; i < n; i++) {
            if (first) {
                first = 0;
            } else {
                fputs(",", stdout);
            }
            if (j == 8) {
                fputs("\n", stdout);
                j = 0;
            }
            if (j == 0) fputs("               ", stdout);
            printf(" 0x%02x", buf[i]);
            j++;
        }
    }
    if (ferror(f)) {
        perror("fread");
        exit(EXIT_FAILURE);
    }
    fputs("\n            }\n        }\n    },\n", stdout);
    fflush(stdout);
    free(buf);
    free(key);
    exit(EXIT_SUCCESS);
}
