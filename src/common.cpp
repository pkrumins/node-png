#include <cstdlib>
#include <cassert>
#include "common.h"

using namespace v8;

Handle<Value>
ErrorException(Isolate *isolate, const char *msg)
{
    HandleScope scope(isolate);
    return Exception::Error(String::NewFromUtf8(isolate, msg));
}

Handle<Value>
VException(Isolate *isolate, const char *msg)
{
    HandleScope scope(isolate);
    return isolate->ThrowException(ErrorException(isolate, msg));
}

bool str_eq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

