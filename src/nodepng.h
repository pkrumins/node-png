#ifndef NODE_PNG_H
#define NODE_PNG_H

#include <node.h>
#include <node_buffer.h>

#include "common.h"

class Png : public node::ObjectWrap {
    int width;
    int height;
    node::Buffer *data;
    buffer_type buf_type;

public:
    static void Initialize(v8::Handle<v8::Object> target);
    Png(node::Buffer *ddata, int wwidth, int hheight, buffer_type bbuf_type);
    v8::Handle<v8::Value> PngEncode();

    static v8::Handle<v8::Value> New(const v8::Arguments &args);
    static v8::Handle<v8::Value> PngEncode(const v8::Arguments &args);
};

#endif

