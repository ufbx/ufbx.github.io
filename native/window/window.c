#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "json_rpc.h"
#include "external/json_output.h"
#include "external/sokol_config.h"
#include "external/sokol_gfx.h"
#include "external/sokol_app.h"
#include "external/sokol_glue.h"
#include "external/umath.h"
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

    {
        jso_stream s = begin_request("init");
        jso_prop_boolean(&s, "pretty", true);
        jso_prop_boolean(&s, "verbose", false);
        free(submit_request(&s));
    }

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

static void serialize_vec3(jso_stream *s, um_vec3 v)
{
    jso_single_line(s);
    jso_object(s);
    jso_prop_double(s, "x", v.x);
    jso_prop_double(s, "y", v.y);
    jso_prop_double(s, "z", v.z);
    jso_end_object(s);
}

uint32_t mouse_buttons = 0;
float cam_yaw = 0.0f;
float cam_pitch = 0.0f;
float cam_distance = 5.0f;

void event(const sapp_event *ev)
{
    switch (ev->type) {

    case SAPP_EVENTTYPE_MOUSE_DOWN:
        mouse_buttons |= (1u << ev->mouse_button);
        break;

    case SAPP_EVENTTYPE_MOUSE_UP:
        mouse_buttons &= ~(1u << ev->mouse_button);
        break;

    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        cam_distance *= powf(2.0f, ev->scroll_y * -0.04f);
		cam_distance = um_min(um_max(cam_distance, 0.01f), 10000.0f);
        break;

    case SAPP_EVENTTYPE_MOUSE_MOVE:
        if (mouse_buttons & (1 << SAPP_MOUSEBUTTON_LEFT)) {
            float scale = um_min((float)sapp_width(), (float)sapp_height());
			cam_yaw -= (float)ev->mouse_dx / scale * 360.0f;
			cam_pitch -= (float)ev->mouse_dy / scale * 360.0f;
            cam_pitch = um_min(um_max(cam_pitch, -85.0f), 85.0f);
        }
        break;

    case SAPP_EVENTTYPE_KEY_DOWN:
        if (!ev->key_repeat) {
            if (ev->key_code == SAPP_KEYCODE_T) {
				jso_stream s = begin_request("freeResources");
				jso_prop_boolean(&s, "targets", true);
				free(submit_request(&s));
            } else if (ev->key_code == SAPP_KEYCODE_R) {
				jso_stream s = begin_request("freeResources");
				jso_prop_boolean(&s, "scenes", true);
				free(submit_request(&s));
            }
        }
        break;

    default:
        break;
    
    }
}

void frame(void)
{
    int width = sapp_width();
    int height = sapp_height();

#if 0
    {
		jso_stream s = begin_request("freeResources");
		jso_prop_boolean(&s, "targets", true);
		jso_prop_boolean(&s, "scenes", true);
		free(submit_request(&s));
    }
#endif

    {
		jso_stream s = begin_request("render");

        um_quat qy = um_quat_axis_angle(um_v3(0,1,0), cam_yaw * UM_DEG_TO_RAD);
        um_quat qp = um_quat_axis_angle(um_v3(1,0,0), cam_pitch * UM_DEG_TO_RAD);
        um_quat rot = um_quat_mulrev(qp, qy);
        um_vec3 eye = um_quat_rotate(rot, um_v3(0,0,cam_distance));

        um_vec3 center = um_v3(0.0f, 1.0f, 0.0f);

		jso_prop_object(&s, "target");
        jso_prop_int(&s, "index", 0);
        jso_prop_int(&s, "width", width);
        jso_prop_int(&s, "height", height);
        jso_prop_int(&s, "samples", 4);
		jso_end_object(&s); // target

		jso_prop_object(&s, "desc");
        jso_prop_string(&s, "sceneName", "main");


        jso_prop_object(&s, "camera");
        jso_prop(&s, "position");
        serialize_vec3(&s, um_add3(center, eye));
        jso_prop(&s, "target");
        serialize_vec3(&s, center);
        jso_prop_double(&s, "fieldOfView", 40.0);
        jso_prop_double(&s, "nearPlane", 1.0f);
        jso_prop_double(&s, "farPlane", 100.0f);
        jso_end_object(&s); // camera

        static double time = 0.0f;
        time += 1.0f / 144.0f;
        if (time > 2.8f)
            time = 0.0f;

        jso_prop_object(&s, "animation");
        jso_prop_double(&s, "time", time);
        jso_end_object(&s);

        jso_prop_int(&s, "selectedElement", 50);
        jso_prop_int(&s, "selectedNode", 50);

		jso_end_object(&s); // desc

		free(submit_request(&s));
    }

    {
		jso_stream s = begin_request("present");
        jso_prop_int(&s, "targetIndex", 0);
        jso_prop_int(&s, "width", width);
        jso_prop_int(&s, "height", height);
		free(submit_request(&s));
    }
}

void cleanup(void)
{
}

sapp_desc sokol_main(int argc, char* argv[]) {
    g_argc = argc;
    g_argv = argv;

    return (sapp_desc){
        .init_cb = init,
        .event_cb = &event,
        .frame_cb = &frame,
        .cleanup_cb = &cleanup,
        .width = 800,
        .height = 600,
        .window_title = "UFBX Viewer",
    };
}

