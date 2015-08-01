#include <cstring>
#include <cstdlib>
#include "common.h"
#include "png_encoder.h"
#include "png.h"

using namespace v8;
using namespace node;

void
Png::Initialize(Handle<Object> target)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Local<FunctionTemplate> t = FunctionTemplate::New(isolate, New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    target->Set(String::NewFromUtf8(isolate, "Png", String::kInternalizedString), t->GetFunction());
}

Png::Png(int wwidth, int hheight, buffer_type bbuf_type, int bbits) :
    width(wwidth), height(hheight), buf_type(bbuf_type), bits(bbits) {}

Handle<Value>
Png::PngEncodeSync(Isolate *isolate)
{
    EscapableHandleScope scope(isolate);

    Local<Value> buf_val = handle(isolate)->GetHiddenValue(String::NewFromUtf8(isolate, "buffer"));

    char *buf_data = Buffer::Data(buf_val->ToObject());

    try {
        PngEncoder encoder((unsigned char*)buf_data, width, height, buf_type, bits);
        encoder.encode();
        int png_len = encoder.get_png_len();
        Local<Object> retbuf = Buffer::New(isolate, png_len);
        memcpy(Buffer::Data(retbuf), encoder.get_png(), png_len);
        return scope.Escape(retbuf);
    }
    catch (const char *err) {
        return VException(isolate, err);
    }
}

void
Png::New(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    ReturnValue<Value> ret = args.GetReturnValue();

    if (args.Length() < 3)
        return ret.Set(VException(isolate, "At least three arguments required - data buffer, width, height, [and input buffer type]"));
    if (!Buffer::HasInstance(args[0]))
        return ret.Set(VException(isolate, "First argument must be Buffer."));
    if (!args[1]->IsInt32())
        return ret.Set(VException(isolate, "Second argument must be integer width."));
    if (!args[2]->IsInt32())
        return ret.Set(VException(isolate, "Third argument must be integer height."));

    buffer_type buf_type = BUF_RGB;
    if (args.Length() >= 4) {
        if (!args[3]->IsString())
            return ret.Set(VException(isolate, "Fourth argument must be 'gray', 'rgb', 'bgr', 'rgba' or 'bgra'."));

        String::Utf8Value bts(args[3]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra") ||
            str_eq(*bts, "gray")))
        {
            return ret.Set(VException(isolate, "Fourth argument must be 'gray', 'rgb', 'bgr', 'rgba' or 'bgra'."));
        }
        
        if (str_eq(*bts, "rgb"))
            buf_type = BUF_RGB;
        else if (str_eq(*bts, "bgr"))
            buf_type = BUF_BGR;
        else if (str_eq(*bts, "rgba"))
            buf_type = BUF_RGBA;
        else if (str_eq(*bts, "bgra"))
            buf_type = BUF_BGRA;
        else if (str_eq(*bts, "gray"))
            buf_type = BUF_GRAY;
        else
            return ret.Set(VException(isolate, "Fourth argument wasn't 'gray', 'rgb', 'bgr', 'rgba' or 'bgra'."));
    }

    int bits = 8;

    if (args.Length() >= 5) {
        if(buf_type != BUF_GRAY)
            return ret.Set(VException(isolate, "Pixel bit width option only valid for \"gray\" buffer type"));
        if(!args[4]->IsInt32())
            return ret.Set(VException(isolate, "Fifth argument must be 8 or 16"));

        if(args[4]->Int32Value() == 8)
            bits = 8;
        else if (args[4]->Int32Value() == 16)
            bits = 16;
        else
            return ret.Set(VException(isolate, "Fifth arguments wasn't 8 or 16"));
    }

    int w = args[1]->Int32Value();
    int h = args[2]->Int32Value();


    if (w < 0)
        return ret.Set(VException(isolate, "Width smaller than 0."));
    if (h < 0)
        return ret.Set(VException(isolate, "Height smaller than 0."));

    Png *png = new Png(w, h, buf_type, bits);
    png->Wrap(args.This());

    // Save buffer.
    png->handle()->SetHiddenValue(String::NewFromUtf8(isolate, "buffer"), args[0]);

    return ret.Set(args.This());
}

void
Png::PngEncodeSync(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    Png *png = ObjectWrap::Unwrap<Png>(args.This());
    args.GetReturnValue().Set(png->PngEncodeSync(isolate));
}

void
Png::UV_PngEncode(uv_work_t* req)
{
    encode_request *enc_req = (encode_request *)req->data;
    Png *png = (Png *)enc_req->png_obj;

    try {
        PngEncoder encoder((unsigned char *)enc_req->buf_data, png->width, png->height, png->buf_type, png->bits);
        encoder.encode();
        enc_req->png_len = encoder.get_png_len();
        enc_req->png = (char *)malloc(sizeof(*enc_req->png)*enc_req->png_len);
        if (!enc_req->png) {
            enc_req->error = strdup("malloc in Png::UV_PngEncode failed.");
            return;
        }
        else {
            memcpy(enc_req->png, encoder.get_png(), enc_req->png_len);
        }
    }
    catch (const char *err) {
        enc_req->error = strdup(err);
    }
}

void 
Png::UV_PngEncodeAfter(uv_work_t *req)
{
    encode_request *enc_req = (encode_request *)req->data;
    delete req;
    
    Isolate *isolate = enc_req->isolate;
    HandleScope scope(isolate);

    Handle<Value> argv[2];

    if (enc_req->error) {
        argv[0] = Undefined(isolate);
        argv[1] = ErrorException(isolate, enc_req->error);
    }
    else {
        Local<Object> buf = Buffer::New(enc_req->png_len);
        memcpy(Buffer::Data(buf), enc_req->png, enc_req->png_len);
        argv[0] = buf;
        argv[1] = Undefined(isolate);
    }

    TryCatch try_catch; // don't quite see the necessity of this
    
    Local<Function> callback = Local<Function>::New(isolate, enc_req->callback);

    callback->Call(isolate->GetCurrentContext()->Global(), 2, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    free(enc_req->png);
    free(enc_req->error);

    ((Png *)enc_req->png_obj)->Unref();
    free(enc_req);
}

void
Png::PngEncodeAsync(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    ReturnValue<Value> ret = args.GetReturnValue();

    if (args.Length() != 1)
        return ret.Set(VException(isolate, "One argument required - callback function."));

    if (!args[0]->IsFunction())
        return ret.Set(VException(isolate, "First argument must be a function."));

    Local<Function> callback = Local<Function>::Cast(args[0]);
    Png *png = ObjectWrap::Unwrap<Png>(args.This());

    encode_request *enc_req = new encode_request(isolate, callback);
    enc_req->png_obj = png;
    enc_req->png = NULL;
    enc_req->png_len = 0;
    enc_req->error = NULL;

    // We need to pull out the buffer data before
    // we go to the thread pool.
    Local<Value> buf_val = png->handle()->GetHiddenValue(String::NewFromUtf8(isolate, "buffer"));

    enc_req->buf_data = Buffer::Data(buf_val->ToObject());


    uv_work_t* req = new uv_work_t;
    req->data = enc_req;
    uv_queue_work(uv_default_loop(), req, UV_PngEncode, (uv_after_work_cb)UV_PngEncodeAfter);

    png->Ref();
}

NODE_MODULE(png, Png::Initialize)
