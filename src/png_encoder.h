#ifndef PNG_ENCODER_H
#define PNG_ENCODER_H

#include <node.h>
#include <png.h>
#include <string>
#include "common.h"

class PngEncoder {
    int width, height;
    unsigned char *data;
    char *png;
    int png_len, mem_len;
    buffer_type buf_type; 

    std::string encoder_error;

public:
    PngEncoder(unsigned char *ddata, int width, int hheight, buffer_type bbuf_type);
    ~PngEncoder();

    static void png_chunk_producer(png_structp png_ptr, png_bytep data, png_size_t length);
    v8::Handle<v8::Value> encode();
    const char *get_png() const;
    int get_png_len() const;
};

#endif

