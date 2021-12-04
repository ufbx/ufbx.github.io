#include "viewer.h"
#include "arena.h"
#include "external/sokol_config.h"
#include "external/sokol_gfx.h"
#include "shaders/copy.h"

static um_vec2 fbx_to_um_vec2(ufbx_vec2 v) { return um_v2((float)v.x, (float)v.y); }
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

static void defer_destroy_image(arena_t *a, sg_image image) { arena_defer(a, &ad_sg_destroy_image, sg_image, &image); }
static void defer_destroy_buffer(arena_t *a, sg_buffer buffer) { arena_defer(a, &ad_sg_destroy_buffer, sg_buffer, &buffer); }
static void defer_destroy_shader(arena_t *a, sg_shader shader) { arena_defer(a, &ad_sg_destroy_shader, sg_shader, &shader); }
static void defer_destroy_pipeline(arena_t *a, sg_pipeline pipe) { arena_defer(a, &ad_sg_destroy_pipeline, sg_pipeline, &pipe); }

static sg_image make_image(arena_t *a, const sg_image_desc *desc) {
	sg_image image = sg_make_image(desc);
	if (image.id) defer_destroy_image(a, image);
	return image;
}

static sg_buffer make_buffer(arena_t *a, const sg_buffer_desc *desc) {
	sg_buffer buffer = sg_make_buffer(desc);
	if (buffer.id) defer_destroy_buffer(a, buffer);
	return buffer;
}

static sg_shader make_shader(arena_t *a, const sg_shader_desc *desc) {
	sg_shader shader = sg_make_shader(desc);
	if (shader.id) defer_destroy_shader(a, shader);
	return shader;
}

static sg_pipeline make_pipeline(arena_t *a, const sg_pipeline_desc *desc) {
	sg_pipeline pipe = sg_make_pipeline(desc);
	if (pipe.id) defer_destroy_pipeline(a, pipe);
	return pipe;
}

typedef struct {
	int temp;
} vi_framebuffer;

typedef struct {
	um_vec3 position;
	um_vec3 normal;
} vi_vertex;

typedef struct {
	uint32_t material_id;
	sg_buffer vertex_buffer;
	sg_buffer index_buffer;
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

	uint32_t *instance_node_ids;
	size_t num_instances;
} vi_mesh;

struct vi_scene {
	arena_t *arena;
	ufbx_scene fbx;

	vi_node *nodes;
	vi_mesh *meshes;
	vi_material *materials;
};

typedef struct {
	arena_t arena;

	sg_backend backend;
	sg_buffer quad_buffer;
	sg_pipeline copy_pipe;
} vi_globals;

static vi_globals vig;

void vi_setup()
{
	arena_init(&vig.arena, NULL);

	vig.backend = sg_query_backend();

	um_vec2 quad_data[] = { { 0.0f, 0.0f }, { 0.0f, 2.0f }, { 2.0f, 0.0f } };
	vig.quad_buffer = make_buffer(&vig.arena, &(sg_buffer_desc){
		.type = SG_BUFFERTYPE_VERTEXBUFFER,
		.data = SG_RANGE(quad_data),
	});

	vig.copy_pipe = make_pipeline(&vig.arena, &(sg_pipeline_desc){
		.shader = make_shader(&vig.arena, copy_shader_desc(vig.backend)),
		.layout.attrs[0] = SG_VERTEXFORMAT_FLOAT2,
	});
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
	size_t num_parts = fbx_mesh->materials.count;
	vi_part *parts = aalloc(vs->arena, vi_part, num_parts);
	mesh->parts = parts;
	mesh->num_parts = num_parts;

	mesh->instance_node_ids = aalloc(vs->arena, uint32_t, fbx_mesh->instances.count);
	mesh->num_instances = fbx_mesh->instances.count;
	for (size_t i = 0; i < fbx_mesh->instances.count; i++) {
		mesh->instance_node_ids[i] = fbx_mesh->instances.data[i]->id;
	}

	for (size_t pi = 0; pi < num_parts; pi++) {
		vi_part *part = &parts[pi];
		ufbx_mesh_material *fbx_mesh_mat = &fbx_mesh->materials.data[pi];

		if (fbx_mesh_mat->material) {
			part->material_id = fbx_mesh_mat->material->id;
		} else {
			part->material_id = (uint32_t)vs->fbx.materials.count;
		}

		arena_t tmp = { 0 };
		
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
				ufbx_vec3 position = ufbx_get_vertex_vec3(&fbx_mesh->vertex_position, indices[ci]);
				ufbx_vec3 normal = ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, indices[ci]);
				vert->position = fbx_to_um_vec3(position);
				vert->normal = fbx_to_um_vec3(normal);
				vert++;
			}
		}

		size_t num_vertices = ufbx_generate_indices(vertices, indices, sizeof(vi_vertex), num_indices, NULL, NULL);

		part->vertex_buffer = make_buffer(vs->arena, &(sg_buffer_desc){
			.type = SG_BUFFERTYPE_VERTEXBUFFER,
			.data = { vertices, num_vertices * sizeof(vi_vertex) },
		});

		part->vertex_buffer = make_buffer(vs->arena, &(sg_buffer_desc){
			.type = SG_BUFFERTYPE_INDEXBUFFER,
			.data = { indices, num_indices * sizeof(uint32_t) },
		});

		arena_free(&tmp);
	}
}

vi_scene *vi_make_scene(const ufbx_scene *fbx_scene)
{
	arena_t *arena = arena_make(&vig.arena);
	vi_scene *vs = aalloc(arena, vi_scene, 1);
	vs->arena = arena;
	if (!vs) return NULL;

	vs->fbx = *fbx_scene;

	vs->meshes = aalloc(vs->arena, vi_mesh, fbx_scene->meshes.count);
	vs->nodes = aalloc(vs->arena, vi_node, fbx_scene->nodes.count);
	vs->materials = aalloc(vs->arena, vi_material, fbx_scene->materials.count + 1); // + NULL

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

void vi_render(vi_scene *scene, const vi_target *target, const vi_desc *desc)
{
}

void vi_present(uint32_t target_index)
{
}
