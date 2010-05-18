#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#define WIDTH 720
#define HEIGHT 400

#define SIZE 720*400*4

#define OUTFILE "png.png"

void png_write_mem(unsigned char *rgba);

int chunk = 0;

class PngWriter {
    unsigned char *rgba_;
    int width_;
    int height_;
    const char *file_;
    FILE *fd_;
public:
    PngWriter(unsigned char *rgba, int width, int height, const char *file) :
        rgba_(rgba), width_(width), height_(height), file_(file), fd_(NULL) {}

    static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        PngWriter *w = (PngWriter *)png_get_io_ptr(png_ptr);
        if (!w->fd_) w->fd_ = fopen(w->file_, "w+");
        printf("%d\n", length);

        char buf[100];
        sprintf(buf, "png%d", chunk++);
        FILE *x = fopen(buf, "w+");
        fwrite(data, length, 1, w->fd_);
        fwrite(data, length, 1, x);
        fclose(x);
    }

    void write() {
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

        png_set_write_fn(png_ptr, (void *)this, user_write_data, NULL);

        png_write_info(png_ptr, info_ptr);
        png_set_invert_alpha(png_ptr);

        png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * HEIGHT);
        int i;
        for (i=0; i<HEIGHT; i++) {
            row_pointers[i] = &rgba_[i*4*WIDTH];
        }
        printf("foo\n");
        png_write_image(png_ptr, row_pointers);
        printf("bar\n");
        png_write_end(png_ptr, NULL);
        png_destroy_write_struct(&png_ptr, &info_ptr);
    }

};

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

    PngWriter pngw(buf, 720, 400, "png2.png");
    pngw.write();
}

