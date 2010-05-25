#include <node.h>
#include <node_buffer.h>
#include <png.h>
#include <cstdlib>

using namespace v8;
using namespace node;

class PngEncoder {
private:
    int width_, height_;
    unsigned char *rgba_data;
    char *png;
    int png_len;

public: 
    static void
    png_chunk_producer(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        PngEncoder *p = (PngEncoder *)png_get_io_ptr(png_ptr);
        p->png_len += length;
        char *new_png = (char *)realloc(p->png, sizeof(char)*p->png_len);
        if (!new_png) {
            free(p->png);
            ThrowException(Exception::Error(
                String::New("realloc failed in node-png (PngEncoder::png_chunk_producer).")));
        }
        p->png = new_png;
        memcpy(p->png+p->png_len-length, data, length);
    }

    PngEncoder(int width, int height, unsigned char *rgba) :
        width_(width), height_(height), rgba_data(rgba),
        png(NULL), png_len(0) {}

    ~PngEncoder() {
        free(png);
    }

    void encode() {
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
            ThrowException(Exception::Error(String::New("png_create_write_struct failed.")));

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!png_ptr)
            ThrowException(Exception::Error(String::New("png_create_info_struct failed.")));

        png_set_IHDR(png_ptr, info_ptr, width_, height_,
                 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        png_set_write_fn(png_ptr, (void *)this, png_chunk_producer, NULL);

        png_write_info(png_ptr, info_ptr);
        png_set_invert_alpha(png_ptr);

        png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height_);
        if (!row_pointers)
            ThrowException(Exception::Error(String::New("malloc failed.")));

        for (int i=0; i<height_; i++)
            row_pointers[i] = rgba_data+4*i*width_;

        png_write_image(png_ptr, row_pointers);
        png_write_end(png_ptr, NULL);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        free(row_pointers);
    }

    char *get_png() {
        return png;
    }

    int get_png_len() {
        return png_len;
    }
};

class Png : public ObjectWrap {
private:
    int width_;
    int height_;
    Buffer *rgba_;

public:
    static void
    Initialize(Handle<Object> target)
    {
        HandleScope scope;
        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        t->InstanceTemplate()->SetInternalFieldCount(1);
        NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncode);
        target->Set(String::NewSymbol("Png"), t->GetFunction());
    }

    Png(Buffer *rgba, int width, int height) : ObjectWrap(),
        rgba_(rgba), width_(width), height_(height) {}

    Handle<Value> PngEncode() {
        HandleScope scope;
        PngEncoder p(width_, height_, (unsigned char *)rgba_->data());
        p.encode();
        return scope.Close(Encode((char *)p.get_png(), p.get_png_len(), BINARY));
    }

protected:
    static Handle<Value>
    New(const Arguments& args)
    {
        HandleScope scope;

        if (args.Length() != 3)
            ThrowException(Exception::Error(String::New("Three arguments required - rgba buffer, width, height.")));
        if (!Buffer::HasInstance(args[0]))
            ThrowException(Exception::Error(String::New("First argument must be Buffer.")));
        if (!args[1]->IsInt32())
            ThrowException(Exception::Error(String::New("Second argument must be integer width.")));
        if (!args[2]->IsInt32())
            ThrowException(Exception::Error(String::New("Third argument must be integer height.")));

        Buffer *rgba = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
        Png *png = new Png(rgba, args[1]->Int32Value(), args[2]->Int32Value());
        png->Wrap(args.This());
        return args.This();
    }

    static Handle<Value>
    PngEncode(const Arguments& args)
    {
        HandleScope scope;
        Png *png = ObjectWrap::Unwrap<Png>(args.This());
        return png->PngEncode();
    }
};

class PngStack : public ObjectWrap {
private:
    int width_, height_;
    unsigned char *rgba;

public:
    static void
    Initialize(Handle<Object> target)
    {
        HandleScope scope;
        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        t->InstanceTemplate()->SetInternalFieldCount(1);
        NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
        NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncode);
        target->Set(String::NewSymbol("PngStack"), t->GetFunction());
    }

    PngStack(int width, int height) : ObjectWrap(),
        width_(width), height_(height)
    { 
        rgba = (unsigned char *)malloc(sizeof(unsigned char) * width * height * 4);
        if (!rgba) {
            ThrowException(Exception::Error(String::New("malloc failed in node-png (PngStack ctor)")));
        }
        memset(rgba, 0xFF, width*height*4);
    }

    ~PngStack() { free(rgba); }

    void Push(int x, int y, int w, int h, Buffer *buf) {
        HandleScope scope;

        unsigned char *data = (unsigned char *)buf->data();
        int start = y*width_*4 + x*4;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < 4*w; j+=4) {
                rgba[start + i*width_*4 + j] = data[i*w*4 + j];
                rgba[start + i*width_*4 + j + 1] = data[i*w*4 + j + 1];
                rgba[start + i*width_*4 + j + 2] = data[i*w*4 + j + 2];
                rgba[start + i*width_*4 + j + 3] = data[i*w*4 + j + 3];
            }
        }
    }

    Handle<Value> PngEncode() {
        HandleScope scope;
        PngEncoder p(width_, height_, rgba);
        p.encode();
        return scope.Close(Encode((char *)p.get_png(), p.get_png_len(), BINARY));
    }

protected:
    static Handle<Value>
    New(const Arguments& args)
    {
        HandleScope scope;

        if (args.Length() != 2)
            ThrowException(Exception::Error(String::New("Two arguments required - width and height.")));
        if (!args[0]->IsInt32())
            ThrowException(Exception::Error(String::New("First argument must be integer width.")));
        if (!args[1]->IsInt32())
            ThrowException(Exception::Error(String::New("Second argument must be integer height.")));

        PngStack *png_stack = new PngStack(args[0]->Int32Value(), args[1]->Int32Value());
        png_stack->Wrap(args.This());
        return args.This();
    }

    static Handle<Value>
    Push(const Arguments& args)
    {
        HandleScope scope;

        PngStack *png_stack = ObjectWrap::Unwrap<PngStack>(args.This());

        if (!args[0]->IsInt32())
            ThrowException(Exception::Error(String::New("First argument must be integer x")));
        if (!args[1]->IsInt32())
            ThrowException(Exception::Error(String::New("Second argument must be integer y")));
        if (!args[2]->IsInt32())
            ThrowException(Exception::Error(String::New("Third argument must be integer w")));
        if (!args[3]->IsInt32())
            ThrowException(Exception::Error(String::New("Fourth argument must be integer h")));
        if (!Buffer::HasInstance(args[4]))
            ThrowException(Exception::Error(String::New("Fifth argument must be Buffer.")));

        int x = args[0]->Int32Value();
        int y = args[1]->Int32Value();
        int w = args[2]->Int32Value();
        int h = args[3]->Int32Value();
        Buffer *rgba = ObjectWrap::Unwrap<Buffer>(args[4]->ToObject());

        png_stack->Push(x, y, w, h, rgba);

        return Undefined();
    }

    static Handle<Value>
    PngEncode(const Arguments& args)
    {
        HandleScope scope;

        PngStack *png_stack = ObjectWrap::Unwrap<PngStack>(args.This());
        return png_stack->PngEncode();
    }
};

extern "C" void
init(Handle<Object> target)
{
    HandleScope scope;
    Png::Initialize(target);
    PngStack::Initialize(target);
}

