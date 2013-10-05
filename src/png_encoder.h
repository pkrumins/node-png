#ifndef PNG_ENCODER_H
#define PNG_ENCODER_H

#include <png.h>

#include "common.h"

class PngEncoder {
    int width, height, bits;
    unsigned char *data;
    char *png;
    unsigned int png_len, mem_len;
    buffer_type buf_type;

public:
    PngEncoder(unsigned char *ddata, int width, int hheight, buffer_type bbuf_type, int bbits);
    ~PngEncoder();

    static void png_chunk_producer(png_structp png_ptr, png_bytep data, png_size_t length);
    void encode();
    const char *get_png() const;
    int get_png_len() const;
};

#endif

