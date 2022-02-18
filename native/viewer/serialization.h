#pragma once

#include "external/json_output.h"
#include "ufbx.h"

static void jso_prop_vec2(jso_stream *s, const char *name, ufbx_vec2 value)
{
	jso_prop_object(s, name);
	jso_prop_double(s, "x", value.x);
	jso_prop_double(s, "y", value.y);
	jso_end_object(s);
}

static void jso_prop_vec3(jso_stream *s, const char *name, ufbx_vec3 value)
{
	jso_prop_object(s, name);
	jso_prop_double(s, "x", value.x);
	jso_prop_double(s, "y", value.y);
	jso_prop_double(s, "z", value.z);
	jso_end_object(s);
}

static void jso_prop_ustring(jso_stream *s, const char *key, ufbx_string str)
{
	jso_prop_string_len(s, key, str.data, str.length);
}

void serialize_scene(jso_stream *s, ufbx_scene *scene);

