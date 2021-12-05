#include "json_rpc.h"

#if defined(__EMSCRIPTEN__)

#include <stdio.h>
#include "external/sokol_gfx.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5_webgl.h>
#define JS_ABI EMSCRIPTEN_KEEPALIVE

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_webgl_ctx;

JS_ABI void js_setup()
{
    g_webgl_ctx = emscripten_webgl_create_context("#ufbx-render-canvas", &(EmscriptenWebGLContextAttributes){
        .powerPreference = EM_WEBGL_POWER_PREFERENCE_LOW_POWER,
        .majorVersion = 2,
        .enableExtensionsByDefault = true,
    });

    printf("Setup canvas: %d\n", g_webgl_ctx);

    emscripten_webgl_make_context_current(g_webgl_ctx);

	sg_setup(&(sg_desc) {
        .context = {
            .sample_count = 1,
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .gl.force_gles2 = false,
        },
	});
}

JS_ABI char *js_rpc(char *input)
{
    return rpc_call(input);
}

#else

#endif
