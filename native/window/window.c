#include <stdio.h>
#include "viewer.h"
#include "external/sokol_config.h"
#include "external/sokol_gfx.h"
#include "external/sokol_app.h"
#include "external/sokol_glue.h"

void init(void)
{
	sg_setup(&(sg_desc) {
		.context = sapp_sgcontext()
	});
}

void frame(void)
{
}

void cleanup(void)
{
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = &frame,
        .cleanup_cb = &cleanup,
        .width = 800,
        .height = 600,
        .window_title = "UFBX Viewer",
    };
}

