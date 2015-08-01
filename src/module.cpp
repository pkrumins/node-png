#include <node.h>

#include "png.h"

extern "C" void
init(v8::Handle<v8::Object> target)
{
    Png::Initialize(target);
}

