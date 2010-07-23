#include "common.h"
#include "png_encoder.h"
#include "nodepng.h"

using namespace v8;
using namespace node;

void
Png::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncode);
    target->Set(String::NewSymbol("Png"), t->GetFunction());
}

Png::Png(Buffer *ddata, int wwidth, int hheight, buffer_type bbuf_type) :
    data(ddata), width(wwidth), height(hheight), buf_type(bbuf_type) {}

Handle<Value>
Png::PngEncode()
{
    HandleScope scope;

    PngEncoder p((unsigned char *)data->data(), width, height, buf_type);
    Handle<Value> ret = p.encode();
    if (!ret->IsUndefined()) return ret;
    return scope.Close(Encode((char *)p.get_png(), p.get_png_len(), BINARY));
}

Handle<Value>
Png::New(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() < 3)
        return VException("At least three arguments required - data buffer, width, height, [and input buffer type]");
    if (!Buffer::HasInstance(args[0]))
        return VException("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer width.");
    if (!args[2]->IsInt32())
        return VException("Third argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 4) {
        if (!args[3]->IsString())
            return VException("Fourth argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[3]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("Fourth argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
        }
        
        if (str_eq(*bts, "rgb"))
            buf_type = BUF_RGB;
        else if (str_eq(*bts, "bgr"))
            buf_type = BUF_BGR;
        else if (str_eq(*bts, "rgba"))
            buf_type = BUF_RGBA;
        else if (str_eq(*bts, "bgra"))
            buf_type = BUF_BGRA;
        else
            return VException("Fourth argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    Buffer *data = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
    int w = args[1]->Int32Value();
    int h = args[2]->Int32Value();

    if (w < 0)
        return VException("Width smaller than 0.");
    if (h < 0)
        return VException("Height smaller than 0.");

    Png *png = new Png(data, w, h, buf_type);
    png->Wrap(args.This());
    return args.This();
}

Handle<Value>
Png::PngEncode(const Arguments &args)
{
    HandleScope scope;
    Png *png = ObjectWrap::Unwrap<Png>(args.This());
    return png->PngEncode();
}

