#ifndef DYNAMIC_PNG_STACK_H
#define DYNAMIC_PNG_STACK_H

#include <node.h>
#include <node_buffer.h>

#include <utility>
#include <vector>

#include <cstdlib>

#include "common.h"

class DynamicPngStack : public node::ObjectWrap {
    struct Png {
        int len, x, y, w, h;
        unsigned char *data;

        Png(unsigned char *ddata, int llen, int xx, int yy, int ww, int hh) :
            len(llen), x(xx), y(yy), w(ww), h(hh)
        {
            data = (unsigned char *)malloc(sizeof(*data)*len);
            if (!data) throw "malloc failed in DynamicPngStack::Png::Png";
            memcpy(data, ddata, len);
        }

        ~Png() {
            free(data);
        }
    };

    typedef std::vector<Png *> vPng;
    typedef vPng::iterator vPngi;
    vPng png_stack;
    Point offset;
    int width, height;
    buffer_type buf_type;

    std::pair<Point, Point> optimal_dimension();

    static void UV_PngEncode(uv_work_t *req);
    static void UV_PngEncodeAfter(uv_work_t *req);
    void construct_png_data(unsigned char *data, Point &top);

public:
    static void Initialize(v8::Handle<v8::Object> target);
    DynamicPngStack(buffer_type bbuf_type);
    ~DynamicPngStack();

    v8::Handle<v8::Value> Push(unsigned char *buf_data, size_t buf_len, int x, int y, int w, int h);
    v8::Handle<v8::Value> Dimensions();
    v8::Handle<v8::Value> PngEncodeSync();

    static v8::Handle<v8::Value> New(const v8::Arguments &args);
    static v8::Handle<v8::Value> Push(const v8::Arguments &args);
    static v8::Handle<v8::Value> Dimensions(const v8::Arguments &args);
    static v8::Handle<v8::Value> PngEncodeSync(const v8::Arguments &args);
    static v8::Handle<v8::Value> PngEncodeAsync(const v8::Arguments &args);
};

#endif

