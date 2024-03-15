#pragma once

#include "example_base.h"

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_glue.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// -- Geometry

typedef struct VertexBuffer {
    sg_buffer buffer;
    size_t count;
    size_t stride;
} VertexBuffer;

typedef struct IndexBuffer {
    sg_buffer buffer;
    size_t count;
    size_t stride;
} IndexBuffer;

VertexBuffer create_vertex_buffer(const void *data, size_t count, size_t stride);
IndexBuffer create_index_buffer(const uint32_t *data, size_t count, size_t stride);

void free_vertex_buffer(VertexBuffer buffer);
void free_index_buffer(IndexBuffer buffer);

// -- Shaders

sg_shader load_shader_data(const char *vs_data, const char *fs_data);
sg_shader load_shader_file(const char *vs_path, const char *fs_path);

// -- Pipelines

sg_pipeline_desc pipeline_default_solid();

// -- Frame

void begin_main_pass(uint32_t clear_color);
void end_main_pass();

// -- Setup

void graphics_setup();
void graphics_cleanup();

// -- Utility

typedef struct Arcball {
	float camera_yaw;
	float camera_pitch;
	float camera_distance;
	uint32_t mouse_buttons;

    Vector3 camera_offset;
    Vector3 camera_target;
    Vector3 camera_position;
    Quaternion camera_rotation;

    float camera_fov;

} Arcball;

void arcball_setup(Arcball *arcball, ufbx_scene *scene);
void arcball_event(Arcball *arcball, const sapp_event *e);
Matrix4 arcball_world_to_view(const Arcball *arcball);
Matrix4 arcball_view_to_clip(const Arcball *arcball);
Matrix4 arcball_world_to_clip(const Arcball *arcball);

#ifdef __cplusplus
}
#endif

#if defined(EXAMPLE_IMPLEMENTATION)

#define SOKOL_DEBUG
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_glue.h"

// -- Geometry

VertexBuffer create_vertex_buffer(const void *data, size_t count, size_t stride)
{
	sg_buffer_desc desc = { 0 };
	desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
	desc.size = count * stride;
	desc.data.ptr = data;
	desc.data.size = count * stride;

	VertexBuffer buffer = { 0 };
	if (count > 0) {
		buffer.buffer = sg_make_buffer(&desc);
	} else {
		buffer.buffer.id = SG_INVALID_ID;
	}
	buffer.count = count;
	buffer.stride = stride;
	return buffer;
}

IndexBuffer create_index_buffer(const uint32_t *data, size_t count, size_t stride)
{
	sg_buffer_desc desc = { 0 };
	desc.type = SG_BUFFERTYPE_INDEXBUFFER;
	desc.size = count * stride;
	desc.data.ptr = data;
	desc.data.size = count * stride;

	IndexBuffer buffer = { 0 };
	if (count > 0) {
		buffer.buffer = sg_make_buffer(&desc);
	} else {
		buffer.buffer.id = SG_INVALID_ID;
	}
	buffer.count = count;
	buffer.stride = stride;
	return buffer;
}

void free_vertex_buffer(VertexBuffer buffer)
{
	sg_destroy_buffer(buffer.buffer);
}

void free_index_buffer(IndexBuffer buffer)
{
	sg_destroy_buffer(buffer.buffer);
}

// -- Shaders

// Hacky parsing for uniform blocks to generate Sokol descs to keep examples terse.

static char *load_file(const char *path, size_t *p_size)
{
	FILE *f = fopen(path, "rb");
	assert(f);

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *data = malloc(size + 1);
	assert(data);

	size_t num_read = fread(data, 1, size, f);
	assert(num_read == size);

	data[size] = '\0';
	if (p_size) *p_size = size;

	fclose(f);
	return data;
}

static bool is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\r';
}

static bool is_word(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

#define MAX_TOKENS 64
#define MAX_TOKEN_LENGTH 64

static void tokenize_line(const char **p_ptr, char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH])
{
	const char *p = *p_ptr;
	size_t num_tokens = 0;

	for (;;) {
		// Skip whitespace
		while (is_space(*p)) p++;

		// Terminate at newline or EOF
		if (*p == '\n' || *p == '\0') break;

		// Just keep scanning until the end if out of tokens
		if (num_tokens >= MAX_TOKENS) {
			p++;
			continue;
		}

		char *token = tokens[num_tokens++];
		size_t len = 0;
		if (is_word(*p)) {
			do {
				if (len + 1 < MAX_TOKEN_LENGTH) token[len++] = *p;
				p++;
			} while (is_word(*p));
		} else {
			do {
				if (len + 1 < MAX_TOKEN_LENGTH) token[len++] = *p;
				p++;
			} while (!is_word(*p) && !is_space(*p) && *p != '\n' && *p != '\0');
		}
		token[len] = '\0';
	}

	while (num_tokens < MAX_TOKENS) {
		tokens[num_tokens++][0] = '\0';
	}

	if (*p == '\n') p++;
	*p_ptr = p;
}

typedef struct {
	char uniform_names[SG_MAX_UB_MEMBERS][MAX_TOKEN_LENGTH];
	size_t num_members;
} ShaderStageUbo;

typedef struct {
	ShaderStageUbo ubos[SG_MAX_SHADERSTAGE_UBS];
} ShaderStageInfo;

static void init_shader_stage(sg_shader_stage_desc *stage, ShaderStageInfo *info, const char *data)
{
	char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH];
	uint32_t ubo_index = 0;

	const char *p = data;
	while (*p) {
		tokenize_line(&p, tokens);

		if (!strcmp(tokens[0], "uniform")) {
			const char *type_name = tokens[1];
			const char *name = tokens[2];
			sg_uniform_type type = SG_UNIFORMTYPE_INVALID;
			size_t size = 0;

			if (!strcmp(type_name, "vec3")) {
				type = SG_UNIFORMTYPE_FLOAT3;
				size = 3*4;
			} else if (!strcmp(type_name, "vec4")) {
				type = SG_UNIFORMTYPE_FLOAT4;
				size = 4*4;
			} else if (!strcmp(type_name, "mat4")) {
				type = SG_UNIFORMTYPE_MAT4;
				size = 4*4*4;
			} else {
				assert(0 && "Unknown uniform type");
			}

			ShaderStageUbo *ubo = &info->ubos[ubo_index];
			size_t uni_index = ubo->num_members++;
			char *name_copy = ubo->uniform_names[uni_index];

			snprintf(name_copy, MAX_TOKEN_LENGTH, "%s", name);
			sg_shader_uniform_desc *uni_desc = &stage->uniform_blocks[ubo_index].uniforms[uni_index];
			uni_desc->array_count = 0;
			uni_desc->name = name_copy;
			uni_desc->type = type;
			stage->uniform_blocks[ubo_index].size += size;
		}
	}

	stage->source = data;
}

sg_shader load_shader_data(const char *vs_data, const char *fs_data)
{
	sg_shader_desc desc = { 0 };
	ShaderStageInfo vs_info = { 0 }, fs_info = { 0 };
	init_shader_stage(&desc.vs, &vs_info, vs_data);
	init_shader_stage(&desc.fs, &fs_info, fs_data);
	return sg_make_shader(&desc);
}

sg_shader load_shader_file(const char *vs_path, const char *fs_path)
{
	char *vs_data = load_file(vs_path, NULL);
	char *fs_data = load_file(fs_path, NULL);

	sg_shader shader = load_shader_data(vs_data, fs_data);

	free(vs_data);
	free(fs_data);

	return shader;
}

sg_pipeline_desc pipeline_default_solid()
{
	sg_pipeline_desc desc = {
		.depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_BACK,
        .face_winding = SG_FACEWINDING_CCW,
    };
	return desc;
}

void begin_main_pass(uint32_t clear_color)
{
    sg_pass pass = { 0 };
    pass.swapchain = sglue_swapchain();
    pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass.action.colors[0].clear_value.r = (float)((clear_color >> 16) & 0xff) / 255.0f;
    pass.action.colors[0].clear_value.g = (float)((clear_color >> 8) & 0xff) / 255.0f;
    pass.action.colors[0].clear_value.b = (float)((clear_color >> 0) & 0xff) / 255.0f;
    pass.action.colors[0].clear_value.a = 1.0f;
    sg_begin_pass(&pass);
}

void end_main_pass()
{
	sg_end_pass();
}

void graphics_setup()
{
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
		.logger = { slog_func },
    });
}

void graphics_cleanup()
{
	sg_shutdown();
}

static void arcball_update_camera(Arcball *arcball)
{
	arcball->camera_rotation = Quaternion_mul(
		Quaternion_axis_angle(Vector3_new(0.0f, 1.0f, 0.0f), arcball->camera_yaw * F_DEG_TO_RAD),
		Quaternion_axis_angle(Vector3_new(1.0f, 0.0f, 0.0f), arcball->camera_pitch * F_DEG_TO_RAD));

	arcball->camera_offset = Quaternion_rotate(arcball->camera_rotation, Vector3_new(0.0f, 0.0f, 1.0f));
	arcball->camera_position = Vector3_add(arcball->camera_target, Vector3_mul(arcball->camera_offset, arcball->camera_distance));
}

void arcball_setup(Arcball *arcball, ufbx_scene *scene)
{
	arcball->camera_distance = 5.0f;
	arcball->camera_fov = 60.0f;

	if (scene) {
		Vector3 scene_min = Vector3_new(HUGE_VALF, HUGE_VALF, HUGE_VALF);
		Vector3 scene_max = Vector3_new(-HUGE_VALF, -HUGE_VALF, -HUGE_VALF);

		// Focus on scene content
		for (size_t mesh_ix = 0; mesh_ix < scene->meshes.count; mesh_ix++) {
			ufbx_mesh *mesh = scene->meshes.data[mesh_ix];

			Vector3 min = Vector3_new(HUGE_VALF, HUGE_VALF, HUGE_VALF);
			Vector3 max = Vector3_new(-HUGE_VALF, -HUGE_VALF, -HUGE_VALF);

			ufbx_vec3_list vertices = mesh->vertices;
			for (size_t i = 0; i < vertices.count; i++) {
				Vector3 v = Vector3_ufbx(vertices.data[i]);
				min = Vector3_min(min, v);
				max = Vector3_max(max, v);
			}

			Vector3 center = Vector3_mul(Vector3_add(min, max), 0.5f);
			Vector3 extent = Vector3_mul(Vector3_sub(max, min), 0.5f);

			for (size_t inst_ix = 0; inst_ix < mesh->instances.count; inst_ix++) {
				ufbx_node *node = mesh->instances.data[inst_ix];

				Matrix4 geometry_to_world = Matrix4_ufbx(node->geometry_to_world);
				Matrix4 abs_geometry_to_world = Matrix4_abs(geometry_to_world);

				Vector3 scene_center = Matrix4_transform_point(geometry_to_world, center);
				Vector3 scene_extent = Matrix4_transform_direction(abs_geometry_to_world, extent);

				scene_min = Vector3_min(scene_min, Vector3_sub(scene_center, scene_extent));
				scene_max = Vector3_max(scene_max, Vector3_add(scene_center, scene_extent));
			}
		}

		Vector3 scene_center = Vector3_mul(Vector3_add(scene_min, scene_max), 0.5f);
		Vector3 scene_extent = Vector3_mul(Vector3_sub(scene_max, scene_min), 0.5f);
		float scene_size = float_max(float_max(scene_extent.x, scene_extent.y), scene_extent.z);

		arcball->camera_distance = scene_size * 2.0f;
		arcball->camera_target = scene_center;
	}

	arcball_update_camera(arcball);
}

void arcball_event(Arcball *arcball, const sapp_event *e)
{
	switch (e->type) {
	case SAPP_EVENTTYPE_MOUSE_DOWN:
		arcball->mouse_buttons |= 1u << (uint32_t)e->mouse_button;
		break;
	case SAPP_EVENTTYPE_MOUSE_UP:
		arcball->mouse_buttons &= ~(1u << (uint32_t)e->mouse_button);
		break;
	case SAPP_EVENTTYPE_UNFOCUSED:
		arcball->mouse_buttons = 0;
		break;
	case SAPP_EVENTTYPE_MOUSE_MOVE:
		if (arcball->mouse_buttons & 1) {
			float scale = float_min((float)sapp_width(), (float)sapp_height());
			arcball->camera_yaw -= e->mouse_dx / scale * 180.0f;
			arcball->camera_pitch -= e->mouse_dy / scale * 180.0f;
			arcball->camera_pitch = float_clamp(arcball->camera_pitch, -89.0f, 89.0f);
		} else if (arcball->mouse_buttons & 4) {
			float scale = float_min((float)sapp_width(), (float)sapp_height());
			float speed = 1.0f * arcball->camera_distance;
			float dx = -speed * e->mouse_dx / scale;
			float dy = speed * e->mouse_dy / scale;
			Vector3 right = Quaternion_rotate(arcball->camera_rotation, Vector3_new(1.0f, 0.0f, 0.0f));
			Vector3 up = Quaternion_rotate(arcball->camera_rotation, Vector3_new(0.0f, 1.0f, 0.0f));
			Vector3 delta = Vector3_add(Vector3_mul(right, dx), Vector3_mul(up, dy));
			arcball->camera_target = Vector3_add(arcball->camera_target, delta);
		}
		break;
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
		arcball->camera_distance *= powf(2.0f, e->scroll_y * -0.02f);
		arcball->camera_distance = float_clamp(arcball->camera_distance, 0.01f, 100000.0f);
		break;
	default:
		break;
	}

	arcball_update_camera(arcball);
}

Matrix4 arcball_world_to_view(const Arcball *arcball)
{
	return Matrix4_look(
		arcball->camera_position,
		Vector3_sub(arcball->camera_target, arcball->camera_position),
		Vector3_new(0.0f, 1.0f, 0.0f));
}

Matrix4 arcball_view_to_clip(const Arcball *arcball)
{
	float range = float_max(arcball->camera_distance, 1000.0f);
    return Matrix4_perspective_gl(arcball->camera_fov, sapp_widthf()/sapp_heightf(), 0.001f * range, 2.0f * range);
}

Matrix4 arcball_world_to_clip(const Arcball *arcball)
{
	return Matrix4_mul(arcball_view_to_clip(arcball), arcball_world_to_view(arcball));
}

#endif
