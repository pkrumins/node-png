#include <cstdlib>

#include "png_encoder.h"
#include "fixed_png_stack.h"
#include "buffer_compat.h"

using namespace v8;
using namespace node;

void
FixedPngStack::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    target->Set(String::NewSymbol("FixedPngStack"), t->GetFunction());
}

FixedPngStack::FixedPngStack(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type)
{ 
    data = (unsigned char *)malloc(sizeof(*data) * width * height * 4);
    if (!data) throw "malloc failed in node-png (FixedPngStack ctor)";
    memset(data, 0xFF, width*height*4);
}

FixedPngStack::~FixedPngStack()
{
    free(data);
}

void
FixedPngStack::Push(unsigned char *buf_data, int x, int y, int w, int h)
{
    int start = y*width*4 + x*4;
    for (int i = 0; i < h; i++) {
        unsigned char *datap = &data[start + i*width*4];
        for (int j = 0; j < w; j++) {
            *datap++ = *buf_data++;
            *datap++ = *buf_data++;
            *datap++ = *buf_data++;
            *datap++ = (buf_type == BUF_RGB || buf_type == BUF_BGR) ? 0x00 : *buf_data++;
        }
    }
}

Handle<Value>
FixedPngStack::PngEncodeSync()
{
    HandleScope scope;

    buffer_type pbt = (buf_type == BUF_BGR || buf_type == BUF_BGRA) ? BUF_BGRA : BUF_RGBA;

    try {
        PngEncoder encoder(data, width, height, pbt, 8);
        encoder.encode();
        int png_len = encoder.get_png_len();
        Buffer *retbuf = Buffer::New(png_len);
        memcpy(BufferData(retbuf), encoder.get_png(), png_len);
        return scope.Close(retbuf->handle_);
    }
    catch (const char *err) {
        return VException(err);
    }
}

Handle<Value>
FixedPngStack::New(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() < 2)
        return VException("At least two arguments required - width and height [and input buffer type].");
    if (!args[0]->IsInt32())
        return VException("First argument must be integer width.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 3) {
        if (!args[2]->IsString())
            return VException("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[2]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
                    str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
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
            return VException("Third argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    int width = args[0]->Int32Value();
    int height = args[1]->Int32Value();

    try {
        FixedPngStack *png_stack = new FixedPngStack(width, height, buf_type);
        png_stack->Wrap(args.This());
        return args.This();
    }
    catch (const char *e) {
        return VException(e);
    }
}

Handle<Value>
FixedPngStack::Push(const Arguments &args)
{
    HandleScope scope;

    if (!Buffer::HasInstance(args[0]))
        return VException("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer x.");
    if (!args[2]->IsInt32())
        return VException("Third argument must be integer y.");
    if (!args[3]->IsInt32())
        return VException("Fourth argument must be integer w.");
    if (!args[4]->IsInt32())
        return VException("Fifth argument must be integer h.");

    FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());
    int x = args[1]->Int32Value();
    int y = args[2]->Int32Value();
    int w = args[3]->Int32Value();
    int h = args[4]->Int32Value();

    if (x < 0)
        return VException("Coordinate x smaller than 0.");
    if (y < 0)
        return VException("Coordinate y smaller than 0.");
    if (w < 0)
        return VException("Width smaller than 0.");
    if (h < 0)
        return VException("Height smaller than 0.");
    if (x >= png_stack->width) 
        return VException("Coordinate x exceeds FixedPngStack's dimensions.");
    if (y >= png_stack->height) 
        return VException("Coordinate y exceeds FixedPngStack's dimensions.");
    if (x+w > png_stack->width) 
        return VException("Pushed PNG exceeds FixedPngStack's width.");
    if (y+h > png_stack->height) 
        return VException("Pushed PNG exceeds FixedPngStack's height.");

    char *buf_data = BufferData(args[0]->ToObject());

    png_stack->Push((unsigned char*)buf_data, x, y, w, h);

    return Undefined();
}

Handle<Value>
FixedPngStack::PngEncodeSync(const Arguments &args)
{
    HandleScope scope;

    FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());
    return png_stack->PngEncodeSync();
}

void
FixedPngStack::UV_PngEncode(uv_work_t *req)
{
    encode_request *enc_req = (encode_request *)req->data;
    FixedPngStack *png = (FixedPngStack *)enc_req->png_obj;

    try {
        PngEncoder encoder(png->data, png->width, png->height, png->buf_type, 8);
        encoder.encode();
        enc_req->png_len =encoder.get_png_len();
        enc_req->png = (char *)malloc(sizeof(*enc_req->png)*enc_req->png_len);
        if (!enc_req->png) {
            enc_req->error = strdup("malloc in FixedPngStack::UV_PngEncode failed.");
            return;
        }
        else {
            memcpy(enc_req->png,encoder.get_png(), enc_req->png_len);
        }
    }
    catch (const char *err) {
        enc_req->error = strdup(err);
    }
}

void 
FixedPngStack::UV_PngEncodeAfter(uv_work_t *req)
{
    HandleScope scope;

    encode_request *enc_req = (encode_request *)req->data;
    delete req;

    Handle<Value> argv[2];

    if (enc_req->error) {
        argv[0] = Undefined();
        argv[1] = ErrorException(enc_req->error);
    }
    else {
        Buffer *buf = Buffer::New(enc_req->png_len);
        memcpy(BufferData(buf), enc_req->png, enc_req->png_len);
        argv[0] = buf->handle_;
        argv[1] = Undefined();
    }

    TryCatch try_catch; // don't quite see the necessity of this

    enc_req->callback->Call(Context::GetCurrent()->Global(), 2, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    enc_req->callback.Dispose();
    free(enc_req->png);
    free(enc_req->error);

    ((FixedPngStack *)enc_req->png_obj)->Unref();
    free(enc_req);
}

Handle<Value>
FixedPngStack::PngEncodeAsync(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return VException("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    FixedPngStack *png = ObjectWrap::Unwrap<FixedPngStack>(args.This());

    encode_request *enc_req = (encode_request *)malloc(sizeof(*enc_req));
    if (!enc_req)
        return VException("malloc in FixedPngStack::PngEncodeAsync failed.");

    enc_req->callback = Persistent<Function>::New(callback);
    enc_req->png_obj = png;
    enc_req->png = NULL;
    enc_req->png_len = 0;
    enc_req->error = NULL;

    uv_work_t* req = new uv_work_t;
    req->data = enc_req;
    uv_queue_work(uv_default_loop(), req, UV_PngEncode, (uv_after_work_cb)UV_PngEncodeAfter);

    png->Ref();

    return Undefined();
}

