#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#define WIDTH 720
#define HEIGHT 400

#define SIZE 720*400*4

#define OUTFILE "png.png"

void png_write_file(unsigned char *rgba);
void png_write_mem(unsigned char *rgba);

int main()
{
    FILE *f = fopen("./foo.dat", "rb");
    if (!f) {
        printf("Couldn't open foo.dat\n");
        exit(1);
    }

    unsigned char buf[SIZE];
    int read = fread(buf, sizeof(unsigned char), SIZE, f);
    printf("Read %d bytes.\n", read);

    //png_write_file(buf);
    png_write_mem(buf);
}

void png_write_file(unsigned char *rgba) {
    printf("Writing to %s.\n", OUTFILE);

    FILE *f = fopen(OUTFILE, "wb");
    if (!f) {
        printf("Couldn't open %s\n", OUTFILE);
        exit(1);
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        printf("Couldn't create png_ptr\n");
        exit(1);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr) {
        printf("Couldn't create info_ptr\n");
        exit(1);
    }
    png_init_io(png_ptr, f);
    png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT,
		     8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    png_set_invert_alpha(png_ptr);

    png_bytep *row_pointers;
    row_pointers = malloc(sizeof(png_bytep) * HEIGHT);
    int i;
    for (i=0; i<HEIGHT; i++) {
        row_pointers[i] = &rgba[i*4*WIDTH];
    }
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}

FILE *out = NULL;

void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    if (!out) out = fopen("crap.png", "w+");
    printf("%d\n", length);
    fwrite(data, length, 1, out);
}

void png_write_mem(unsigned char *rgba) {
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        printf("Couldn't create png_ptr\n");
        exit(1);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr) {
        printf("Couldn't create info_ptr\n");
        exit(1);
    }
    png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT,
		     8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    voidp write_io_ptr = png_get_io_ptr(png_ptr);
    png_set_write_fn(png_ptr, write_io_ptr, user_write_data, NULL);

    png_write_info(png_ptr, info_ptr);
    png_set_invert_alpha(png_ptr);

    png_bytep *row_pointers;
    row_pointers = malloc(sizeof(png_bytep) * HEIGHT);
    int i;
    for (i=0; i<HEIGHT; i++) {
        row_pointers[i] = &rgba[i*4*WIDTH];
    }
    printf("foo\n");
    png_write_image(png_ptr, row_pointers);
    printf("bar\n");
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}

