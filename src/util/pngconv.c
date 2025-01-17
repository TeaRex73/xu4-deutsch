#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include "pngconv.h"

static void *my_malloc(size_t size);
static void setBWPalette(png_color *palette);
static void setEgaPalette(png_color *palette);
static void setVgaPalette(png_color *palette);

static void *my_malloc(size_t size)
{
    void *ret = malloc(size);
    if (!ret) {
        perror("out of memory");
        exit(EXIT_FAILURE);
    }
    return ret;
}

static void setBWPalette(png_color *palette)
{
    palette[0].red = 0;
    palette[0].green = 0;
    palette[0].blue = 0;
    palette[1].red = 255;
    palette[1].green = 255;
    palette[1].blue = 255;
}

#define setpalentry(i, r, g, b) do { \
    palette[i].red = r;              \
    palette[i].green = g;            \
    palette[i].blue = b; } while(0)

static void setEgaPalette(png_color *palette)
{
#ifdef RASB_PI
    setpalentry( 0,   0,   0,   0);
    setpalentry( 1,  56, 139, 255);
    setpalentry( 2,  56, 189,   0);
    setpalentry( 3, 255, 255, 255);
    setpalentry( 4, 199,  66, 255);
    setpalentry( 5, 199, 116,   0);
    setpalentry( 6, 199, 116,   0);
    setpalentry( 7, 255, 255, 255);
    setpalentry( 8,   0,   0,   0);
    setpalentry( 9,  56, 139, 255);
    setpalentry(10,  56, 189,   0);
    setpalentry(11,  56, 139, 255);
    setpalentry(12, 199, 116,   0);
    setpalentry(13, 199,  66, 255);
    setpalentry(14, 199, 116,   0);
    setpalentry(15, 255, 255, 255);
#else
    setpalentry( 0,   0,   0,   0);
    setpalentry( 1,  54, 146, 255);
    setpalentry( 2,  60, 204,   0);
    setpalentry( 3, 241, 241, 241);
    setpalentry( 4, 214,  67, 255);
    setpalentry( 5, 216, 115,   0);
    setpalentry( 6, 216, 115,   0);
    setpalentry( 7, 241, 241, 241);
    setpalentry( 8,   0,   0,   0);
    setpalentry( 9,  54, 146, 255);
    setpalentry(10,  60, 204,   0);
    setpalentry(11,  54, 146, 255);
    setpalentry(12, 216, 115,   0);
    setpalentry(13, 214,  67, 255);
    setpalentry(14, 216, 115,   0);
    setpalentry(15, 241, 241, 241);
#endif
}

#undef setpalentry

static void setVgaPalette(png_color *palette)
{
    FILE *pal;
    int i;
    pal = fopen("u4vga.pal", "rb");
    if (!pal) {
        perror("u4vga.pal");
        exit(1);
    }
    for (i = 0; i < 256; i++) {
        palette[i].red = getc(pal) * 255 / 63;
        palette[i].green = getc(pal) * 255 / 63;
        palette[i].blue = getc(pal) * 255 / 63;
    }
    fclose(pal);
}

int writePngFromEga(
    unsigned char *data, int height, int width, int bits, const char *fname
)
{
    FILE *fp;
    const unsigned char *p;
    png_structp png_ptr;
    png_infop info_ptr;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    int palette_size;
    png_color *palette = NULL;
    png_byte **row_pointers;
    int i, j;
    row_pointers = (png_byte **)my_malloc(height * sizeof(png_byte *));
    for (i = 0; i < height; i++)
        row_pointers[i] =
            (png_byte *) my_malloc(width * sizeof(png_byte) * bits / 8);
    p = data;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width * bits / 8; j++) {
            row_pointers[i][j] = *p++;
        }
    }
    fp = fopen(fname, "wb");
    if (!fp) {
        perror(fname);
        exit(1);
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_write_struct error\n");
        exit(1);
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fprintf(stderr, "png_create_info_struct error\n");
        exit(1);
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        fprintf(stderr, "longjump error\n");
        exit(1);
    }
    if (bits == 1) {
        palette_size = 2;
    } else if (bits == 4) {
        palette_size = 16;
    } else if (bits == 8) {
        palette_size = 256;
    } else {
        palette_size = 0;
    }
    if (palette_size != 0) {
        palette = (png_color *)my_malloc(sizeof(png_color) * palette_size);
        printf("palette size = %d\n", palette_size);
        if (palette_size == 2) {
            setBWPalette(palette);
        } else if (palette_size == 16) {
            setEgaPalette(palette);
        } else {
            setVgaPalette(palette);
        }
    } else {
        palette = NULL;
    }
    png_init_io(png_ptr, fp);
    switch (bits) {
    case 1:
    case 4:
    case 8:
        bit_depth = bits;
        color_type = PNG_COLOR_TYPE_PALETTE;
        break;
    case 24:
        bit_depth = bits / 3;
        color_type = PNG_COLOR_TYPE_RGB;
        break;
    case 32:
        bit_depth = bits / 4;
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        break;
    default:
        abort();
    }
    interlace_type = PNG_INTERLACE_NONE;
    compression_type = PNG_COMPRESSION_TYPE_DEFAULT;
    filter_method = PNG_FILTER_TYPE_DEFAULT;
    png_set_IHDR(
        png_ptr,
        info_ptr,
        (png_uint_32) width,
        (png_uint_32) height,
        bit_depth,
        color_type,
        interlace_type,
        compression_type,
        filter_method
    );
    if (palette_size != 0) {
        png_set_PLTE(png_ptr, info_ptr, palette, palette_size);
    }
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    fclose(fp);
    if (palette) free(palette);
    return 0;
}

int readEgaFromPng(
    unsigned char **data, int *height, int *width, int *bits, const char *fname
)
{
    FILE *fp;
    unsigned char *p;
    char header[8];
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    png_uint_32 pwidth, pheight;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    png_byte **row_pointers;
    unsigned int i, j;
    fp = fopen(fname, "rb");
    if (!fp) {
        perror(fname);
        exit(1);
    }
    if (fread(header, 1, sizeof(header), fp) != sizeof(header)) {
        perror("fread failed");
    }
    if (png_sig_cmp((png_byte *)header, 0, sizeof(header)) != 0) {
        fprintf(stderr, "not a PNG\n");
        exit(1);
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_read_struct error\n");
        exit(1);
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(
            &png_ptr, (png_infopp) NULL, (png_infopp) NULL
        );
        fprintf(stderr, "png_create_info_struct error\n");
        exit(1);
    }
    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fprintf(stderr, "png_create_info_struct error\n");
        exit(1);
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        fprintf(stderr, "longjump error\n");
        exit(1);
    }
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sizeof(header));
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    png_get_IHDR(
        png_ptr,
        info_ptr,
        &pwidth, &pheight,
        &bit_depth,
        &color_type,
        &interlace_type,
        &compression_type,
        &filter_method
    );
    *height = pheight;
    *width = pwidth;
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        *bits = bit_depth;
    } else if (color_type == PNG_COLOR_TYPE_RGB) {
        *bits = bit_depth * 3;
    } else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        *bits = bit_depth * 4;
    }
    row_pointers = png_get_rows(png_ptr, info_ptr);
    *data = (unsigned char *) my_malloc(pwidth * pheight * (*bits) / 8);
    p = *data;
    for (i = 0; i < pheight; i++) {
        for (j = 0; j < pwidth * (*bits) / 8; j++) {
            *p++ = row_pointers[i][j];
        }
    }
    fclose(fp);
    return 0;
}
