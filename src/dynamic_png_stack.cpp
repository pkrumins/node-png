#include "png_encoder.h"
#include "dynamic_png_stack.h"

using namespace v8;
using namespace node;

std::pair<Point, Point>
DynamicPngStack::optimal_dimension()
{
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

    return std::make_pair(top, bottom);
}

void
DynamicPngStack::construct_png_data(unsigned char *data, Point &top)
{
    if (buf_type == BUF_RGB || buf_type == BUF_BGR) {
        for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it) {
            Png *png = *it;
            int start = (png->y - top.y)*width*4 + (png->x - top.x)*4;
            for (int i = 0; i < png->h; i++) {
                for (int j = 0, k = 0; k < 3*png->w; j+=4, k+=3) {
                    data[start + i*width*4 + j] = png->data[i*png->w*3 + k];
                    data[start + i*width*4 + j + 1] = png->data[i*png->w*3 + k + 1];
                    data[start + i*width*4 + j + 2] = png->data[i*png->w*3 + k + 2];
                    data[start + i*width*4 + j + 3] = 0x00;
                }
            }
        }
    }
    else {
        for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it) {
            Png *png = *it;
            int start = (png->y - top.y)*width*4 + (png->x - top.x)*4;
            for (int i = 0; i < png->h; i++) {
                for (int j = 0; j < 4*png->w; j+=4) {
                    data[start + i*width*4 + j] = png->data[i*png->w*4 + j];
                    data[start + i*width*4 + j + 1] = png->data[i*png->w*4 + j + 1];
                    data[start + i*width*4 + j + 2] = png->data[i*png->w*4 + j + 2];
                    data[start + i*width*4 + j + 3] = png->data[i*png->w*4 + j + 3];
                }
            }
        }
    }
}

void
DynamicPngStack::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    NODE_SET_PROTOTYPE_METHOD(t, "dimensions", Dimensions);
    target->Set(String::NewSymbol("DynamicPngStack"), t->GetFunction());
}

DynamicPngStack::DynamicPngStack(buffer_type bbuf_type) :
    buf_type(bbuf_type) {}


DynamicPngStack::~DynamicPngStack()
{
    for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it)
        delete *it;
}

Handle<Value>
DynamicPngStack::Push(Buffer *buf, int x, int y, int w, int h)
{
    try {
        Png *png = new Png((unsigned char *)buf->data(), buf->length(), x, y, w, h);
        png_stack.push_back(png);
        return Undefined();
    }
    catch (const char *e) {
        return VException(e);
    }
}

Handle<Value>
DynamicPngStack::PngEncodeSync()
{
    HandleScope scope;

    std::pair<Point, Point> optimal = optimal_dimension();
    Point top = optimal.first, bot = optimal.second;

    // printf("width, height: %d, %d\n", bot.x - top.x, bot.y - top.y);

    offset = top;
    width = bot.x - top.x;
    height = bot.y - top.y;

    unsigned char *data = (unsigned char*)malloc(sizeof(*data) * width * height * 4);
    if (!data) return VException("malloc failed in DynamicPngStack::PngEncode");
    memset(data, 0xFF, width*height*4);

    construct_png_data(data, top);

    buffer_type pbt = BUF_RGBA;
    if (buf_type == BUF_BGR || buf_type == BUF_BGRA)
        pbt = BUF_BGRA;

    try {
        PngEncoder p(data, width, height, pbt);
        p.encode();
        free(data);
        return scope.Close(Encode((char *)p.get_png(), p.get_png_len(), BINARY));
    }
    catch (const char *err) {
        return VException(err);
    }
}

Handle<Value>
DynamicPngStack::Dimensions()
{
    HandleScope scope;

    Local<Object> dim = Object::New();
    dim->Set(String::NewSymbol("x"), Integer::New(offset.x));
    dim->Set(String::NewSymbol("y"), Integer::New(offset.y));
    dim->Set(String::NewSymbol("width"), Integer::New(width));
    dim->Set(String::NewSymbol("height"), Integer::New(height));

    return scope.Close(dim);
}


Handle<Value>
DynamicPngStack::New(const Arguments &args)
{
    HandleScope scope;

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 1) {
        if (!args[0]->IsString())
            return VException("First argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[0]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("First argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
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
            return VException("First argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    DynamicPngStack *png_stack = new DynamicPngStack(buf_type);
    png_stack->Wrap(args.This());
    return args.This();
}

Handle<Value>
DynamicPngStack::Push(const Arguments &args)
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

    Buffer *data = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
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

    DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());
    return scope.Close(png_stack->Push(data, x, y, w, h));
}

Handle<Value>
DynamicPngStack::Dimensions(const Arguments &args)
{
    HandleScope scope;

    DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());
    return scope.Close(png_stack->Dimensions());
}

Handle<Value>
DynamicPngStack::PngEncodeSync(const Arguments &args)
{
    HandleScope scope;

    DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());
    return scope.Close(png_stack->PngEncodeSync());
}

struct encode_request {
    Persistent<Function> callback;
    DynamicPngStack *png_obj;
    char *png;
    int png_len;
    char *error;
};

int
DynamicPngStack::EIO_PngEncode(eio_req *req)
{
    encode_request *enc_req = (encode_request *)req->data;
    DynamicPngStack *png = enc_req->png_obj;

    std::pair<Point, Point> optimal = png->optimal_dimension();
    Point top = optimal.first, bot = optimal.second;

    // printf("width, height: %d, %d\n", bot.x - top.x, bot.y - top.y);

    png->offset = top;
    png->width = bot.x - top.x;
    png->height = bot.y - top.y;

    unsigned char *data = (unsigned char*)malloc(sizeof(*data) * png->width * png->height * 4);
    if (!data) {
        enc_req->error = strdup("malloc failed in DynamicPngStack::EIO_PngEncode.");
        return 0;
    }
    memset(data, 0xFF, png->width*png->height*4);

    png->construct_png_data(data, top);

    buffer_type pbt = BUF_RGBA;
    if (png->buf_type == BUF_BGR || png->buf_type == BUF_BGRA)
        pbt = BUF_BGRA;

    try {
        PngEncoder p(data, png->width, png->height, pbt);
        p.encode();
        free(data);
        enc_req->png_len = p.get_png_len();
        enc_req->png = (char *)malloc(sizeof(*enc_req->png)*enc_req->png_len);
        if (!enc_req->png) {
            enc_req->error = strdup("malloc in DynamicPngStack::EIO_PngEncode failed.");
        }
        else {
            memcpy(enc_req->png, p.get_png(), enc_req->png_len);
        }
    }
    catch (const char *err) {
        enc_req->error = strdup(err);
    }

    return 0;
}

int 
DynamicPngStack::EIO_PngEncodeAfter(eio_req *req)
{
    HandleScope scope;

    ev_unref(EV_DEFAULT_UC);
    encode_request *enc_req = (encode_request *)req->data;

    Handle<Value> argv[2];

    if (enc_req->error) {
        argv[0] = Undefined();
        argv[1] = ErrorException(enc_req->error);
    }
    else {
        argv[0] = Local<Value>::New(Encode(enc_req->png, enc_req->png_len, BINARY));
        argv[1] = Undefined();
    }

    TryCatch try_catch; // don't quite see the necessity of this

    enc_req->callback->Call(Context::GetCurrent()->Global(), 2, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    enc_req->callback.Dispose();
    free(enc_req->png);
    free(enc_req->error);

    enc_req->png_obj->Unref();
    free(enc_req);

    return 0;
}

Handle<Value>
DynamicPngStack::PngEncodeAsync(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return VException("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    DynamicPngStack *png = ObjectWrap::Unwrap<DynamicPngStack>(args.This());

    encode_request *enc_req = (encode_request *)malloc(sizeof(*enc_req));
    if (!enc_req)
        return VException("malloc in DynamicPngStack::PngEncodeAsync failed.");

    enc_req->callback = Persistent<Function>::New(callback);
    enc_req->png_obj = png;
    enc_req->png = NULL;
    enc_req->png_len = 0;
    enc_req->error = NULL;

    eio_custom(EIO_PngEncode, EIO_PRI_DEFAULT, EIO_PngEncodeAfter, enc_req);

    ev_ref(EV_DEFAULT_UC);
    png->Ref();

    return Undefined();
}

