
// Example application header for ufbx samples.
// Uses Sokol internally for rendering.

#ifndef EXAMPLE_APP_INCLUDED
#define EXAMPLE_APP_INCLUDED

#include "example_math.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "external/sokol_gfx.h"
#include "external/sokol_app.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Implement this! Called when the app has been started.
// sokol_gfx and other libraries are initialized before this is called.
void example_setup(void);

// Implement this! Called on every Sokol event.
void example_event(const sapp_event *e);

// Implement this! Called every frame while the example is running.
void example_frame(void);

// -- Arguments

typedef struct Args {

    // Filename to open.
    const char *filename;

    // Path to store screenshot to (handled internally).
    const char *screenshot_path;

} Args;

extern Args example_args;

// -- Graphics

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

sg_shader load_shader_data(const char *vs_data, const char *fs_data);
sg_shader load_shader_file(const char *vs_path, const char *fs_path);

sg_pipeline_desc pipeline_default_solid();

// Begin the main framebuffer pass
void begin_main_pass(uint32_t clear_color);

// End the main framebuffer pass
void end_main_pass();

// -- Arcball

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

void arcball_setup(Arcball *arcball, struct ufbx_scene *scene);
void arcball_event(Arcball *arcball, const sapp_event *e);
Matrix4 arcball_world_to_view(const Arcball *arcball);
Matrix4 arcball_view_to_clip(const Arcball *arcball);
Matrix4 arcball_world_to_clip(const Arcball *arcball);

#if defined(__cplusplus)
}
#endif

#endif
