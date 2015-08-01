#ifndef NODE_PNG_H
#define NODE_PNG_H

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include "common.h"

class Png : public node::ObjectWrap {
    int width;
    int height;
    buffer_type buf_type;
    int bits;

    static void UV_PngEncode(uv_work_t *req);
    static void UV_PngEncodeAfter(uv_work_t *req);
public:
    static void Initialize(v8::Handle<v8::Object> target);
    Png(int wwidth, int hheight, buffer_type bbuf_type, int bbits);
    v8::Handle<v8::Value> PngEncodeSync(v8::Isolate *isolate);

    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void PngEncodeSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void PngEncodeAsync(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif

