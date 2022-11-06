#pragma once

#include "ufbx.h"
#include "external/umath.h"
#include <stdint.h>

typedef struct vi_scene vi_scene;

typedef struct vi_target {
	uint32_t target_index;
	uint32_t width;
	uint32_t height;
	uint32_t samples;
	float pixel_scale;
} vi_target;

typedef struct vi_desc {
	um_vec3 camera_pos;
	um_vec3 camera_target;
	float field_of_view;
	float near_plane;
	float far_plane;
	uint32_t selected_element_id;
	uint32_t highlight_vertex_index;
	uint32_t highlight_face_index;
	double time;

	const ufbx_prop_override *overrides;
	size_t num_overrides;
} vi_desc;

void vi_setup();
void vi_shutdown();
void vi_free_targets();

vi_scene *vi_make_scene(ufbx_scene *fbx_scene);
void vi_free_scene(vi_scene *scene);

void vi_render(vi_scene *scene, const vi_target *target, const vi_desc *desc);
void vi_present(uint32_t target_index, uint32_t width, uint32_t height);
bool vi_get_pixels(uint32_t target_index, uint32_t width, uint32_t height, void *dst);
