#include <node.h>
#include <node_buffer.h>
#include <png.h>
#include <cstdlib>
#include <vector>

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

    PngEncoder(unsigned char *rgba, int width, int height) :
        rgba_data(rgba), width_(width), height_(height),
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
            ThrowException(Exception::Error(String::New("malloc failed in node-png (PngEncoder::encode).")));

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
        PngEncoder p((unsigned char *)rgba_->data(), width_, height_);
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
        int w = args[1]->Int32Value();
        int h = args[2]->Int32Value();

        if (w < 0)
            ThrowException(Exception::Error(String::New("Width smaller than 0.")));
        if (h < 0)
            ThrowException(Exception::Error(String::New("Height smaller than 0.")));

        Png *png = new Png(rgba, w, h);
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

class FixedPngStack : public ObjectWrap {
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
        target->Set(String::NewSymbol("FixedPngStack"), t->GetFunction());
    }

    FixedPngStack(int width, int height) : ObjectWrap(),
        width_(width), height_(height)
    { 
        rgba = (unsigned char *)malloc(sizeof(unsigned char) * width * height * 4);
        if (!rgba) {
            ThrowException(Exception::Error(String::New("malloc failed in node-png (FixedPngStack ctor)")));
        }
        memset(rgba, 0xFF, width*height*4);
    }

    ~FixedPngStack() { free(rgba); }

    void Push(Buffer *buf, int x, int y, int w, int h) {
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
        PngEncoder p(rgba, width_, height_);
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

        FixedPngStack *png_stack = new FixedPngStack(args[0]->Int32Value(), args[1]->Int32Value());
        png_stack->Wrap(args.This());
        return args.This();
    }

    static Handle<Value>
    Push(const Arguments& args)
    {
        HandleScope scope;

        FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());

        if (!Buffer::HasInstance(args[0]))
            ThrowException(Exception::Error(String::New("First argument must be Buffer.")));
        if (!args[1]->IsInt32())
            ThrowException(Exception::Error(String::New("Second argument must be integer x.")));
        if (!args[2]->IsInt32())
            ThrowException(Exception::Error(String::New("Third argument must be integer y.")));
        if (!args[3]->IsInt32())
            ThrowException(Exception::Error(String::New("Fourth argument must be integer w.")));
        if (!args[4]->IsInt32())
            ThrowException(Exception::Error(String::New("Fifth argument must be integer h.")));

        Buffer *rgba = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
        int x = args[1]->Int32Value();
        int y = args[2]->Int32Value();
        int w = args[3]->Int32Value();
        int h = args[4]->Int32Value();

        if (x < 0)
            ThrowException(Exception::Error(String::New("Coordinate x smaller than 0.")));
        if (y < 0)
            ThrowException(Exception::Error(String::New("Coordinate y smaller than 0.")));
        if (w < 0)
            ThrowException(Exception::Error(String::New("Width smaller than 0.")));
        if (h < 0)
            ThrowException(Exception::Error(String::New("Height smaller than 0.")));
        if (x >= png_stack->width_) 
            ThrowException(Exception::Error(String::New("Coordinate x exceeds FixedPngStack's dimensions.")));
        if (y >= png_stack->height_) 
            ThrowException(Exception::Error(String::New("Coordinate y exceeds FixedPngStack's dimensions.")));
        if (x+w > png_stack->width_) 
            ThrowException(Exception::Error(String::New("Pushed PNG exceeds FixedPngStack's width.")));
        if (y+h > png_stack->height_) 
            ThrowException(Exception::Error(String::New("Pushed PNG exceeds FixedPngStack's height.")));

        png_stack->Push(rgba, x, y, w, h);

        return Undefined();
    }

    static Handle<Value>
    PngEncode(const Arguments& args)
    {
        HandleScope scope;

        FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());
        return png_stack->PngEncode();
    }
};

class DynamicPngStack : public ObjectWrap {
private:
    struct Png {
        int x, y, w, h, len;
        unsigned char *rgba;

        Png(unsigned char *rgba, int len, int x, int y, int w, int h) {
            this->rgba = (unsigned char *)malloc(sizeof(unsigned char)*len);
            if (!this->rgba) {
                ThrowException(Exception::Error(String::New("malloc failed in DynamicPngStack::Png::Png")));
            }
            memcpy(this->rgba, rgba, len);
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
        }

        ~Png() {
            printf("~Png\n");
            free(rgba);
        }
    };

    struct Point {
        int x, y;
        Point() {}
        Point(int xx, int yy) : x(xx), y(yy) {}
    };
    
    typedef std::vector<Png *> vPng;
    typedef vPng::iterator vPngi;
    vPng png_stack;
    Point offset;
    int width, height;

    std::vector<Point> OptimalDimension() {
        Point top(-1, -1), bottom(-1, -1);
        for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it) {
            Png *png = *it;
            if (top.x == -1 || png->x < top.x)
                top.x = png->x;
            if (top.y == -1 || png->y < top.y)
                top.y = png->y;
            if (bottom.x == -1 || png->x + png->w > bottom.x)
                bottom.x = png->x + png->w;
            if (bottom.y == -1 || png->y + png->h > bottom.y)
                bottom.y = png->y + png->h;
        }

        /*
        printf("top    x, y: %d, %d\n", top.x, top.y);
        printf("bottom x, y: %d, %d\n", bottom.x, bottom.y);
        */

        std::vector<Point> ret;
        ret.push_back(top);
        ret.push_back(bottom);
        return ret;
    }

public:
    static void
    Initialize(Handle<Object> target)
    {
        HandleScope scope;
        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        t->InstanceTemplate()->SetInternalFieldCount(1);
        NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
        NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncode);
        NODE_SET_PROTOTYPE_METHOD(t, "dimensions", Dimensions);
        target->Set(String::NewSymbol("DynamicPngStack"), t->GetFunction());
    }

    DynamicPngStack() : ObjectWrap() {}
    ~DynamicPngStack() {
        for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it) {
            delete *it;
        }
    }

    void Push(Buffer *buf, int x, int y, int w, int h) {
        png_stack.push_back(
            new Png((unsigned char *)buf->data(), buf->length(), x, y, w, h)
        );
    }

    Handle<Value> PngEncode() {
        HandleScope scope;

        std::vector<Point> optimal = OptimalDimension();
        Point top = optimal[0], bot = optimal[1];

        // printf("width, height: %d, %d\n", bot.x - top.x, bot.y - top.y);

        offset = top;
        width = bot.x - top.x;
        height = bot.y - top.y;

        unsigned char *rgba = (unsigned char*)malloc(sizeof(unsigned char) * width * height * 4);
        if (!rgba) {
            ThrowException(Exception::Error(String::New("malloc failed in DynamicPngStack::PngEncode")));
        }
        memset(rgba, 0xFF, width*height*4);

        for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it) {
            Png *png = *it;
            unsigned char *data = png->rgba;
            int start = (png->y - top.y)*width*4 + (png->x - top.x)*4;
            for (int i = 0; i < png->h; i++) {
                for (int j = 0; j < 4*png->w; j+=4) {
                    rgba[start + i*width*4 + j] = data[i*png->w*4 + j];
                    rgba[start + i*width*4 + j + 1] = data[i*png->w*4 + j + 1];
                    rgba[start + i*width*4 + j + 2] = data[i*png->w*4 + j + 2];
                    rgba[start + i*width*4 + j + 3] = data[i*png->w*4 + j + 3];
                }
            }
        }

        PngEncoder p(rgba, width, height);
        p.encode();
        return scope.Close(Encode((char *)p.get_png(), p.get_png_len(), BINARY));
    }

    Handle<Value> Dimensions() {
        HandleScope scope;

        Local<Object> dim = Array::New(2);
        dim->Set(String::NewSymbol("x"), Integer::New(offset.x));
        dim->Set(String::NewSymbol("y"), Integer::New(offset.y));
        dim->Set(String::NewSymbol("width"), Integer::New(width));
        dim->Set(String::NewSymbol("height"), Integer::New(height));
        return dim;
    }

protected:
    static Handle<Value>
    New(const Arguments& args)
    {
        HandleScope scope;

        DynamicPngStack *png_stack = new DynamicPngStack();
        png_stack->Wrap(args.This());
        return args.This();
    }

    static Handle<Value>
    Push(const Arguments& args)
    {
        HandleScope scope;

        DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());

        if (!Buffer::HasInstance(args[0]))
            ThrowException(Exception::Error(String::New("First argument must be Buffer.")));
        if (!args[1]->IsInt32())
            ThrowException(Exception::Error(String::New("Second argument must be integer x.")));
        if (!args[2]->IsInt32())
            ThrowException(Exception::Error(String::New("Third argument must be integer y.")));
        if (!args[3]->IsInt32())
            ThrowException(Exception::Error(String::New("Fourth argument must be integer w.")));
        if (!args[4]->IsInt32())
            ThrowException(Exception::Error(String::New("Fifth argument must be integer h.")));

        Buffer *rgba = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
        int x = args[1]->Int32Value();
        int y = args[2]->Int32Value();
        int w = args[3]->Int32Value();
        int h = args[4]->Int32Value();

        if (x < 0)
            ThrowException(Exception::Error(String::New("Coordinate x smaller than 0.")));
        if (y < 0)
            ThrowException(Exception::Error(String::New("Coordinate y smaller than 0.")));
        if (w < 0)
            ThrowException(Exception::Error(String::New("Width smaller than 0.")));
        if (h < 0)
            ThrowException(Exception::Error(String::New("Height smaller than 0.")));

        png_stack->Push(rgba, x, y, w, h);

        return Undefined();
    }

    static Handle<Value>
    Dimensions(const Arguments &args)
    {
        HandleScope scope;

        DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());
        return png_stack->Dimensions();
    }

    static Handle<Value>
    PngEncode(const Arguments &args)
    {
        HandleScope scope;

        DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());
        return png_stack->PngEncode();
    }
};

extern "C" void
init(Handle<Object> target)
{
    HandleScope scope;
    Png::Initialize(target);
    FixedPngStack::Initialize(target);
    DynamicPngStack::Initialize(target);
}

