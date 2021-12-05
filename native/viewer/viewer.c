#include "viewer.h"
#include "arena.h"
#include "external/sokol_config.h"
#include "external/sokol_gfx.h"
#include "shaders/copy.h"
#include "shaders/mesh.h"

#if defined(SOKOL_GLES3) || defined(SOKOL_GLES2)
	#define HAS_GL 1
	#include <GLES3/gl3.h>
#else
	#define HAS_GL 0
#endif

// static um_vec2 fbx_to_um_vec2(ufbx_vec2 v) { return um_v2((float)v.x, (float)v.y); }
static um_vec3 fbx_to_um_vec3(ufbx_vec3 v) { return um_v3((float)v.x, (float)v.y, (float)v.z); }
um_quat fbx_to_um_quat(ufbx_quat v) { return um_quat_xyzw((float)v.x, (float)v.y, (float)v.z, (float)v.w); }
um_mat fbx_to_um_mat(ufbx_matrix m) {
	return um_mat_rows(
		(float)m.m00, (float)m.m01, (float)m.m02, (float)m.m03,
		(float)m.m10, (float)m.m11, (float)m.m12, (float)m.m13,
		(float)m.m20, (float)m.m21, (float)m.m22, (float)m.m23,
		0, 0, 0, 1,
	);
}

static void ad_sg_destroy_image(void *user) { sg_destroy_image(*(sg_image*)user); }
static void ad_sg_destroy_buffer(void *user) { sg_destroy_buffer(*(sg_buffer*)user); }
static void ad_sg_destroy_shader(void *user) { sg_destroy_shader(*(sg_shader*)user); }
static void ad_sg_destroy_pipeline(void *user) { sg_destroy_pipeline(*(sg_pipeline*)user); }
static void ad_sg_destroy_pass(void *user) { sg_destroy_pass(*(sg_pass*)user); }

static void *defer_destroy_image(arena_t *a, sg_image image) { return arena_defer(a, &ad_sg_destroy_image, sg_image, &image); }
static void *defer_destroy_buffer(arena_t *a, sg_buffer buffer) { return arena_defer(a, &ad_sg_destroy_buffer, sg_buffer, &buffer); }
static void *defer_destroy_shader(arena_t *a, sg_shader shader) { return arena_defer(a, &ad_sg_destroy_shader, sg_shader, &shader); }
static void *defer_destroy_pipeline(arena_t *a, sg_pipeline pipe) { return arena_defer(a, &ad_sg_destroy_pipeline, sg_pipeline, &pipe); }
static void *defer_destroy_pass(arena_t *a, sg_pass pass) { return arena_defer(a, &ad_sg_destroy_pass, sg_pass, &pass); }

static sg_image make_image(arena_t *a, void **p_defer, const sg_image_desc *desc) {
	sg_image image = sg_make_image(desc);
	if (image.id) defer_destroy_image(a, image);
	return image;
}

static sg_buffer make_buffer(arena_t *a, void **p_defer, const sg_buffer_desc *desc) {
	sg_buffer buffer = sg_make_buffer(desc);
	if (buffer.id) defer_destroy_buffer(a, buffer);
	return buffer;
}

static sg_shader make_shader(arena_t *a, void **p_defer, const sg_shader_desc *desc) {
	sg_shader shader = sg_make_shader(desc);
	if (shader.id) defer_destroy_shader(a, shader);
	return shader;
}

static sg_pipeline make_pipeline(arena_t *a, void **p_defer, const sg_pipeline_desc *desc) {
	sg_pipeline pipe = sg_make_pipeline(desc);
	if (pipe.id) defer_destroy_pipeline(a, pipe);
	return pipe;
}

static sg_pass make_pass(arena_t *a, void **p_defer, const sg_pass_desc *desc) {
	sg_pass pass = sg_make_pass(desc);
	if (pass.id) defer_destroy_pass(a, pass);
	return pass;
}

typedef struct {
	uint32_t max_width, max_height;
	uint32_t cur_width, cur_height;
	sg_image color_target;
	sg_image depth_target;
	sg_pass pass;
	uint32_t msaa;

	void *defer_color;
	void *defer_depth;
	void *defer_pass;
} vi_framebuffer;

typedef struct {
	um_vec3 position;
	um_vec3 normal;
} vi_vertex;

typedef struct {
	uint32_t material_id;
	sg_buffer vertex_buffer;
	sg_buffer index_buffer;
	uint32_t num_indices;
	uint32_t num_vertices;
} vi_part;

typedef struct {
	um_vec3 albedo_color;
} vi_material;

typedef struct {
	um_mat node_to_world;
	um_mat geometry_to_world;
} vi_node;

typedef struct {
	vi_part *parts;
	size_t num_parts;
} vi_mesh;

struct vi_scene {
	arena_t *arena;
	ufbx_scene fbx;

	vi_node *nodes;
	vi_mesh *meshes;
	vi_material *materials;

	um_mat world_to_view;
	um_mat view_to_clip;
	um_mat world_to_clip;
};

enum {
	MAX_FRAMEBUFFERS = 64,
};

typedef struct {
	uint32_t samples;
	sg_pixel_format color_format;
	sg_pixel_format depth_format;
} vi_pipelines_desc;

typedef struct {
	vi_pipelines_desc desc;

	sg_pipeline mesh_pipe;
} vi_pipelines;

typedef struct {
	arena_t arena;

	sg_backend backend;
	sg_buffer quad_buffer;
	sg_pipeline post_pipe;
	sg_pipeline present_pipe;

	vi_framebuffer render_buffer;
	vi_framebuffer framebuffers[MAX_FRAMEBUFFERS];

	sg_shader mesh_shader;

	alist_t(vi_pipelines) pipelines;
} vi_globals;

static vi_globals vig;

static void vi_init_pipelines(vi_pipelines *ps)
{
	int samples = (int)ps->desc.samples;
	sg_pixel_format color_format = ps->desc.color_format;
	sg_pixel_format depth_format = ps->desc.depth_format;

	ps->mesh_pipe = make_pipeline(&vig.arena, NULL, &(sg_pipeline_desc){
		.shader = vig.mesh_shader,
		.depth.write_enabled = true,
		.depth.compare = SG_COMPAREFUNC_LESS_EQUAL,
		.sample_count = samples,
		.colors[0].pixel_format = color_format,
		.depth.pixel_format = depth_format,
		.index_type = SG_INDEXTYPE_UINT32,
		.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
		.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3,
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
	});
}

static vi_pipelines *vi_get_pipelines(const vi_pipelines_desc *desc)
{
	for (size_t i = 0; i < vig.pipelines.count; i++) {
		vi_pipelines *ps = &vig.pipelines.data[i];
		if (!memcmp(&ps->desc, desc, sizeof(vi_pipelines_desc))) {
			return ps;
		}
	}

	vi_pipelines *ps = alist_push_zero(&vig.arena, vi_pipelines, &vig.pipelines);
	memcpy(&ps->desc, desc, sizeof(vi_pipelines_desc));
	vi_init_pipelines(ps);
	return ps;
}

void vi_setup()
{
	arena_init(&vig.arena, NULL);

	vig.backend = sg_query_backend();

	um_vec2 quad_data[] = { um_v2(0.0f, 0.0f), um_v2(2.0f, 0.0f), um_v2(0.0f, 2.0f) };
	vig.quad_buffer = make_buffer(&vig.arena, NULL, &(sg_buffer_desc){
		.type = SG_BUFFERTYPE_VERTEXBUFFER,
		.data = SG_RANGE(quad_data),
	});

	vig.post_pipe = make_pipeline(&vig.arena, NULL, &(sg_pipeline_desc){
		.shader = make_shader(&vig.arena, NULL, copy_shader_desc(vig.backend)),
		.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
		.depth.pixel_format = SG_PIXELFORMAT_NONE,
		.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2,
	});

	vig.present_pipe = make_pipeline(&vig.arena, NULL, &(sg_pipeline_desc){
		.shader = make_shader(&vig.arena, NULL, copy_shader_desc(vig.backend)),
		.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2,
	});

	vig.mesh_shader = make_shader(&vig.arena, NULL, mesh_shader_desc(vig.backend));
}

void vi_shutdown()
{
	arena_free(&vig.arena);
	memset(&vig, 0, sizeof(vig));
}

static void vi_init_node(vi_scene *vs, vi_node *node, ufbx_node *fbx_node)
{
	node->geometry_to_world = fbx_to_um_mat(fbx_node->geometry_to_world);
	node->node_to_world = fbx_to_um_mat(fbx_node->node_to_world);
}

static void vi_init_mesh(vi_scene *vs, vi_mesh *mesh, ufbx_mesh *fbx_mesh)
{
	vi_part *parts = aalloc(vs->arena, vi_part, fbx_mesh->materials.count);
	mesh->parts = parts;

	size_t num_parts = 0;
	for (size_t pi = 0; pi < fbx_mesh->materials.count; pi++) {
		ufbx_mesh_material *fbx_mesh_mat = &fbx_mesh->materials.data[pi];
		if (fbx_mesh_mat->num_triangles == 0) continue;

		vi_part *part = &parts[num_parts++];

		if (fbx_mesh_mat->material) {
			part->material_id = fbx_mesh_mat->material->id;
		} else {
			part->material_id = (uint32_t)vs->fbx.materials.count;
		}

		arena_t tmp;
		arena_init(&tmp, NULL);
		
		size_t num_tri_ix = fbx_mesh->max_face_triangles * 3;
		uint32_t *tri_ix = aalloc_uninit(&tmp, uint32_t, num_tri_ix);

		size_t num_indices = fbx_mesh_mat->num_triangles * 3;
		vi_vertex *vertices = aalloc_uninit(&tmp, vi_vertex, num_indices);
		uint32_t *indices = aalloc_uninit(&tmp, uint32_t, num_indices);

		vi_vertex *vert = vertices;
		for (size_t fi = 0; fi < fbx_mesh_mat->num_faces; fi++) {
			ufbx_face face = fbx_mesh->faces[fbx_mesh_mat->face_indices[fi]];
			size_t num_tris = ufbx_triangulate_face(tri_ix, num_tri_ix, fbx_mesh, face);
			for (size_t ci = 0; ci < num_tris * 3; ci++) {
				size_t index = tri_ix[ci];
				ufbx_vec3 position = ufbx_get_vertex_vec3(&fbx_mesh->vertex_position, index);
				ufbx_vec3 normal = ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, index);
				vert->position = fbx_to_um_vec3(position);
				vert->normal = fbx_to_um_vec3(normal);
				vert++;
			}
		}

		size_t num_vertices = ufbx_generate_indices(vertices, indices, sizeof(vi_vertex), num_indices, NULL, NULL);

		part->vertex_buffer = make_buffer(vs->arena, NULL, &(sg_buffer_desc){
			.type = SG_BUFFERTYPE_VERTEXBUFFER,
			.data = { vertices, num_vertices * sizeof(vi_vertex) },
		});

		part->index_buffer = make_buffer(vs->arena, NULL, &(sg_buffer_desc){
			.type = SG_BUFFERTYPE_INDEXBUFFER,
			.data = { indices, num_indices * sizeof(uint32_t) },
		});

		part->num_indices = (uint32_t)num_indices;
		part->num_vertices = (uint32_t)num_vertices;

		arena_free(&tmp);
	}

	mesh->num_parts = num_parts;
}

vi_scene *vi_make_scene(const ufbx_scene *fbx_scene)
{
	arena_t *arena = arena_create(&vig.arena);
	vi_scene *vs = aalloc(arena, vi_scene, 1);
	vs->arena = arena;
	if (!vs) return NULL;

	vs->fbx = *fbx_scene;

	vs->meshes = aalloc(vs->arena, vi_mesh, fbx_scene->meshes.count);
	vs->nodes = aalloc(vs->arena, vi_node, fbx_scene->nodes.count);
	vs->materials = aalloc(vs->arena, vi_material, fbx_scene->materials.count + 1); // + NULL

	for (size_t i = 0; i < vs->fbx.nodes.count; i++) {
		vi_init_node(vs, &vs->nodes[i], vs->fbx.nodes.data[i]);
	}

	for (size_t i = 0; i < vs->fbx.meshes.count; i++) {
		vi_init_mesh(vs, &vs->meshes[i], vs->fbx.meshes.data[i]);
	}

	// NULL material
	{
		vi_material *mat = &vs->materials[vs->fbx.materials.count];
		mat->albedo_color = um_dup3(0.8f);
	}

	return vs;
}

void vi_free_scene(vi_scene *scene)
{
	arena_free(scene->arena);
}

typedef struct {
	uint32_t width, height;
	uint32_t msaa;
	bool has_depth;
} vi_framebuffer_desc;

static void vi_init_framebuffer(vi_framebuffer *fb, const vi_framebuffer_desc *desc)
{
	fb->cur_width = desc->width;
	fb->cur_height = desc->height;
	if (desc->width <= fb->max_width && desc->height <= fb->max_height && desc->msaa == fb->msaa) return;
	fb->msaa = desc->msaa;

	arena_cancel(&vig.arena, fb->defer_pass);
	arena_cancel(&vig.arena, fb->defer_color);
	arena_cancel(&vig.arena, fb->defer_depth);
	fb->defer_pass = NULL;
	fb->defer_color = NULL;
	fb->defer_depth = NULL;

	uint32_t new_width = desc->width > fb->max_width ? desc->width : fb->max_width;
	uint32_t new_height = desc->height > fb->max_height ? desc->height : fb->max_height;
	fb->max_width = new_width;
	fb->max_height = new_height;

	fb->color_target = make_image(&vig.arena, &fb->defer_color, &(sg_image_desc){
		.width = (int)new_width,
		.height = (int)new_height,
		.sample_count = (int)desc->msaa,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.render_target = true,
		.mag_filter = SG_FILTER_LINEAR,
		.min_filter = SG_FILTER_LINEAR,
	});

	if (desc->has_depth) {
		fb->depth_target = make_image(&vig.arena, &fb->defer_depth, &(sg_image_desc){
			.width = (int)new_width,
			.height = (int)new_height,
			.sample_count = (int)desc->msaa,
			.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
			.render_target = true,
		});
	} else {
		fb->depth_target.id = 0;
	}

	fb->pass = make_pass(&vig.arena, &fb->defer_pass, &(sg_pass_desc){
		.color_attachments[0].image = fb->color_target,
		.depth_stencil_attachment.image = fb->depth_target,
	});
}

static um_mat vi_mat_perspective(float fov, float aspect, float near_z, float far_z)
{
	bool use_d3d = false;
	switch (vig.backend) {
	case SG_BACKEND_GLCORE33: use_d3d = false;
    case SG_BACKEND_GLES2: use_d3d = false;
    case SG_BACKEND_GLES3: use_d3d = false;
    case SG_BACKEND_D3D11: use_d3d = true;
    case SG_BACKEND_METAL_IOS: use_d3d = true;
    case SG_BACKEND_METAL_MACOS: use_d3d = true;
    case SG_BACKEND_METAL_SIMULATOR: use_d3d = true;
    case SG_BACKEND_WGPU: use_d3d = true;
	case SG_BACKEND_DUMMY: use_d3d = true;
	}

	if (use_d3d) {
		return um_mat_perspective_d3d(fov, aspect, near_z, far_z);
	} else {
		return um_mat_perspective_gl(fov, aspect, near_z, far_z);
	}
}

static void vi_draw_meshes(vi_pipelines *ps, vi_scene *vs)
{
	for (size_t mesh_ix = 0; mesh_ix < vs->fbx.meshes.count; mesh_ix++) {
		ufbx_mesh *fbx_mesh = vs->fbx.meshes.data[mesh_ix];
		vi_mesh *mesh = &vs->meshes[mesh_ix];

		for (size_t inst_ix = 0; inst_ix < fbx_mesh->instances.count; inst_ix++) {
			uint32_t node_ix = fbx_mesh->instances.data[inst_ix]->id;
			vi_node *node = &vs->nodes[node_ix];

			for (size_t part_ix = 0; part_ix < mesh->num_parts; part_ix++) {
				vi_part *part = &mesh->parts[part_ix];

				sg_apply_pipeline(ps->mesh_pipe);

				ubo_mesh_vertex_t vu = {
					.geometry_to_world = node->geometry_to_world,
					.world_to_clip = vs->world_to_clip,
				};
				sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(vu));

				sg_apply_bindings(&(sg_bindings){
					.vertex_buffers[0] = part->vertex_buffer,
					.index_buffer = part->index_buffer,
				});

				sg_draw(0, (int)part->num_indices, 1);
			}
		}
	}
}

static void vi_draw_postprocess(uint32_t width, uint32_t height, vi_framebuffer *src)
{
	sg_apply_pipeline(vig.post_pipe);

	ubo_copy_t ubo_copy = {
		.uv_scale.x = (float)src->cur_width / (float)src->max_width,
		.uv_scale.y = (float)src->cur_height / (float)src->max_height,
	};
	sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_ubo_copy, SG_RANGE_REF(ubo_copy));

	sg_apply_bindings(&(sg_bindings){
		.fs_images[0] = src->color_target,
		.vertex_buffers[0] = vig.quad_buffer,
	});

	sg_draw(0, 3, 1);
}

static void vi_draw_present(vi_framebuffer *src)
{
	sg_apply_pipeline(vig.present_pipe);

	ubo_copy_t ubo_copy = {
		.uv_scale.x = (float)src->cur_width / (float)src->max_width,
		.uv_scale.y = (float)src->cur_height / (float)src->max_height,
	};
	sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_ubo_copy, SG_RANGE_REF(ubo_copy));

	sg_apply_bindings(&(sg_bindings){
		.fs_images[0] = src->color_target,
		.vertex_buffers[0] = vig.quad_buffer,
	});

	sg_draw(0, 3, 1);
}

static void vi_update(vi_scene *vs, const vi_target *target, const vi_desc *desc)
{
	float aspect = (float)target->width / (float)target->height;

	vs->world_to_view = um_mat_look_at(desc->camera_pos, desc->camera_target, um_v3(0,1,0));
	vs->view_to_clip = vi_mat_perspective(desc->field_of_view * UM_DEG_TO_RAD, aspect, desc->near_plane, desc->far_plane);
	vs->world_to_clip = um_mat_mulrev(vs->world_to_view, vs->view_to_clip);

	for (size_t i = 0; i < vs->fbx.nodes.count; i++) {
		ufbx_node *fbx_node = vs->fbx.nodes.data[i];
		vi_node *node = &vs->nodes[i];
		node->node_to_world = fbx_to_um_mat(fbx_node->node_to_world);
		node->geometry_to_world = fbx_to_um_mat(fbx_node->geometry_to_world);
	}
}

void vi_render(vi_scene *vs, const vi_target *target, const vi_desc *desc)
{
	assert(target->target_index < MAX_FRAMEBUFFERS);

	vi_update(vs, target, desc);

	vi_framebuffer *render_fb = &vig.render_buffer;
	vi_framebuffer *dst_fb = &vig.framebuffers[target->target_index];

	uint32_t samples = target->samples > 0 ? target->samples : 1;
	vi_pipelines *ps = vi_get_pipelines(&(vi_pipelines_desc){
		.color_format = SG_PIXELFORMAT_RGBA8,
		.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
		.samples = target->samples,
	});

	vi_init_framebuffer(render_fb, &(vi_framebuffer_desc){
		.width = target->width,
		.height = target->height,
		.msaa = samples,
		.has_depth = true,
	});

	vi_init_framebuffer(dst_fb, &(vi_framebuffer_desc){
		.width = target->width,
		.height = target->height,
		.has_depth = false,
	});

	sg_begin_pass(render_fb->pass, &(sg_pass_action){
		.colors[0].action = SG_ACTION_CLEAR,
		.colors[0].value = { 0.2f, 0.2f, 0.3f, 1.0f },
		.depth.action = SG_ACTION_CLEAR,
		.depth.value = 1.0f,
	});

	sg_apply_viewport(0, 0, (int)render_fb->cur_width, (int)render_fb->cur_height, true);

	vi_draw_meshes(ps, vs);

	sg_end_pass();

	sg_begin_pass(dst_fb->pass, &(sg_pass_action){
		.colors[0].action = SG_ACTION_DONTCARE,
	});

	sg_apply_viewport(0, 0, (int)dst_fb->cur_width, (int)dst_fb->cur_height, true);
	vi_draw_postprocess(dst_fb->cur_width, dst_fb->cur_height, render_fb);

	sg_end_pass();
}

void vi_present(uint32_t target_index, uint32_t width, uint32_t height)
{
	vi_framebuffer *src_fb = &vig.framebuffers[target_index];

	sg_begin_default_pass(&(sg_pass_action){
		.colors[0].action = SG_ACTION_DONTCARE,
	}, (int)width, (int)height);

	vi_draw_present(src_fb);

	sg_end_pass();
}

bool vi_get_pixels(uint32_t target_index, uint32_t width, uint32_t height, void *dst)
{
#if HAS_GL
	vi_framebuffer *src_fb = &vig.framebuffers[target_index];
	sg_pass_info info = sg_query_pass_info(src_fb->pass);

	GLint prev_fb;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, info.hack_gl_fb);
	glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_RGBA, GL_UNSIGNED_BYTE, dst);
	glBindFramebuffer(GL_FRAMEBUFFER, prev_fb);
	return true;
#else
	return false;
#endif
}
