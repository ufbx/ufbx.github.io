#include "viewer.h"
#include "arena.h"
#include "external/sokol_config.h"
#include "external/sokol_gfx.h"
#include "external/sokol_gl.h"
#include "shaders/copy.h"
#include "shaders/mesh.h"

#if defined(SOKOL_GLES3) || defined(SOKOL_GLES2)
	#define HAS_GL 1
	#include <GLES3/gl3.h>
#else
	#define HAS_GL 0
#endif

// TEMPT MEP
#include <stdio.h>

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
static void ad_sg_destroy_sgl_context(void *user) { sgl_destroy_context(*(sgl_context*)user); }

static void *defer_destroy_image(arena_t *a, sg_image image) { return arena_defer(a, &ad_sg_destroy_image, sg_image, &image); }
static void *defer_destroy_buffer(arena_t *a, sg_buffer buffer) { return arena_defer(a, &ad_sg_destroy_buffer, sg_buffer, &buffer); }
static void *defer_destroy_shader(arena_t *a, sg_shader shader) { return arena_defer(a, &ad_sg_destroy_shader, sg_shader, &shader); }
static void *defer_destroy_pipeline(arena_t *a, sg_pipeline pipe) { return arena_defer(a, &ad_sg_destroy_pipeline, sg_pipeline, &pipe); }
static void *defer_destroy_pass(arena_t *a, sg_pass pass) { return arena_defer(a, &ad_sg_destroy_pass, sg_pass, &pass); }
static void *defer_destroy_sgl_context(arena_t *a, sgl_context ctx) { return arena_defer(a, &ad_sg_destroy_sgl_context, sgl_context, &ctx); }

static sg_image make_image(arena_t *a, void **p_defer, const sg_image_desc *desc) {
	sg_image image = sg_make_image(desc);
	assert(image.id);
	void *defer = NULL;
	if (image.id) defer = defer_destroy_image(a, image);
	if (p_defer) *p_defer = defer;
	return image;
}

static sg_buffer make_buffer(arena_t *a, void **p_defer, const sg_buffer_desc *desc) {
	sg_buffer buffer = sg_make_buffer(desc);
	assert(buffer.id);
	void *defer = NULL;
	if (buffer.id) defer = defer_destroy_buffer(a, buffer);
	if (p_defer) *p_defer = defer;
	return buffer;
}

static sg_shader make_shader(arena_t *a, void **p_defer, const sg_shader_desc *desc) {
	sg_shader shader = sg_make_shader(desc);
	assert(shader.id);
	void *defer = NULL;
	if (shader.id) defer = defer_destroy_shader(a, shader);
	if (p_defer) *p_defer = defer;
	return shader;
}

static sg_pipeline make_pipeline(arena_t *a, void **p_defer, const sg_pipeline_desc *desc) {
	sg_pipeline pipe = sg_make_pipeline(desc);
	assert(pipe.id);
	void *defer = NULL;
	if (pipe.id) defer = defer_destroy_pipeline(a, pipe);
	if (p_defer) *p_defer = defer;
	return pipe;
}

static sg_pass make_pass(arena_t *a, void **p_defer, const sg_pass_desc *desc) {
	sg_pass pass = sg_make_pass(desc);
	assert(pass.id);
	void *defer = NULL;
	if (pass.id) defer = defer_destroy_pass(a, pass);
	if (p_defer) *p_defer = defer;
	return pass;
}

static sgl_context make_sgl_context(arena_t *a, void **p_defer, const sgl_context_desc_t *desc) {
	sgl_context ctx = sgl_make_context(desc);
	assert(ctx.id);
	void *defer = NULL;
	if (ctx.id) defer = defer_destroy_sgl_context(a, ctx);
	if (p_defer) *p_defer = defer;
	return ctx;
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
	uint8_t info[4];
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

	sgl_context gl_context;
	sg_pipeline mesh_pipe;
} vi_pipelines;

typedef struct {
	arena_t arena;

	sg_backend backend;
	bool origin_top_left;
	sg_buffer quad_buffer;
	sg_pipeline post_pipe;
	sg_pipeline present_pipe;

	arena_t *fb_arena;
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
		.layout.attrs[2].format = SG_VERTEXFORMAT_UBYTE4,
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
	});

	ps->gl_context = make_sgl_context(&vig.arena, NULL, &(sgl_context_desc_t){
		.color_format = color_format,
		.depth_format = depth_format,
		.sample_count = samples,
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
	switch (vig.backend) {
	case SG_BACKEND_GLCORE33: vig.origin_top_left = false; break;
    case SG_BACKEND_GLES2: vig.origin_top_left = false; break;
    case SG_BACKEND_GLES3: vig.origin_top_left = false; break;
    case SG_BACKEND_D3D11: vig.origin_top_left = true; break;
    case SG_BACKEND_METAL_IOS: vig.origin_top_left = true; break;
    case SG_BACKEND_METAL_MACOS: vig.origin_top_left = true; break;
    case SG_BACKEND_METAL_SIMULATOR: vig.origin_top_left = true; break;
    case SG_BACKEND_WGPU: vig.origin_top_left = true; break;
	case SG_BACKEND_DUMMY: vig.origin_top_left = true; break;
	}

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

void vi_free_targets()
{
	arena_free(vig.fb_arena);
	vig.fb_arena = NULL;
	memset(&vig.render_buffer, 0, sizeof(vig.render_buffer));
	memset(&vig.framebuffers, 0, sizeof(vig.framebuffers));
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

	uint8_t *vertex_ids = aalloc(NULL, uint8_t, fbx_mesh->num_vertices);

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
			for (size_t ti = 0; ti < num_tris; ti++) {
				uint8_t vert_ids[3] = { 0 };
				bool id_used[4] = { 0 };

				for (size_t ci = 0; ci < 3; ci++) {
					size_t index = tri_ix[ti * 3 + ci];
					uint32_t vertex = fbx_mesh->vertex_indices[index];
					uint8_t existing_id = vertex_ids[vertex];
					if (!id_used[existing_id]) {
						id_used[existing_id] = true;
						vert_ids[ci] = existing_id;
					}
				}

				// Assign unique vertex indices
				for (size_t ci = 0; ci < 3; ci++) {
					if (vert_ids[ci] == 0) {
						size_t index = tri_ix[ti * 3 + ci];
						uint32_t vertex = fbx_mesh->vertex_indices[index];
						uint8_t unused_id = 1;
						while (id_used[unused_id]) unused_id++;
						vert_ids[ci] = unused_id;
						id_used[unused_id] = true;
						vertex_ids[vertex] = unused_id;
					}
				}

				for (size_t ci = 0; ci < 3; ci++) {
					size_t index = tri_ix[ti * 3 + ci];
					ufbx_vec3 position = ufbx_get_vertex_vec3(&fbx_mesh->vertex_position, index);
					ufbx_vec3 normal = ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, index);
					vert->position = fbx_to_um_vec3(position);
					vert->normal = fbx_to_um_vec3(normal);
					vert->info[0] = vert_ids[ci];
					vert->info[1] = 0;
					vert->info[2] = 0;
					vert->info[3] = 0;
					vert++;
				}
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

	afree(NULL, vertex_ids);
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
	if (!scene) return;
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

	if (!vig.fb_arena) {
		vig.fb_arena = arena_create(&vig.arena);
	}

	arena_cancel(vig.fb_arena, fb->defer_pass, true);
	arena_cancel(vig.fb_arena, fb->defer_color, true);
	arena_cancel(vig.fb_arena, fb->defer_depth, true);
	fb->defer_pass = NULL;
	fb->defer_color = NULL;
	fb->defer_depth = NULL;

	uint32_t new_width = desc->width > fb->max_width ? desc->width : fb->max_width;
	uint32_t new_height = desc->height > fb->max_height ? desc->height : fb->max_height;
	fb->max_width = new_width;
	fb->max_height = new_height;

	fb->color_target = make_image(vig.fb_arena, &fb->defer_color, &(sg_image_desc){
		.width = (int)new_width,
		.height = (int)new_height,
		.sample_count = (int)desc->msaa,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.render_target = true,
		.mag_filter = SG_FILTER_LINEAR,
		.min_filter = SG_FILTER_LINEAR,
	});

	if (desc->has_depth) {
		fb->depth_target = make_image(vig.fb_arena, &fb->defer_depth, &(sg_image_desc){
			.width = (int)new_width,
			.height = (int)new_height,
			.sample_count = (int)desc->msaa,
			.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
			.render_target = true,
		});
	} else {
		fb->depth_target.id = 0;
	}

	fb->pass = make_pass(vig.fb_arena, &fb->defer_pass, &(sg_pass_desc){
		.color_attachments[0].image = fb->color_target,
		.depth_stencil_attachment.image = fb->depth_target,
	});
}

static um_mat vi_mat_perspective(float fov, float aspect, float near_z, float far_z)
{
	bool use_d3d = false;
	switch (vig.backend) {
	case SG_BACKEND_GLCORE33: use_d3d = false; break;
    case SG_BACKEND_GLES2: use_d3d = false; break;
    case SG_BACKEND_GLES3: use_d3d = false; break;
    case SG_BACKEND_D3D11: use_d3d = true; break;
    case SG_BACKEND_METAL_IOS: use_d3d = true; break;
    case SG_BACKEND_METAL_MACOS: use_d3d = true; break;
    case SG_BACKEND_METAL_SIMULATOR: use_d3d = true; break;
    case SG_BACKEND_WGPU: use_d3d = true; break;
	case SG_BACKEND_DUMMY: use_d3d = true; break;
	}

	if (use_d3d) {
		return um_mat_perspective_d3d(fov, aspect, near_z, far_z);
	} else {
		return um_mat_perspective_gl(fov, aspect, near_z, far_z);
	}
}

static um_vec3 hex_to_um3(uint32_t hex)
{
	return um_v3(
		(float)((hex>>16)&0xff)/255.0f,
		(float)((hex>> 8)&0xff)/255.0f,
		(float)((hex>> 0)&0xff)/255.0f);
}

static void vi_draw_meshes(vi_pipelines *ps, vi_scene *vs, const vi_desc *desc)
{
	for (size_t mesh_ix = 0; mesh_ix < vs->fbx.meshes.count; mesh_ix++) {
		ufbx_mesh *fbx_mesh = vs->fbx.meshes.data[mesh_ix];
		vi_mesh *mesh = &vs->meshes[mesh_ix];

		for (size_t inst_ix = 0; inst_ix < fbx_mesh->instances.count; inst_ix++) {
			ufbx_node *fbx_node = fbx_mesh->instances.data[inst_ix];
			vi_node *node = &vs->nodes[fbx_node->id];

			for (size_t part_ix = 0; part_ix < mesh->num_parts; part_ix++) {
				vi_part *part = &mesh->parts[part_ix];

				sg_apply_pipeline(ps->mesh_pipe);

				ufbx_material *fbx_material = NULL;
				if (part->material_id < vs->fbx.materials.count) {
					fbx_material = vs->fbx.materials.data[part->material_id];
				}

				um_vec3 highlight_color = um_zero3;
				float highlight = 0.0f;
				if (fbx_mesh->element_id == desc->selected_element_id) {
					highlight = 1.0f;
					highlight_color = hex_to_um3(0xf4bf6e);
				} else if (fbx_material && fbx_material->element_id == desc->selected_element_id) {
					highlight = 1.0f;
					highlight_color = hex_to_um3(0x6cdaa2);
				} else if (fbx_node->element_id == desc->selected_element_id) {
					highlight = 0.5f;
					highlight_color = hex_to_um3(0x6cb9da);
				}

				ubo_mesh_vertex_t vu = {
					.geometry_to_world = node->geometry_to_world,
					.world_to_clip = vs->world_to_clip,
					.highlight = highlight,
				};
				sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(vu));


				ubo_mesh_pixel_t pu = {
					.highlight_color = highlight_color,
				};
				sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(pu));

				sg_apply_bindings(&(sg_bindings){
					.vertex_buffers[0] = part->vertex_buffer,
					.index_buffer = part->index_buffer,
				});

				sg_draw(0, (int)part->num_indices, 1);
			}
		}
	}
}

static void sgl_v3(um_vec3 v)
{
	sgl_v3f(v.x, v.y, v.z);
}

static void vi_draw_debug(vi_pipelines *ps, vi_scene *vs, const vi_desc *desc)
{
	if (desc->selected_element_id < vs->fbx.elements.count) {
		ufbx_element *elem = vs->fbx.elements.data[desc->selected_element_id];

		if (elem->type == UFBX_ELEMENT_NODE) {
			ufbx_node *node = (ufbx_node*)elem;

			um_mat node_to_world = fbx_to_um_mat(node->node_to_world);
			um_vec3 c = node_to_world.cols[3].xyz;

			sgl_begin_lines();
			sgl_c3f(1.0f, 0.0f, 0.0f);
			for (size_t i = 0; i < 3; i++) {
				sgl_v3(c);
				sgl_v3(um_add3(c, node_to_world.cols[i].xyz));
			}
			sgl_end();
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

	sgl_set_context(ps->gl_context);

	sgl_matrix_mode_projection();
	sgl_load_matrix(vs->view_to_clip.m);
	sgl_matrix_mode_modelview();
	sgl_load_matrix(vs->world_to_view.m);

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

	sg_apply_viewport(0, 0, (int)render_fb->cur_width, (int)render_fb->cur_height, vig.origin_top_left);

	vi_draw_meshes(ps, vs, desc);
	vi_draw_debug(ps, vs, desc);

	sgl_draw();

	sg_end_pass();

	sg_begin_pass(dst_fb->pass, &(sg_pass_action){
		.colors[0].action = SG_ACTION_DONTCARE,
	});

	sg_apply_viewport(0, 0, (int)dst_fb->cur_width, (int)dst_fb->cur_height, vig.origin_top_left);
	vi_draw_postprocess(dst_fb->cur_width, dst_fb->cur_height, render_fb);

	sg_end_pass();

	sgl_set_context(sgl_default_context());
	sg_commit();
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
