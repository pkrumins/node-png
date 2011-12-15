#ifndef NODE_PNG_H
#define NODE_PNG_H

#include <node.h>
#include <node_buffer.h>

#include "common.h"

class Png : public node::ObjectWrap {
    int width;
    int height;
    buffer_type buf_type;

    static void EIO_PngEncode(eio_req *req);
    static int EIO_PngEncodeAfter(eio_req *req);
public:
    static void Initialize(v8::Handle<v8::Object> target);
    Png(int wwidth, int hheight, buffer_type bbuf_type);
    v8::Handle<v8::Value> PngEncodeSync();

    static v8::Handle<v8::Value> New(const v8::Arguments &args);
    static v8::Handle<v8::Value> PngEncodeSync(const v8::Arguments &args);
    static v8::Handle<v8::Value> PngEncodeAsync(const v8::Arguments &args);
};

#endif

