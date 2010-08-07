#include <node.h>

#include "png.h"
#include "fixed_png_stack.h"
#include "dynamic_png_stack.h"

extern "C" void
init(v8::Handle<v8::Object> target)
{
    v8::HandleScope scope;

    Png::Initialize(target);
    FixedPngStack::Initialize(target);
    DynamicPngStack::Initialize(target);
}

