#include "rpc.h"
#include "external/json_input.h"
#include "external/json_output.h"
#include "external/cputime.h"
#include "arena.h"
#include "viewer.h"
#include "serialization.h"
#include "ufbx.h"
#include <stdarg.h>
#include <stdio.h>

#if defined(_WIN32)
	#define NOMINMAX
	#include <Windows.h>
#endif

static bool g_pretty = false;
static bool g_verbose = false;
static uint64_t g_start_cpu_tick = 0;

void log_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
#if defined(_WIN32)
	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, args);
	OutputDebugStringA(buf);

	va_list args2;
	va_copy(args2, args);
	vfprintf(stderr, fmt, args2);
	va_end(args2);
#else
	vfprintf(stderr, fmt, args);
#endif
	va_end(args);
}

jso_stream begin_response()
{
	uint64_t cpu_tick = cputime_cpu_tick();
	cputime_end_init();
	jso_stream s;
	jso_init_growable(&s);
	s.pretty = g_pretty;
	jso_object(&s);
	jso_single_line(&s);
	jso_prop_object(&s, "rpc");
	double sec = cputime_cpu_delta_to_sec(NULL, cpu_tick - g_start_cpu_tick);
	jso_prop_double(&s, "duration", sec);
	jso_end_object(&s);
	return s;
}

char *end_response(jso_stream *s)
{
	jso_end_object(s);
	return jso_close_growable(s);
}

char *fmt_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[512];
	int len = vsnprintf(buf, sizeof(buf), fmt, args);
	if (len < 0) len = 0;
	va_end(args);

	jso_stream s = begin_response();
	jso_prop_string_len(&s, "error", buf, len);
	return end_response(&s);
}

enum {
	MAX_NAME_LEN = 64,
	MAX_LODAED_SCENES = 16,
};

typedef struct {
	arena_t arena;
	const char *name;
	ufbx_scene *fbx_scene;
	vi_scene *vi_scene;
} rpc_scene;

typedef struct {
	arena_t arena;
	rpc_scene *scenes;
	size_t num_scenes;
} rpc_globals;

static rpc_globals rpcg;

char *rpc_cmd_init(arena_t *tmp, jsi_obj *args)
{
	arena_init(&rpcg.arena, NULL);

	g_pretty = jsi_get_bool(args, "pretty", g_pretty);
	g_verbose = jsi_get_bool(args, "verbose", g_verbose);

	jso_stream s = begin_response();
	jso_prop_boolean(&s, "pretty", g_pretty);
	jso_prop_boolean(&s, "verbose", g_verbose);
	return end_response(&s);
}

char *rpc_cmd_load_scene(arena_t *tmp, jsi_obj *args)
{
	const char *name = jsi_get_str(args, "name", NULL);
	if (!name) return fmt_error("Missing field: 'name'");
	const void *data = (const void*)jsi_get_int64(args, "dataPointer", 0);
	size_t size = (size_t)jsi_get_int64(args, "size", 0);
	if (!data || !size) return fmt_error("Bad data range: { %p, %zu }", data, size);

	rpc_scene *scene = NULL;
	for (size_t i = 0; i < rpcg.num_scenes; i++) {
		if (!strcmp(rpcg.scenes[i].name, name)) {
			scene = &rpcg.scenes[i];
			break;
		}
	}
	if (!scene) {
		rpcg.num_scenes++;
		rpcg.scenes = arealloc(&rpcg.arena, rpc_scene, rpcg.num_scenes + 1, rpcg.scenes);
		scene = rpcg.scenes + rpcg.num_scenes++;
		memset(scene, 0, sizeof(rpc_scene));
		scene->name = aalloc_copy_str(&scene->arena, name);
	}

	ufbx_load_opts opts = {
		.allow_null_material = true,
	};
	ufbx_error error;
	ufbx_scene *fbx_scene = ufbx_load_memory(data, size, &opts, &error);
	if (!fbx_scene) {
		char *buf = aalloc(tmp, char, 4096);
		ufbx_format_error(buf, sizeof(buf), &error);
		return fmt_error("Failed to load scene:\n%s", buf);
	}

	scene->fbx_scene = fbx_scene;
	scene->vi_scene = vi_make_scene(fbx_scene);

	jso_stream s = begin_response();
	jso_prop(&s, "scene");
	serialize_scene(&s, fbx_scene);
	return end_response(&s);
}

char *rpc_cmd_render(arena_t *tmp, jsi_obj *args)
{
	jsi_obj *target = jsi_get_obj(args, "target");
	jsi_obj *desc = jsi_get_obj(args, "desc");
	if (!target) return fmt_error("Missing field: 'target'");
	if (!desc) return fmt_error("Missing field: 'desc'");

	jso_stream s = begin_response();
	return end_response(&s);
}

char *rpc_handle(arena_t *tmp, jsi_value *value)
{
	jsi_obj *obj = jsi_as_obj(value);
	if (!obj) return fmt_error("Expected a top-level object");

	const char *cmd = jsi_get_str(obj, "cmd", "(missing)");
	if (!strcmp(cmd, "init")) {
		return rpc_cmd_init(tmp, obj);
	} else if (!strcmp(cmd, "loadScene")) {
		return rpc_cmd_load_scene(tmp, obj);
	} else if (!strcmp(cmd, "render")) {
		return rpc_cmd_render(tmp, obj);
	} else {
		return fmt_error("Unknown cmd: '%s'\n", cmd);
	}
}

char *rpc_call(const char *input)
{
	if (g_verbose) {
		log_printf("RPC request: %s\n", input);
	}

	jsi_args args = {
		.store_integers_as_int64 = true,
	};
	jsi_value *value = jsi_parse_string(input, &args);

	cputime_begin_init();
	g_start_cpu_tick = cputime_cpu_tick();

	if (!value) {
		return fmt_error("Failed to parse JSON: %zu:%zu: %s",
			args.error.line, args.error.column, args.error.description);
	}

	arena_t tmp;
	arena_init(&tmp, NULL);
	char *result = rpc_handle(&tmp, value);
	arena_free(&tmp);

	if (g_verbose) {
		log_printf("RPC response: %s\n", result);
	}

	jsi_free(value);
	return result;
}
