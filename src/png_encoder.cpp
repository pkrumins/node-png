#include <cstdlib>

#include "png_encoder.h"
#include "common.h"

void
PngEncoder::png_chunk_producer(png_structp png_ptr, png_bytep data, png_size_t length)
{
    PngEncoder *p = (PngEncoder *)png_get_io_ptr(png_ptr);

    if (!p->png) {
        p->png = (char *)malloc(sizeof(p->png)*41); // from tests pngs are at least 41 bytes
        if (!p->png)
            throw "malloc failed in node-png (PngEncoder::png_chunk_producer)";
        p->mem_len = 41;
    }

    if (p->png_len + length > p->mem_len) {
        char *new_png = (char *)realloc(p->png, sizeof(char)*p->png_len + length);
        if (!new_png)
            throw "realloc failed in node-png (PngEncoder::png_chunk_producer).";
        p->png = new_png;
        p->mem_len += length;
    }
    memcpy(p->png + p->png_len, data, length);
    p->png_len += length;
}

PngEncoder::PngEncoder(unsigned char *ddata, int wwidth, int hheight,
                       buffer_type bbuf_type, int bbits) {
    data = ddata;
    width = wwidth;
    height = hheight;
    buf_type = bbuf_type;
    bits = bbits;
    png = NULL;
    png_len = 0;
    mem_len = 0;
}

PngEncoder::~PngEncoder() {
    free(png);
}

void
PngEncoder::encode()
{
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        throw "png_create_write_struct failed.";

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        throw "png_create_info_struct failed.";

    int color_type;
    switch (buf_type) {
    case BUF_RGB:
    case BUF_BGR:
        color_type = PNG_COLOR_TYPE_RGB;
        break;
    case BUF_GRAY:
        color_type = PNG_COLOR_TYPE_GRAY;
        break;
    default:
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }

    png_set_IHDR(png_ptr, info_ptr, width, height,
        bits, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_bytep *row_pointers = NULL;

    try {
        png_set_write_fn(png_ptr, (void *)this, png_chunk_producer, NULL);
        png_write_info(png_ptr, info_ptr);
        png_set_invert_alpha(png_ptr);

        if (buf_type == BUF_BGR || buf_type == BUF_BGRA)
            png_set_bgr(png_ptr);

        png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
        if (!row_pointers)
            throw "malloc failed in node-png (PngEncoder::encode).";

        switch (buf_type) {
        case BUF_RGB:
        case BUF_BGR:
            for (int i=0; i<height; i++)
                row_pointers[i] = data+3*i*width;
            break;
        case BUF_GRAY:
            for (int i=0; i<height; i++)
                row_pointers[i] = data+(bits/8)*i*width;
            break;
        default:
            for (int i=0; i<height; i++)
                row_pointers[i] = data+4*i*width;
        }

        png_write_image(png_ptr, row_pointers);
        png_write_end(png_ptr, NULL);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        free(row_pointers);
    }
    catch (const char *err) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        free(row_pointers);
        throw;
    }
}

const char *
PngEncoder::get_png() const {
    return png;
}

int
PngEncoder::get_png_len() const {
    return png_len;
}

