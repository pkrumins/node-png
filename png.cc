#include <node.h>
#include <node_events.h>
#include <node_buffer.h>
#include <png.h>
#include <cstdlib>

using namespace v8;
using namespace node;

static Persistent<String> end_symbol;
static Persistent<String> data_symbol;

class Png : public EventEmitter {
private:
    int width_;
    int height_;
    Buffer *rgba_;

public:
    static void
    Initialize(v8::Handle<v8::Object> target)
    {
        HandleScope scope;
        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        t->Inherit(EventEmitter::constructor_template);
        t->InstanceTemplate()->SetInternalFieldCount(1);
        end_symbol = NODE_PSYMBOL("end");
        data_symbol = NODE_PSYMBOL("data");
        NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncode);
        target->Set(String::NewSymbol("Png"), t->GetFunction());
    }

    Png(Buffer *rgba, size_t width, size_t height) :
        EventEmitter(), rgba_(rgba), width_(width), height_(height) { }

    static void
    chunk_emitter(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        Png *p = (Png *)png_get_io_ptr(png_ptr);
        Local<Value> args[2] = {
            Encode((char *)data, length, BINARY),
            Integer::New(length)
        };
        p->Emit(data_symbol, 2, args);
    }

    void PngEncode() {
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
            ThrowException(Exception::Error(String::New("png_create_write_struct failed.")));

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!png_ptr)
            ThrowException(Exception::Error(String::New("png_create_info_struct failed.")));

        png_set_IHDR(png_ptr, info_ptr, width_, height_,
                 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        png_set_write_fn(png_ptr, (void *)this, chunk_emitter, NULL);

        png_write_info(png_ptr, info_ptr);
        png_set_invert_alpha(png_ptr);

        png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height_);
        if (!row_pointers)
            ThrowException(Exception::Error(String::New("malloc failed.")));

        unsigned char *rgba_data = (unsigned char *)rgba_->data();
        for (int i=0; i<height_; i++)
            row_pointers[i] = rgba_data+4*i*width_;

        png_write_image(png_ptr, row_pointers);
        png_write_end(png_ptr, NULL);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        free(row_pointers);

        Emit(end_symbol, 0, NULL);
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
        png->PngEncode();
        return Undefined();
    }
};

extern "C" void
init(Handle<Object> target)
{
    HandleScope scope;
    Png::Initialize(target);
}

