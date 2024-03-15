#pragma once

#include "example_base.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"

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

    float camera_fov;

} Arcball;

void arcball_setup(Arcball *arcball);
void arcball_event(Arcball *arcball, const sapp_event *e);
Matrix4 arcball_world_to_view(const Arcball *arcball);
Matrix4 arcball_view_to_clip(const Arcball *arcball);
Matrix4 arcball_world_to_clip(const Arcball *arcball);

#ifdef __cplusplus
}
#endif
