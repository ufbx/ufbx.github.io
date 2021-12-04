#pragma once

#include "external/json_output.h"
#include "ufbx.h"

void serialize_scene(jso_stream *s, ufbx_scene *scene);

