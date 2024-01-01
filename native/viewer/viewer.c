#include "viewer.h"
#include "arena.h"
#include "resources.h"
#include "external/sokol_config.h"
#include "external/sokol_gfx.h"
#include "shaders/copy.h"
#include "shaders/mesh.h"
#include "shaders/debug.h"
#include "shaders/icon.h"

#if defined(SOKOL_GLES3) || defined(SOKOL_GLES2)
	#define HAS_GL 1
	#include <GLES3/gl3.h>
#else
	#define HAS_GL 0
#endif

#define MAX_DEBUG_VERTICES 2048
#define MAX_DEBUG_INDICES 4096

#define MAX_ICON_VERTICES 512
#define MAX_ICON_INDICES 1024

bool vi_initialized = false;

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

static size_t get_buffer_size(size_t size)
{
	size_t elements = (size + 15) / 16;
	size_t min_rows = (elements + 511) / 512;
	size_t rows = 1;
	while (rows < min_rows) rows *= 2;
	return rows * 512 * 16;
}

static sg_image make_static_buffer(arena_t *a, void **p_defer, const void *data, size_t size)
{
	size_t rows = size / (512 * 16);
	return make_image(a, p_defer, &(sg_image_desc){
		.width = 512,
		.height = (int)rows,
		.usage = SG_USAGE_IMMUTABLE,
		.pixel_format = SG_PIXELFORMAT_RGBA32F,
		.data.subimage[0][0] = { data, size },
	});
}

static sg_image make_dynamic_buffer(arena_t *a, void **p_defer, size_t size)
{
	size_t rows = size / (512 * 16);
	return make_image(a, p_defer, &(sg_image_desc){
		.width = 512,
		.height = (int)rows,
		.usage = SG_USAGE_DYNAMIC,
		.pixel_format = SG_PIXELFORMAT_RGBA32F,
	});
}

static void update_dynamic_buffer(sg_image buffer, const void *data, size_t size)
{
	sg_update_image(buffer, &(sg_image_data){
		.subimage[0][0] = { data, size },
	});
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
	uint8_t r, g, b, a;
} vi_color8;

static vi_color8 vi_rgb8(uint32_t hex) {
	return (vi_color8){ (hex>>16u)&0xff, (hex>>8u)&0xff, hex&0xff, 0xff };
}

static vi_color8 vi_rgba8(uint32_t hex) {
	return (vi_color8){ (hex>>24u)&0xff, (hex>>16u)&0xff, (hex>>8u)&0xff, hex&0xff };
}

typedef struct {
	um_vec4 position;
	vi_color8 color;
} vi_debug_vertex;

typedef struct {
	um_vec4 position;
	um_vec2 uv;
	uint8_t sdf_thresholds[4];
	vi_color8 color;
	vi_color8 outline_color;
} vi_icon_vertex;

typedef struct {
	um_vec3 position;
	um_vec3 normal;
	int32_t vertex_id;
} vi_vertex;

typedef struct {
	float f_num_bones;
	float f_bone_begin;
	float f_num_blends;
	float f_blend_begin;
} vi_deform_vertex;

typedef struct {
	float f_cluster_index;
	float weight;
} vi_deform_bone;

typedef struct {
	float f_keyframe_index;
	um_vec3 offset;
} vi_deform_blend;

typedef struct {
	float weight;
	float f_channel_id;
	float f_shape_id;
	float pad;
} vi_blend_keyframe_info;

typedef struct {
	um_mat geometry_to_bone;
	um_quat q0;
	um_quat qe;
	um_vec4 qs;
} vi_cluster_info;

typedef struct {
	uint32_t material_index;
	sg_buffer vertex_buffer;
	sg_buffer index_buffer;
	uint32_t num_indices;
	uint32_t num_vertices;
} vi_part;

typedef struct {
	um_vec3 base_color;
	um_vec3 emission_color;
} vi_material;

typedef struct {
	um_mat node_to_world;
	um_mat geometry_to_world;
} vi_node;

typedef struct {
	vi_part *parts;
	size_t num_parts;
	sg_image deform_buffer;
} vi_mesh;

typedef struct {
	size_t keyframe_offset;
} vi_blend_channel;

typedef struct {
	uint32_t x, y;
	uint32_t width, height;
} vi_rect;

struct vi_scene {
	arena_t *arena;
	ufbx_scene fbx;
	ufbx_scene *fbx_scene;
	ufbx_scene *fbx_state;
	void *fbx_state_defer;
	void *fbx_anim_defer;

	vi_node *nodes;
	vi_mesh *meshes;
	vi_material *materials;
	vi_blend_channel *blend_channels;

	size_t global_buffer_size;
	size_t global_cluster_offset;
	size_t global_keyframe_offset;
	sg_image global_buffer;
	void *global_buffer_cpu;
	vi_cluster_info *global_clusters;
	vi_blend_keyframe_info *global_keyframes;

	um_mat world_to_view;
	um_mat view_to_clip;
	um_mat world_to_clip;
	float pixel_scale;
	um_vec2 pixel_size;
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

	sg_pipeline debug_pipe;
	sg_pipeline debug_pipe_post;

	sg_pipeline icon_pipe;
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
	sg_shader debug_shader;
	sg_shader icon_shader;

	alist_t(vi_pipelines) pipelines;

	alist_t(vi_debug_vertex) debug_vertices;
	alist_t(uint32_t) debug_indices;

	alist_t(vi_icon_vertex) icon_vertices;
	alist_t(uint32_t) icon_indices;

	sg_buffer debug_vb;
	sg_buffer debug_ib;

	sg_buffer icon_vb;
	sg_buffer icon_ib;

	sg_image icon_atlas;
} vi_globals;

typedef enum {
	VI_ICON_NONE,
	VI_ICON_CAMERA,
	VI_ICON_LIGHT,
	VI_ICON_X,
	VI_ICON_Y,
	VI_ICON_Z,
} vi_icon;

static const vi_rect icons[] = {
	[VI_ICON_CAMERA] = { 0, 0, 64, 64 },
	[VI_ICON_LIGHT] = { 64, 0, 64, 64 },
	[VI_ICON_X] = { 192, 0, 32, 32 },
	[VI_ICON_Y] = { 224, 0, 32, 32 },
	[VI_ICON_Z] = { 192, 32, 32, 32 },
};

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
		.layout.attrs[2].format = SG_VERTEXFORMAT_UFBX_INT,
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
	});

	ps->debug_pipe = make_pipeline(&vig.arena, NULL, &(sg_pipeline_desc){
		.shader = vig.debug_shader,
		.depth.compare = SG_COMPAREFUNC_LESS_EQUAL,
		.sample_count = samples,
		.colors[0].pixel_format = color_format,
		.stencil.enabled = true,
		.stencil.front.compare = SG_COMPAREFUNC_EQUAL,
		.stencil.front.pass_op = SG_STENCILOP_INCR_CLAMP,
		.stencil.write_mask = 0xff,
		.stencil.read_mask = 0xff,
		.depth.pixel_format = depth_format,
		.index_type = SG_INDEXTYPE_UINT32,
		.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4,
		.layout.attrs[1].format = SG_VERTEXFORMAT_UBYTE4N,
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
	});

	ps->debug_pipe_post = make_pipeline(&vig.arena, NULL, &(sg_pipeline_desc){
		.shader = vig.debug_shader,
		.depth.compare = SG_COMPAREFUNC_GREATER,
		.stencil.enabled = true,
		.stencil.front.compare = SG_COMPAREFUNC_EQUAL,
		.stencil.front.pass_op = SG_STENCILOP_INCR_CLAMP,
		.stencil.write_mask = 0xff,
		.stencil.read_mask = 0xff,
		.sample_count = samples,
		.blend_color.a = 0.2f,
		.colors[0].pixel_format = color_format,
		.colors[0].blend.enabled = true,
		.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_BLEND_ALPHA,
		.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA,
		.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ZERO,
		.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE,
		.depth.pixel_format = depth_format,
		.index_type = SG_INDEXTYPE_UINT32,
		.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4,
		.layout.attrs[1].format = SG_VERTEXFORMAT_UBYTE4N,
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
	});

	ps->icon_pipe = make_pipeline(&vig.arena, NULL, &(sg_pipeline_desc){
		.shader = vig.icon_shader,
		.sample_count = samples,
		.colors[0].pixel_format = color_format,
		.colors[0].blend.enabled = true,
		.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
		.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ZERO,
		.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE,
		.depth.pixel_format = depth_format,
		.index_type = SG_INDEXTYPE_UINT32,
		.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4,
		.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2,
		.layout.attrs[2].format = SG_VERTEXFORMAT_UBYTE4N,
		.layout.attrs[3].format = SG_VERTEXFORMAT_UBYTE4N,
		.layout.attrs[4].format = SG_VERTEXFORMAT_UBYTE4N,
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

	vi_pipelines *ps = alist_push(&vig.arena, vi_pipelines, &vig.pipelines);
	memcpy(&ps->desc, desc, sizeof(vi_pipelines_desc));
	vi_init_pipelines(ps);
	return ps;
}

void vi_setup()
{
	if (vi_initialized) return;

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
	vig.debug_shader = make_shader(&vig.arena, NULL, debug_shader_desc(vig.backend));
	vig.icon_shader = make_shader(&vig.arena, NULL, icon_shader_desc(vig.backend));

	vig.debug_vb = make_buffer(&vig.arena, NULL, &(sg_buffer_desc){
		.type = SG_BUFFERTYPE_VERTEXBUFFER,
		.usage = SG_USAGE_STREAM,
		.size = sizeof(vi_debug_vertex) * MAX_DEBUG_VERTICES,
	});

	vig.debug_ib = make_buffer(&vig.arena, NULL, &(sg_buffer_desc){
		.type = SG_BUFFERTYPE_INDEXBUFFER,
		.usage = SG_USAGE_STREAM,
		.size = sizeof(uint32_t) * MAX_DEBUG_INDICES,
	});

	vig.icon_vb = make_buffer(&vig.arena, NULL, &(sg_buffer_desc){
		.type = SG_BUFFERTYPE_VERTEXBUFFER,
		.usage = SG_USAGE_STREAM,
		.size = sizeof(vi_icon_vertex) * MAX_ICON_VERTICES,
	});

	vig.icon_ib = make_buffer(&vig.arena, NULL, &(sg_buffer_desc){
		.type = SG_BUFFERTYPE_INDEXBUFFER,
		.usage = SG_USAGE_STREAM,
		.size = sizeof(uint32_t) * MAX_ICON_INDICES,
	});

	{
		size_t extent = 256;
		size_t num_pixels = extent * extent;
		uint8_t *icon_pixels = aalloc_uninit(NULL, uint8_t, num_pixels);

		ufbx_inflate_input input = {
			.data = icon_atlas,
			.data_size = icon_atlas_size,
			.total_size = icon_atlas_size,
		};

		ufbx_inflate_retain retain;
		retain.initialized = false;
		ufbx_inflate(icon_pixels, num_pixels, &input, &retain);

		uint8_t prev = 0;
		for (size_t i = 0; i < num_pixels; i++) {
			prev += icon_pixels[i];
			icon_pixels[i] = prev;
		}

		vig.icon_atlas = make_image(&vig.arena, NULL, &(sg_image_desc){
			.width = (int)extent,
			.height = (int)extent,
			.pixel_format = SG_PIXELFORMAT_R8,
			.data.subimage[0][0] = { icon_pixels, num_pixels },
			.mag_filter = SG_FILTER_LINEAR,
			.min_filter = SG_FILTER_LINEAR,
		});

		afree(NULL, icon_pixels);
	}

	vi_initialized = true;
}

void vi_shutdown()
{
	if (vi_initialized) {
		arena_free(&vig.arena);
		memset(&vig, 0, sizeof(vig));
		vi_initialized = false;
	}
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

	arena_t tmp;
	arena_init(&tmp, NULL);

	uint8_t *vertex_ids = aalloc(&tmp, uint8_t, fbx_mesh->num_vertices);

	vi_deform_vertex *d_verts = aalloc(&tmp, vi_deform_vertex, fbx_mesh->num_vertices);
	alist_t(vi_deform_bone) d_bones = { 0 };
	alist_t(vi_deform_blend) d_blends = { 0 };

	for (size_t di = 0; di < fbx_mesh->skin_deformers.count; di++) {
		ufbx_skin_deformer *deformer = fbx_mesh->skin_deformers.data[di];
		for (size_t vi = 0; vi < fbx_mesh->num_vertices; vi++) {
			ufbx_skin_vertex vert = deformer->vertices.data[vi];
			ufbx_skin_weight *weights = deformer->weights.data + vert.weight_begin;
			for (size_t i = 0; i < vert.num_weights; i++) {
				ufbx_skin_cluster *cluster = deformer->clusters.data[weights[i].cluster_index];
				vi_deform_bone *d_bone = alist_push(&tmp, vi_deform_bone, &d_bones);
				d_bone->f_cluster_index = (float)cluster->typed_id;
				d_bone->weight = (float)weights[i].weight;
			}
			// We need to keep bones paired for the GPU
			if (d_bones.count % 2 != 0) {
				vi_deform_bone *d_bone = alist_push(&tmp, vi_deform_bone, &d_bones);
				d_bone->f_cluster_index = d_bones.data[d_bones.count - 2].f_cluster_index;
				d_bone->weight = 0.0f;
			}


			float f_num = (float)((vert.num_weights + 1) / 2);
			if (d_verts[vi].f_num_bones == 0.0f) {
				float dq_weight = (float)vert.dq_weight;
				d_verts[vi].f_num_bones = f_num + um_min(um_max(dq_weight, 0.0f), 1.0f) * 0.5f;
			} else {
				d_verts[vi].f_num_bones += f_num;
			}
		}
	}

	for (size_t vi = 0; vi < fbx_mesh->num_vertices; vi++) {
		for (size_t di = 0; di < fbx_mesh->blend_deformers.count; di++) {
			ufbx_blend_deformer *deformer = fbx_mesh->blend_deformers.data[di];
			for (size_t ci = 0; ci < deformer->channels.count; ci++) {
				ufbx_blend_channel *channel = deformer->channels.data[ci];	
				for (size_t ki = 0; ki < channel->keyframes.count; ki++) {
					ufbx_blend_shape *shape = channel->keyframes.data[ki].shape;
					float f_keyframe_index = (float)(vs->blend_channels[channel->typed_id].keyframe_offset + ki);
					ufbx_vec3 offset = ufbx_get_blend_shape_vertex_offset(shape, vi);
					if (offset.x == 0.0f && offset.y == 0.0f && offset.z == 0.0f) continue;

					vi_deform_blend *d_blend = alist_push(&tmp, vi_deform_blend, &d_blends);
					d_blend->f_keyframe_index = f_keyframe_index;
					d_blend->offset = fbx_to_um_vec3(offset);
					d_verts[vi].f_num_blends += 1.0f;
				}
			}
		}
	}

	size_t deform_buf_size = 0;
	const size_t d_vertex_offset = 0;
	deform_buf_size += fbx_mesh->num_vertices * sizeof(vi_deform_vertex);
	const size_t d_bone_offset = deform_buf_size;
	deform_buf_size += d_bones.count * sizeof(vi_deform_bone);
	const size_t d_blend_offset = deform_buf_size;
	deform_buf_size += d_blends.count * sizeof(vi_deform_blend);
	assert(deform_buf_size % 16 == 0);
	deform_buf_size = get_buffer_size(deform_buf_size);
	char *deform_buf = aalloc(&tmp, char, deform_buf_size);

	size_t bone_ix = 0;
	size_t d_bone_pos = d_bone_offset;
	size_t d_blend_pos = d_blend_offset;
	for (size_t vi = 0; vi < fbx_mesh->num_vertices; vi++) {
		vi_deform_vertex *d_vert = &d_verts[vi];

		float total_weight = 0.0f;
		size_t num_bones = (size_t)d_vert->f_num_bones * 2;
		vi_deform_bone *vert_bones = d_bones.data + bone_ix;
		bone_ix += num_bones;
		for (size_t i = 0; i < num_bones; i++) {
			total_weight += vert_bones[i].weight;
		}
		if (total_weight <= 0.0f) {
			d_vert->f_num_bones = 0.0f;
		} else {
			for (size_t i = 0; i < num_bones; i++) {
				vert_bones[i].weight /= total_weight;
			}
		}

		d_vert->f_bone_begin = (float)(d_bone_pos / 16);
		d_vert->f_blend_begin = (float)(d_blend_pos / 16);
		d_bone_pos += num_bones * sizeof(vi_deform_bone);
		d_blend_pos += (size_t)d_vert->f_num_blends * sizeof(vi_deform_blend);
	}

	memcpy(deform_buf + d_vertex_offset, d_verts, fbx_mesh->num_vertices * sizeof(vi_deform_vertex));
	memcpy(deform_buf + d_bone_offset, d_bones.data, d_bones.count * sizeof(vi_deform_bone));
	memcpy(deform_buf + d_blend_offset, d_blends.data, d_blends.count * sizeof(vi_deform_blend));

	mesh->deform_buffer = make_static_buffer(vs->arena, NULL, deform_buf, deform_buf_size);

	size_t num_parts = 0;
	for (size_t pi = 0; pi < fbx_mesh->materials.count; pi++) {
		ufbx_mesh_part *fbx_mesh_part = &fbx_mesh->material_parts.data[pi];
		if (fbx_mesh_part->num_triangles == 0) continue;

		vi_part *part = &parts[num_parts++];

		part->material_index = (uint32_t)pi;

		arena_t tmp_inner;
		arena_init(&tmp_inner, NULL);
		
		size_t num_tri_ix = fbx_mesh->max_face_triangles * 3;
		uint32_t *tri_ix = aalloc_uninit(&tmp_inner, uint32_t, num_tri_ix);

		size_t num_indices = fbx_mesh_part->num_triangles * 3;
		vi_vertex *vertices = aalloc_uninit(&tmp_inner, vi_vertex, num_indices);
		uint32_t *indices = aalloc_uninit(&tmp_inner, uint32_t, num_indices);

		vi_vertex *vert = vertices;
		for (size_t fi = 0; fi < fbx_mesh_part->num_faces; fi++) {
			ufbx_face face = fbx_mesh->faces.data[fbx_mesh_part->face_indices.data[fi]];
			size_t num_tris = ufbx_triangulate_face(tri_ix, num_tri_ix, fbx_mesh, face);
			for (size_t ti = 0; ti < num_tris; ti++) {
				uint8_t vert_ids[3] = { 0 };
				bool id_used[4] = { 0 };

				for (size_t ci = 0; ci < 3; ci++) {
					size_t index = tri_ix[ti * 3 + ci];
					uint32_t vertex = fbx_mesh->vertex_indices.data[index];
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
						uint32_t vertex = fbx_mesh->vertex_indices.data[index];
						uint8_t unused_id = 1;
						while (id_used[unused_id]) unused_id++;
						vert_ids[ci] = unused_id;
						id_used[unused_id] = true;
						vertex_ids[vertex] = unused_id;
					}
				}

				for (size_t ci = 0; ci < 3; ci++) {
					size_t index = tri_ix[ti * 3 + ci];
					int32_t vertex_id = fbx_mesh->vertex_indices.data[index];
					ufbx_vec3 position = ufbx_get_vertex_vec3(&fbx_mesh->vertex_position, index);
					ufbx_vec3 normal = ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, index);
					vert->position = fbx_to_um_vec3(position);
					vert->normal = fbx_to_um_vec3(normal);
					vert->vertex_id = vert_ids[ci] | vertex_id << 2;
					vert++;
				}
			}
		}

		ufbx_vertex_stream streams[] = { vertices, sizeof(vi_vertex) };
		size_t num_vertices = ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

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

		arena_free(&tmp_inner);
	}

	arena_free(&tmp);
	mesh->num_parts = num_parts;
}

static void vi_init_material(vi_scene *vs, vi_material *material, ufbx_material *fbx_material)
{
	material->base_color = um_mul3(
		fbx_to_um_vec3(fbx_material->pbr.base_color.value_vec3),
		(float)fbx_material->pbr.base_factor.value_real);
	material->emission_color = um_mul3(
		fbx_to_um_vec3(fbx_material->pbr.emission_color.value_vec3),
		(float)fbx_material->pbr.emission_factor.value_real);
}

void vi_init_globals(vi_scene *vs)
{
	size_t num_blend_keyframes = 0;
	for (size_t i = 0; i < vs->fbx.blend_channels.count; i++) {
		ufbx_blend_channel *fbx_channel = vs->fbx.blend_channels.data[i];
		vs->blend_channels[i].keyframe_offset = num_blend_keyframes;
		num_blend_keyframes += fbx_channel->keyframes.count;
	}

	size_t global_buffer_size = 0;
	vs->global_cluster_offset = global_buffer_size;
	global_buffer_size += vs->fbx.skin_clusters.count * sizeof(vi_cluster_info);
	vs->global_keyframe_offset = global_buffer_size;
	global_buffer_size += num_blend_keyframes * sizeof(vi_blend_keyframe_info);
	global_buffer_size = get_buffer_size(global_buffer_size);
	vs->global_buffer_size = global_buffer_size;
	vs->global_buffer_cpu = aalloc(vs->arena, char, vs->global_buffer_size);
	vs->global_clusters = (vi_cluster_info*)((char*)vs->global_buffer_cpu + vs->global_cluster_offset);
	vs->global_keyframes = (vi_blend_keyframe_info*)((char*)vs->global_buffer_cpu + vs->global_keyframe_offset);
	vs->global_buffer = make_dynamic_buffer(vs->arena, NULL, global_buffer_size);
}

static void vi_update_globals(vi_scene *vs, const ufbx_scene *fbx_scene)
{
	for (size_t chan_ix = 0; chan_ix < vs->fbx.blend_channels.count; chan_ix++) {
		ufbx_blend_channel *channel = fbx_scene->blend_channels.data[chan_ix];
		vi_blend_keyframe_info *infos = vs->global_keyframes + vs->blend_channels[chan_ix].keyframe_offset;
		for (size_t i = 0; i < channel->keyframes.count; i++) {
			ufbx_blend_keyframe key = channel->keyframes.data[i];
			infos[i].weight = (float)key.effective_weight;
			infos[i].f_channel_id = (float)channel->typed_id;
			infos[i].f_shape_id = (float)key.shape->typed_id;
			infos[i].pad = 0.0f;
		}
	}

	for (size_t cluster_ix = 0; cluster_ix < vs->fbx.skin_clusters.count; cluster_ix++) {
		ufbx_skin_cluster *cluster = fbx_scene->skin_clusters.data[cluster_ix];
		vi_cluster_info *info = &vs->global_clusters[cluster_ix];
		info->geometry_to_bone = fbx_to_um_mat(cluster->geometry_to_world);

		um_quat q0, qe;
		q0 = fbx_to_um_quat(cluster->geometry_to_world_transform.rotation);
		qe.xyz = um_mul3(fbx_to_um_vec3(cluster->geometry_to_world_transform.translation), 0.5f);
		qe.w = 0.0f;
		qe = um_quat_mul(qe, q0);

		info->q0 = q0;
		info->qe = qe;
		info->qs.xyz = fbx_to_um_vec3(cluster->geometry_to_world_transform.scale);
		info->qs.w = 0.0f;
	}

	update_dynamic_buffer(vs->global_buffer, vs->global_buffer_cpu, vs->global_buffer_size);
}

vi_scene *vi_make_scene(ufbx_scene *fbx_scene)
{
	arena_t *arena = arena_create(&vig.arena);
	vi_scene *vs = aalloc(arena, vi_scene, 1);
	vs->arena = arena;
	if (!vs) return NULL;

	vs->fbx_scene = fbx_scene;
	vs->fbx = *fbx_scene;

	vs->meshes = aalloc(vs->arena, vi_mesh, fbx_scene->meshes.count);
	vs->nodes = aalloc(vs->arena, vi_node, fbx_scene->nodes.count);
	vs->materials = aalloc(vs->arena, vi_material, fbx_scene->materials.count + 1); // + NULL
	vs->blend_channels = aalloc(vs->arena, vi_blend_channel, fbx_scene->blend_channels.count);

	vi_init_globals(vs);

	for (size_t i = 0; i < vs->fbx.nodes.count; i++) {
		vi_init_node(vs, &vs->nodes[i], vs->fbx.nodes.data[i]);
	}

	for (size_t i = 0; i < vs->fbx.meshes.count; i++) {
		vi_init_mesh(vs, &vs->meshes[i], vs->fbx.meshes.data[i]);
	}

	for (size_t i = 0; i < vs->fbx.materials.count; i++) {
		vi_init_material(vs, &vs->materials[i], vs->fbx.materials.data[i]);
	}

	// NULL material
	{
		vi_material *mat = &vs->materials[vs->fbx.materials.count];
		mat->base_color = um_dup3(0.8f);
		mat->emission_color = um_dup3(0.0f);
	}

	vi_update_globals(vs, fbx_scene);

	sg_commit();

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
	ufbx_element *selected_element = NULL;
	if (desc->selected_element_id < vs->fbx.elements.count) {
		selected_element = vs->fbx.elements.data[desc->selected_element_id];
	}
	for (size_t mesh_ix = 0; mesh_ix < vs->fbx.meshes.count; mesh_ix++) {
		ufbx_mesh *fbx_mesh = vs->fbx.meshes.data[mesh_ix];
		vi_mesh *mesh = &vs->meshes[mesh_ix];

		for (size_t inst_ix = 0; inst_ix < fbx_mesh->instances.count; inst_ix++) {
			ufbx_node *fbx_node = fbx_mesh->instances.data[inst_ix];
			vi_node *node = &vs->nodes[fbx_node->typed_id];

			for (size_t part_ix = 0; part_ix < mesh->num_parts; part_ix++) {
				vi_part *part = &mesh->parts[part_ix];

				sg_apply_pipeline(ps->mesh_pipe);

				ufbx_material *fbx_material = NULL;
				if (part->material_index < fbx_node->materials.count) {
					fbx_material = fbx_node->materials.data[part->material_index];
				}

				// Use NULL material by default
				vi_material *material = &vs->materials[vs->fbx.materials.count];
				if (fbx_material) {
					material = &vs->materials[fbx_material->typed_id];
				}

				int highlight_cluster = -1;
				int highlight_channel = -1;
				int highlight_shape = -1;

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

				if (selected_element && selected_element->type == UFBX_ELEMENT_SKIN_CLUSTER) {
					highlight_cluster = selected_element->typed_id;
					highlight_color = hex_to_um3(0xdf91e8);
				} else if (selected_element && selected_element->type == UFBX_ELEMENT_BLEND_CHANNEL) {
					highlight_channel = selected_element->typed_id;
					highlight_color = hex_to_um3(0xdf91e8);
				} else if (selected_element && selected_element->type == UFBX_ELEMENT_BLEND_SHAPE) {
					highlight_shape = selected_element->typed_id;
					highlight_color = hex_to_um3(0xdf91e8);
				}

				for (size_t i = 0; i < fbx_mesh->all_deformers.count; i++) {
					if (fbx_mesh->all_deformers.data[i]->element_id == desc->selected_element_id) {
						highlight = 1.0f;
						highlight_color = hex_to_um3(0xdf91e8);
					}
				}

				ubo_mesh_vertex_t vu = {
					.u_geometry_to_world = node->geometry_to_world,
					.u_world_to_clip = vs->world_to_clip,
					.u_highlight = highlight,
					.ui_highlight_cluster = (float)highlight_cluster,
					.ui_highlight_channel = (float)highlight_channel,
					.ui_highlight_shape = (float)highlight_shape,
					.ui_g_cluster_begin = (float)(vs->global_cluster_offset / 16),
					.ui_g_keyframe_begin = (float)(vs->global_keyframe_offset / 16),
				};
				sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(vu));

				ubo_mesh_pixel_t pu = {
					.highlight_color = highlight_color,
					.pixel_scale = vs->pixel_scale,
					.base_color = material->base_color,
					.emission_color = material->emission_color,
				};
				sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(pu));

				sg_apply_bindings(&(sg_bindings){
					.vs_images[SLOT_u_deform_buffer] = mesh->deform_buffer,
					.vs_images[SLOT_u_global_buffer] = vs->global_buffer,
					.vertex_buffers[0] = part->vertex_buffer,
					.index_buffer = part->index_buffer,
				});

				sg_draw(0, (int)part->num_indices, 1);
			}
		}
	}
}

static void gl_draw_line_4d(vi_scene *vs, um_vec4 a, um_vec4 b, float width, vi_color8 color, bool top)
{
	um_vec2 delta = um_sub2(um_div2(b.xy, b.w), um_div2(a.xy, a.w));
	um_vec2 nx = um_normalize2(delta);
	if (um_equal2(nx, um_zero2)) nx = um_v2(1.0f, 0.0f);
	um_vec2 ny = um_v2(nx.y, -nx.x);

	um_vec2 pixel_size = um_mul2(vs->pixel_size, width);
	nx = um_mulv2(nx, pixel_size);
	ny = um_mulv2(ny, pixel_size);

	static const um_vec2 envelope[] = {
		{ -0.93969262078f, 0.34202014332f },
		{ -0.64278760968f, 0.76604444311f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.64278760968f, 0.76604444311f },
		{ 1.93969262078f, 0.34202014332f },
	};
	const size_t num_segments = sizeof(envelope)/sizeof(*envelope);
	uint32_t base = (uint32_t)vig.debug_vertices.count;

	vi_debug_vertex *verts = alist_push_n(NULL, vi_debug_vertex, &vig.debug_vertices, num_segments * 2);
	uint32_t *indices = alist_push_n(NULL, uint32_t, &vig.debug_indices, (num_segments-1)*6);

	for (size_t i = 0; i < num_segments; i++) {
		um_vec2 e = envelope[i];
		um_vec4 v;
		float w;
		if (e.x < 0.5f) {
			w = a.w;
			v.xy = um_mad2(a.xy, nx, e.x * w);
			v.z = a.z;
			v.w = a.w;
		} else {
			w = b.w;
			v.xy = um_mad2(b.xy, nx, (e.x - 1.0f) * w);
			v.z = b.z;
			v.w = b.w;
		}
		um_vec4 va = v, vb = v;
		va.xy = um_mad2(va.xy, ny, e.y * +w);
		vb.xy = um_mad2(vb.xy, ny, e.y * -w);

		if (top) {
			va.z = 0.0f;
			vb.z = 0.0f;
		}

		verts[i*2+0].position = va;
		verts[i*2+1].position = vb;
		verts[i*2+0].color = color;
		verts[i*2+1].color = color;
	}
	for (uint32_t i = 0; i < num_segments - 1; i++) {
		uint32_t b = base + i * 2;
		indices[i*6+0] = b + 0;
		indices[i*6+1] = b + 2;
		indices[i*6+2] = b + 1;
		indices[i*6+3] = b + 2;
		indices[i*6+4] = b + 3;
		indices[i*6+5] = b + 1;
	}
}

static void gl_draw_line_3d(vi_scene *vs, um_vec3 a, um_vec3 b, float width, vi_color8 color, bool top)
{
	um_vec4 a4, b4;
	a4.xyz = a; a4.w = 1.0f;
	b4.xyz = b; b4.w = 1.0f;
	um_vec4 va = um_mat_mulr(vs->world_to_clip, a4);
	um_vec4 vb = um_mat_mulr(vs->world_to_clip, b4);
	gl_draw_line_4d(vs, va, vb, width, color, top);
}

static void gl_draw_icon_4d(vi_scene *vs, vi_rect rect, um_vec4 pos, float extent, vi_color8 color, float outline_width, vi_color8 outline_color)
{
	uint32_t base = (uint32_t)vig.icon_vertices.count;

	vi_icon_vertex *verts = alist_push_n(NULL, vi_icon_vertex, &vig.icon_vertices, 4);
	uint32_t *indices = alist_push_n(NULL, uint32_t, &vig.icon_indices, 6);

	um_vec2 uv_base = um_mul2(um_v2((float)rect.x, (float)rect.y), 1.0f / 256.0f);
	um_vec2 uv_size = um_mul2(um_v2((float)rect.width, (float)rect.height), 1.0f / 256.0f);
	float uv_pixels = um_min((float)rect.width, (float)rect.height);

	um_vec2 size = um_mul2(vs->pixel_size, extent);

	float pixel_distance = um_min((8.0f * uv_pixels) / extent / vs->pixel_scale, 32.0f);

	if (outline_width == 0.0f) {
		outline_color = color;
	} else {
		outline_width = outline_width * (8.0f * uv_pixels) / extent;
	}

	float th_a = 128.0f;
	float th_b = th_a + pixel_distance;
	float th_c = th_a + um_min(um_max(outline_width, pixel_distance), 48.0f);
	float th_d = th_c + pixel_distance;

	vi_icon_vertex vertex;
	vertex.position = pos;
	vertex.uv = uv_base;
	vertex.color = color;
	vertex.outline_color = outline_color;
	vertex.sdf_thresholds[0] = (uint8_t)th_a;
	vertex.sdf_thresholds[1] = (uint8_t)th_b;
	vertex.sdf_thresholds[2] = (uint8_t)th_c;
	vertex.sdf_thresholds[3] = (uint8_t)th_d;

	verts[0] = vertex;
	verts[0].position.x -= size.x * pos.w;
	verts[0].position.y += size.y * pos.w;

	verts[1] = vertex;
	verts[1].position.x += size.x * pos.w;
	verts[1].position.y += size.y * pos.w;
	verts[1].uv.x += uv_size.x;

	verts[2] = vertex;
	verts[2].position.x -= size.x * pos.w;
	verts[2].position.y -= size.y * pos.w;
	verts[2].uv.y += uv_size.y;

	verts[3] = vertex;
	verts[3].position.x += size.x * pos.w;
	verts[3].position.y -= size.y * pos.w;
	verts[3].uv.x += uv_size.x;
	verts[3].uv.y += uv_size.y;

	indices[0] = base + 0;
	indices[1] = base + 2;
	indices[2] = base + 1;
	indices[3] = base + 2;
	indices[4] = base + 3;
	indices[5] = base + 1;
}

static void vi_draw_widgets(vi_pipelines *ps, vi_scene *vs, const vi_desc *desc)
{
	float widget_scale = desc->widget_scale;

	if (desc->selected_element_id < vs->fbx.elements.count) {
		ufbx_element *fbx_elem = vs->fbx_state->elements.data[desc->selected_element_id];

		if (fbx_elem->type == UFBX_ELEMENT_NODE) {
			vi_node *node = &vs->nodes[fbx_elem->typed_id];

			um_mat node_to_world = node->node_to_world;
			um_vec3 c = node_to_world.cols[3].xyz;

			for (size_t i = 0; i < 3; i++) {
				um_vec3 cx = um_mad3(c, node_to_world.cols[i].xyz, widget_scale);
				gl_draw_line_3d(vs, c, cx, 5.0f, vi_rgb8(0x6cb9da), true);
			}

			for (size_t i = 0; i < 3; i++) {
				um_vec3 cx = um_mad3(c, node_to_world.cols[i].xyz, widget_scale);
				gl_draw_line_3d(vs, c, cx, 10.0f, vi_rgb8(0x0e2a36), true);

				um_vec4 c4;
				c4.xyz = um_mad3(c, node_to_world.cols[i].xyz, widget_scale * 1.1f);
				c4.w = 1.0f;
				c4 = um_mat_mulr(vs->world_to_clip, c4);
				gl_draw_icon_4d(vs, icons[VI_ICON_X + i], c4, 30.0f, vi_rgb8(0x6cb9da), 2.5f, vi_rgb8(0x0e2a36));
			}
		} if (fbx_elem->type == UFBX_ELEMENT_MESH) {
			ufbx_mesh *fbx_mesh = (ufbx_mesh*)fbx_elem;

   			uint32_t highlight_index = desc->highlight_vertex_index;
   			uint32_t highlight_face = desc->highlight_face_index;
			if (highlight_face < fbx_mesh->num_faces) {
				ufbx_face face = fbx_mesh->faces.data[highlight_face];

				for (size_t inst_ix = 0; inst_ix < fbx_mesh->instances.count; inst_ix++) {
					ufbx_node *fbx_node = fbx_mesh->instances.data[inst_ix];

					um_vec3 positions[32];

					for (size_t i = 0; i < face.num_indices; i++) {
						size_t index = face.index_begin + i;
						uint32_t vertex = fbx_mesh->vertex_indices.data[index];

						ufbx_matrix geometry_to_world = fbx_node->geometry_to_world;

						um_vec3 pos = fbx_to_um_vec3(fbx_mesh->vertices.data[vertex]);

						if (fbx_mesh->blend_deformers.count > 0) {
							ufbx_blend_deformer *blend = fbx_mesh->blend_deformers.data[0];
							pos = um_add3(pos, fbx_to_um_vec3(ufbx_get_blend_vertex_offset(blend, vertex)));
						}

						if (fbx_mesh->skin_deformers.count > 0) {
							ufbx_skin_deformer *skin = fbx_mesh->skin_deformers.data[0];
							geometry_to_world = ufbx_get_skin_vertex_matrix(skin, vertex, &geometry_to_world);
						}

						um_mat mat = fbx_to_um_mat(geometry_to_world);
						pos = um_transform_point(&mat, pos);
						if (i < 32) {
							positions[i] = pos;
						}

						if (index == highlight_index) {
							if (fbx_mesh->vertex_normal.exists) {
								um_mat normal_mat = um_mat_transpose(um_mat_inverse(mat));

								um_vec3 normal = fbx_to_um_vec3(ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, index));
								normal = um_normalize3(um_transform_direction(&normal_mat, normal));
								// TODO: Hardcoded length
								gl_draw_line_3d(vs, pos, um_mad3(pos, normal, 0.5f), 4.0f, vi_rgb8(0xffffff), false);
							}

							gl_draw_line_3d(vs, pos, pos, 8.0f, vi_rgb8(0xffffff), false);
						}
					}

					if (face.num_indices < 32) {
						for (uint32_t ai = 0; ai < face.num_indices; ai++) {
							uint32_t bi = (ai + 1) % face.num_indices;
							gl_draw_line_3d(vs, positions[ai], positions[bi], 4.0f, vi_rgb8(0xcdcde0), false);
						}
					}
				}
			}
		}
	}

	for (size_t i = 0; i < vs->fbx.elements.count; i++) {
		ufbx_element *fbx_element = vs->fbx_state->elements.data[i];

		vi_icon icon = VI_ICON_NONE;
		if (fbx_element->type == UFBX_ELEMENT_CAMERA) {
			icon = VI_ICON_CAMERA;
		} else if (fbx_element->type == UFBX_ELEMENT_LIGHT) {
			icon = VI_ICON_LIGHT;
		}

		if (icon == VI_ICON_NONE) continue;

		for (size_t inst_ix = 0; inst_ix < fbx_element->instances.count; inst_ix++) {
			ufbx_node *fbx_node = fbx_element->instances.data[inst_ix];
			vi_node *node = &vs->nodes[fbx_node->typed_id];

			um_mat node_to_world = node->node_to_world;
			um_vec3 c = node_to_world.cols[3].xyz;

			vi_color8 color, outline_color;
			if (desc->selected_element_id == fbx_element->element_id) {
				color = vi_rgb8(0xf4bf6e);
				outline_color = vi_rgb8(0x342e25);
			} else if (desc->selected_element_id == fbx_node->element_id) {
				color = vi_rgb8(0x6cb9da);
				outline_color = vi_rgb8(0x342e25);
			} else {
				color = vi_rgba8(0xdddddd88);
				outline_color = vi_rgba8(0x2c2c2c);
			}

			um_vec4 c4;
			c4.xyz = c;
			c4.w = 1.0f;
			c4 = um_mat_mulr(vs->world_to_clip, c4);
			gl_draw_icon_4d(vs, icons[icon], c4, 50.0f, color, 2.5f, outline_color);
		}
	}

	for (size_t i = 0; i < vs->fbx.cameras.count; i++) {
		ufbx_camera *fbx_camera = vs->fbx_state->cameras.data[i];
		for (size_t inst_ix = 0; inst_ix < fbx_camera->instances.count; inst_ix++) {
			ufbx_node *fbx_node = fbx_camera->instances.data[inst_ix];
			vi_node *node = &vs->nodes[fbx_node->typed_id];

			float near = 0.5f * widget_scale;
			float far = 2.0f * widget_scale;

			float dx = (float)fbx_camera->field_of_view_tan.x;
			float dy = (float)fbx_camera->field_of_view_tan.y;

			um_mat mat = um_mat_mulrev(node->node_to_world, vs->world_to_clip);

			vi_color8 color, outline_color = { 0 };
			if (desc->selected_element_id == fbx_camera->element_id) {
				color = vi_rgb8(0xf4bf6e);
				outline_color = vi_rgb8(0x342e25);
			} else if (desc->selected_element_id == fbx_node->element_id) {
				color = vi_rgb8(0x73919e);
			} else {
				color = vi_rgb8(0x888888);
			}

			um_vec4 n00 = um_mat_mulr(mat, um_v4(near, -dy*near, -dx*near, 1.0f));
			um_vec4 n01 = um_mat_mulr(mat, um_v4(near, +dy*near, -dx*near, 1.0f));
			um_vec4 n10 = um_mat_mulr(mat, um_v4(near, -dy*near, +dx*near, 1.0f));
			um_vec4 n11 = um_mat_mulr(mat, um_v4(near, +dy*near, +dx*near, 1.0f));
			um_vec4 f00 = um_mat_mulr(mat, um_v4(far, -dy*far, -dx*far, 1.0f));
			um_vec4 f01 = um_mat_mulr(mat, um_v4(far, +dy*far, -dx*far, 1.0f));
			um_vec4 f10 = um_mat_mulr(mat, um_v4(far, -dy*far, +dx*far, 1.0f));
			um_vec4 f11 = um_mat_mulr(mat, um_v4(far, +dy*far, +dx*far, 1.0f));

			bool top = false;

			{
				float width = 5.0f;

				gl_draw_line_4d(vs, n00, n01, width, color, top);
				gl_draw_line_4d(vs, n01, n11, width, color, top);
				gl_draw_line_4d(vs, n11, n10, width, color, top);
				gl_draw_line_4d(vs, n10, n00, width, color, top);

				gl_draw_line_4d(vs, f00, f01, width, color, top);
				gl_draw_line_4d(vs, f01, f11, width, color, top);
				gl_draw_line_4d(vs, f11, f10, width, color, top);
				gl_draw_line_4d(vs, f10, f00, width, color, top);

				gl_draw_line_4d(vs, n00, f00, width, color, top);
				gl_draw_line_4d(vs, n01, f01, width, color, top);
				gl_draw_line_4d(vs, n11, f11, width, color, top);
				gl_draw_line_4d(vs, n10, f10, width, color, top);
			}

			if (outline_color.r) {
				float width = 10.0f;

				gl_draw_line_4d(vs, n00, n01, width, outline_color, top);
				gl_draw_line_4d(vs, n01, n11, width, outline_color, top);
				gl_draw_line_4d(vs, n11, n10, width, outline_color, top);
				gl_draw_line_4d(vs, n10, n00, width, outline_color, top);

				gl_draw_line_4d(vs, f00, f01, width, outline_color, top);
				gl_draw_line_4d(vs, f01, f11, width, outline_color, top);
				gl_draw_line_4d(vs, f11, f10, width, outline_color, top);
				gl_draw_line_4d(vs, f10, f00, width, outline_color, top);

				gl_draw_line_4d(vs, n00, f00, width, outline_color, top);
				gl_draw_line_4d(vs, n01, f01, width, outline_color, top);
				gl_draw_line_4d(vs, n11, f11, width, outline_color, top);
				gl_draw_line_4d(vs, n10, f10, width, outline_color, top);
			}
		}
	}

	for (size_t i = 0; i < vs->fbx.lights.count; i++) {
		ufbx_light *fbx_light = vs->fbx_state->lights.data[i];
		for (size_t inst_ix = 0; inst_ix < fbx_light->instances.count; inst_ix++) {
			ufbx_node *fbx_node = fbx_light->instances.data[inst_ix];
			vi_node *node = &vs->nodes[fbx_node->typed_id];

			if (fbx_light->type == UFBX_LIGHT_DIRECTIONAL) {
				um_mat node_to_world = node->node_to_world;
				um_vec3 c = node_to_world.cols[3].xyz;

				vi_color8 color, outline_color = { 0 };
				if (desc->selected_element_id == fbx_light->element_id) {
					color = vi_rgb8(0xf4bf6e);
					outline_color = vi_rgb8(0x342e25);
				} else if (desc->selected_element_id == fbx_node->element_id) {
					color = vi_rgb8(0x73919e);
				} else {
					color = vi_rgb8(0x888888);
				}

				um_vec3 cx = um_mad3(c, node_to_world.cols[1].xyz, -widget_scale * 3.0f);
				gl_draw_line_3d(vs, c, cx, 5.0f, color, false);

				if (outline_color.r) {
					gl_draw_line_3d(vs, c, cx, 10.0f, outline_color, false);
				}
			}
		}
	}

}

static void vi_draw_debug(vi_pipelines *ps, vi_scene *vs, const vi_desc *desc)
{
	if (vig.debug_vertices.count > 0) {

		sg_update_buffer(vig.debug_vb, &(sg_range){ vig.debug_vertices.data, vig.debug_vertices.count * sizeof(vi_debug_vertex) });
		sg_update_buffer(vig.debug_ib, &(sg_range){ vig.debug_indices.data, vig.debug_indices.count * sizeof(uint32_t) });

		sg_apply_pipeline(ps->debug_pipe);
		sg_apply_bindings(&(sg_bindings){
			.vertex_buffers[0] = vig.debug_vb,
			.index_buffer = vig.debug_ib,
		});

		sg_draw(0, (int)vig.debug_indices.count, 1);

		sg_apply_pipeline(ps->debug_pipe_post);
		sg_apply_bindings(&(sg_bindings){
			.vertex_buffers[0] = vig.debug_vb,
			.index_buffer = vig.debug_ib,
		});

		sg_draw(0, (int)vig.debug_indices.count, 1);

		vig.debug_vertices.count = 0;
		vig.debug_indices.count = 0;
	}

	if (vig.icon_vertices.count > 0) {

		sg_update_buffer(vig.icon_vb, &(sg_range){ vig.icon_vertices.data, vig.icon_vertices.count * sizeof(vi_icon_vertex) });
		sg_update_buffer(vig.icon_ib, &(sg_range){ vig.icon_indices.data, vig.icon_indices.count * sizeof(uint32_t) });

		sg_apply_pipeline(ps->icon_pipe);
		sg_apply_bindings(&(sg_bindings){
			.vertex_buffers[0] = vig.icon_vb,
			.index_buffer = vig.icon_ib,
			.fs_images[SLOT_icon_atlas] = vig.icon_atlas,
		});

		sg_draw(0, (int)vig.icon_indices.count, 1);

		vig.icon_vertices.count = 0;
		vig.icon_indices.count = 0;
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

static void ad_free_ufbx_scene(void *user) { ufbx_free_scene(*(ufbx_scene**)user); }
static void ad_free_ufbx_anim(void *user) { ufbx_free_anim(*(ufbx_anim**)user); }

static void vi_update(vi_scene *vs, const vi_target *target, const vi_desc *desc)
{
	float aspect = (float)target->width / (float)target->height;

	ufbx_anim *anim = vs->fbx.anim;

	// TODO: Cache
	if (vs->fbx_state) {
		arena_cancel(vs->arena, vs->fbx_state_defer, true);
		arena_cancel(vs->arena, vs->fbx_anim_defer, true);
		vs->fbx_state_defer = NULL;
		vs->fbx_anim_defer = NULL;
	}

	if (desc->num_overrides > 0) {
		ufbx_anim_opts opts = { 0 };

		opts.prop_overrides.data = desc->overrides;
		opts.prop_overrides.count = desc->num_overrides;

		arena_t tmp;
		arena_init(&tmp, NULL);
		uint32_t *layers = aalloc(&tmp, uint32_t, anim->layers.count);
		for (size_t i = 0; i < anim->layers.count; i++) {
			layers[i] = anim->layers.data[i]->typed_id;
		}

		anim = ufbx_create_anim(vs->fbx_scene, &opts, NULL);
		arena_free(&tmp);

		vs->fbx_anim_defer = arena_defer(vs->arena, ad_free_ufbx_anim, ufbx_anim*, &anim);
	}
	ufbx_scene *fbx_state = ufbx_evaluate_scene(vs->fbx_scene, anim, desc->time, NULL, NULL);
	vs->fbx_state = fbx_state;
	vs->fbx_state_defer = arena_defer(vs->arena, ad_free_ufbx_scene, ufbx_scene*, &vs->fbx_state);

	vs->world_to_view = um_mat_look_at(desc->camera_pos, desc->camera_target, um_v3(0,1,0));
	vs->view_to_clip = vi_mat_perspective(desc->field_of_view * UM_DEG_TO_RAD, aspect, desc->near_plane, desc->far_plane);
	vs->world_to_clip = um_mat_mulrev(vs->world_to_view, vs->view_to_clip);
	vs->pixel_scale = target->pixel_scale;
	vs->pixel_size.x = 1.0f / (float)target->width * target->pixel_scale;
	vs->pixel_size.y = 1.0f / (float)target->height * target->pixel_scale;

	for (size_t i = 0; i < vs->fbx.nodes.count; i++) {
		ufbx_node *fbx_node = fbx_state->nodes.data[i];
		vi_node *node = &vs->nodes[i];
		node->node_to_world = fbx_to_um_mat(fbx_node->node_to_world);
		node->geometry_to_world = fbx_to_um_mat(fbx_node->geometry_to_world);
	}

	vi_update_globals(vs, fbx_state);
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
		.colors[0].value = { 0.5f, 0.5f, 0.6f, 1.0f },
		.depth.action = SG_ACTION_CLEAR,
		.depth.value = 1.0f,
	});

	sg_apply_viewport(0, 0, (int)render_fb->cur_width, (int)render_fb->cur_height, vig.origin_top_left);

	vi_draw_meshes(ps, vs, desc);
	vi_draw_widgets(ps, vs, desc);
	vi_draw_debug(ps, vs, desc);

	sg_end_pass();

	sg_begin_pass(dst_fb->pass, &(sg_pass_action){
		.colors[0].action = SG_ACTION_DONTCARE,
	});

	sg_apply_viewport(0, 0, (int)dst_fb->cur_width, (int)dst_fb->cur_height, vig.origin_top_left);
	vi_draw_postprocess(dst_fb->cur_width, dst_fb->cur_height, render_fb);

	sg_end_pass();

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
