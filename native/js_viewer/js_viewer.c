#include "json_rpc.h"

#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
    #define JS_ABI EMSCRIPTEN_KEEPALIVE
#else
    #define JS_ABI
#endif

JS_ABI char *js_rpc(const char *input)
{
    return rpc_call(input);
}
