#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "json_rpc.h"
#include "external/json_output.h"
#include "external/sokol_config.h"
#include "external/sokol_gfx.h"
#include "external/sokol_app.h"
#include "external/sokol_glue.h"
#include <stdio.h>
#include <stdlib.h>

static char **g_argv;
static int g_argc;

static jso_stream begin_request(const char *cmd)
{
    jso_stream s;
	jso_init_growable(&s);
    s.pretty = true;
	jso_object(&s);
	jso_prop_string(&s, "cmd", cmd);
    return s;
}

static char *submit_request(jso_stream *s)
{
	jso_end_object(s);
	char *json = jso_close_growable(s);
	return rpc_call(json);
}

void init(void)
{
	sg_setup(&(sg_desc) {
		.context = sapp_sgcontext()
	});

    if (g_argc > 1) {
        const char *path = g_argv[1];
        FILE *f = fopen(path, "rb");
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);

        void *data = malloc(size);
        fread(data, 1, size, f);
        fclose(f);

        jso_stream s = begin_request("loadScene");
        jso_prop_string(&s, "name", "main");
        jso_prop_int64(&s, "dataPointer", (int64_t)(intptr_t)data);
        jso_prop_int64(&s, "size", (int64_t)size);
        char *result = submit_request(&s);

        free(result);
        free(data);
    }
}

void frame(void)
{
}

void cleanup(void)
{
}

sapp_desc sokol_main(int argc, char* argv[]) {
    g_argc = argc;
    g_argv = argv;

    {
        jso_stream s = begin_request("setOptions");
        jso_prop_boolean(&s, "pretty", true);
        jso_prop_boolean(&s, "verbose", true);
        free(submit_request(&s));
    }

    return (sapp_desc){
        .init_cb = init,
        .frame_cb = &frame,
        .cleanup_cb = &cleanup,
        .width = 800,
        .height = 600,
        .window_title = "UFBX Viewer",
    };
}

