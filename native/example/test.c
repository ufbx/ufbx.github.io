#include "example_graphics.h"

#define SOKOL_DEBUG
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_glue.h"

#include <stdlib.h>

typedef struct {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
} Vertex;

typedef struct {
    VertexBuffer vertex_buffer;
    IndexBuffer index_buffer;
} MeshPart;

MeshPart create_mesh_part(ufbx_mesh *mesh, ufbx_mesh_part *part)
{
    size_t num_triangles = part->num_triangles;
    Vertex *vertices = calloc(num_triangles * 3, sizeof(Vertex));
    size_t num_vertices = 0;

    // Reserve space for the maximum triangle indices.
    size_t num_tri_indices = mesh->max_face_triangles * 3;
    uint32_t *tri_indices = calloc(num_tri_indices, sizeof(uint32_t));

    // Iterate over each face using the specific material.
    for (size_t face_ix = 0; face_ix < part->num_faces; face_ix++) {
        ufbx_face face = mesh->faces.data[part->face_indices.data[face_ix]];

        // Triangulate the face into `tri_indices[]`.
        uint32_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

        // Iterate over each triangle corner contiguously.
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];

            // Convert ufbx vertex into custom types.
            Vertex *v = &vertices[num_vertices++];
            v->position = Vector3_ufbx(ufbx_get_vertex_vec3(&mesh->vertex_position, index));
            v->normal = Vector3_ufbx(ufbx_get_vertex_vec3(&mesh->vertex_normal, index));
            v->uv = Vector2_ufbx(ufbx_get_vertex_vec2(&mesh->vertex_uv, index));
        }
    }

    // Should have written all the vertices.
    free(tri_indices);
    assert(num_vertices == num_triangles * 3);

    // Generate the index buffer.
    ufbx_vertex_stream streams[1] = {
        { vertices, num_vertices, sizeof(Vertex) },
    };
    size_t num_indices = num_triangles * 3;
    uint32_t *indices = calloc(num_indices, sizeof(uint32_t));

    // This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
    // indices are written in `indices[]` and the number of unique vertices is returned.
    num_vertices = ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

    MeshPart result;
    result.vertex_buffer = create_vertex_buffer(vertices, num_vertices, sizeof(Vertex));
    result.index_buffer = create_index_buffer(indices, num_indices, sizeof(uint32_t));

    free(indices);
    free(vertices);

    return result;
}

// -- Scene representation

typedef struct Mesh {
    MeshPart *parts;
    size_t num_parts;
} Mesh;

typedef struct Material {
    Vector3 base_color;
} Material;

typedef struct Node {
    uint32_t mesh_index;
    Matrix4 node_to_world;

    uint32_t *material_indices;
    size_t num_materials;
} Node;

typedef struct Scene {

    Mesh *meshes;
    size_t num_meshes;

    Material *materials;
    size_t num_materials;

    Node *nodes;
    size_t num_nodes;

} Scene;

Mesh create_mesh(ufbx_mesh *mesh)
{
    size_t num_parts = mesh->material_parts.count;

    Mesh result = { 0 };

    result.num_parts = num_parts;
    result.parts = calloc(num_parts, sizeof(MeshPart));
    for (size_t i = 0; i < num_parts; i++) {
        result.parts[i] = create_mesh_part(mesh, &mesh->material_parts.data[i]);
    }
    return result;
}

Material create_material(ufbx_material *material)
{
    Material result = { 0 };
    result.base_color = Vector3_ufbx(material->pbr.base_color.value_vec3);
    return result;
}

Node create_node(ufbx_node *node)
{
    size_t num_materials = node->materials.count;

    Node result = { 0 };

    result.node_to_world = Matrix4_ufbx(node->node_to_world);
    result.mesh_index = node->mesh ? node->mesh->typed_id : UINT32_MAX;

    result.num_materials = num_materials;
    result.material_indices = calloc(num_materials, sizeof(uint32_t));
    for (size_t i = 0; i < num_materials; i++) {
        result.material_indices[i] = node->materials.data[i]->typed_id;
    }
    return result;
}

Scene create_scene(ufbx_scene *scene)
{
    Scene result = { 0 };

    result.num_meshes = scene->meshes.count;
    result.meshes = calloc(result.num_meshes, sizeof(Mesh));
    for (size_t i = 0; i < result.num_meshes; i++) {
        result.meshes[i] = create_mesh(scene->meshes.data[i]);
    }

    result.num_materials = scene->materials.count;
    result.materials = calloc(result.num_materials, sizeof(Material));
    for (size_t i = 0; i < result.num_materials; i++) {
        result.materials[i] = create_material(scene->materials.data[i]);
    }

    result.num_nodes = scene->nodes.count;
    result.nodes = calloc(result.num_nodes, sizeof(Node));
    for (size_t i = 0; i < result.num_nodes; i++) {
        result.nodes[i] = create_node(scene->nodes.data[i]);
    }

    return result;
}

// Order and type of uniforms must match what is declared in the shader.

typedef struct VertexUniforms {
    Matrix4 object_to_world;
    Matrix4 world_to_clip;
} VertexUniforms;

typedef struct FragmentUniforms {
    Vector3 base_color;
} FragmentUniforms;

const char *vertex_shader = "#version 330\n"
    "\n" ""
    "\n" "uniform mat4 object_to_world;"
    "\n" "uniform mat4 world_to_clip;"
    "\n" ""
    "\n" "layout(location=0) in vec3 position;"
    "\n" "layout(location=1) in vec3 normal;"
    "\n" "layout(location=2) in vec2 uv;"
    "\n" ""
    "\n" "out vec3 v_normal;"
    "\n" "out vec2 v_uv;"
    "\n" ""
    "\n" "void main() {"
    "\n" "    vec3 world_position = (object_to_world * vec4(position, 1.0)).xyz;"
    "\n" "    gl_Position = world_to_clip * vec4(world_position, 1.0);"
    "\n" "    v_normal = (object_to_world * vec4(normal, 0.0)).xyz;"
    "\n" "    v_uv = uv;"
    "\n" "}";

const char *fragment_shader = "#version 330\n"
    "\n" ""
    "\n" "uniform vec3 base_color;"
    "\n" ""
    "\n" "in vec3 v_normal;"
    "\n" "in vec2 v_uv;"
    "\n" ""
    "\n" "out vec4 o_color;"
    "\n" ""
    "\n" "void main() {"
    "\n" "    float l = dot(normalize(v_normal), normalize(vec3(1.0))) * 0.5 + 0.5;"
    "\n" "    o_color = vec4(base_color * l, 1.0);"
    "\n" "}";

typedef struct State {
    const char *filename;
    Scene scene;
    sg_pipeline mesh_pipe;
    Arcball arcball;
} State;

State state;

void init(void)
{
    graphics_setup();
	arcball_setup(&state.arcball);

    sg_pipeline_desc mesh_desc = pipeline_default_solid();
    mesh_desc.shader = load_shader_data(vertex_shader, fragment_shader);
    mesh_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // Vertex.position
    mesh_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; // Vertex.normal
    mesh_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2; // Vertex.uv
    mesh_desc.index_type = SG_INDEXTYPE_UINT32;
    state.mesh_pipe = sg_make_pipeline(&mesh_desc);

    ufbx_load_opts opts = { 
        .target_axes = ufbx_axes_right_handed_y_up,
        .target_unit_meters = 1.0,
        .geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY,
        .space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY,
    };

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file(state.filename, &opts, &error);
    if (!scene) {
        char error_desc[512];
        ufbx_format_error(error_desc, sizeof(error_desc), &error);
        fprintf(stderr, error_desc);
        exit(1);
    }

    state.scene = create_scene(scene);

    ufbx_free_scene(scene);
}

void frame(void)
{
    Scene scene = state.scene;

    VertexUniforms vertex_uniforms = { 0 };
    FragmentUniforms fragment_uniforms = { 0 };
    vertex_uniforms.world_to_clip = arcball_world_to_clip(&state.arcball);

    begin_main_pass(0x6495ed);

    sg_apply_pipeline(state.mesh_pipe);

    for (size_t node_ix = 0; node_ix < scene.num_nodes; node_ix++) {
        Node node = scene.nodes[node_ix];
        if (node.mesh_index == UINT32_MAX) {
            continue;
        }

        vertex_uniforms.object_to_world = node.node_to_world;

        Mesh mesh = scene.meshes[node.mesh_index];

        for (size_t part_ix = 0; part_ix < mesh.num_parts; part_ix++) {
            MeshPart part = mesh.parts[part_ix];
            if (part.index_buffer.count == 0) continue;

            if (part_ix < node.num_materials) {
                Material material = scene.materials[node.material_indices[part_ix]];
                fragment_uniforms.base_color = material.base_color;
            } else {
                fragment_uniforms.base_color = Vector3_new(0.8f, 0.8f, 0.8f);
            }

            sg_bindings bindings = { 
                .vertex_buffers = { part.vertex_buffer.buffer },
                .index_buffer = part.index_buffer.buffer,
            };

            sg_apply_bindings(&bindings);
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(vertex_uniforms));
            sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(fragment_uniforms));
            sg_draw(0, (int)part.index_buffer.count, 1);
        }
    }

    end_main_pass();
}

void event(const sapp_event *e)
{
    arcball_event(&state.arcball, e);
}

void cleanup(void)
{
    graphics_cleanup();
}

sapp_desc sokol_main(int argc, char* argv[]) {

    state.filename = argv[1];

    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup,
        .width = 800,
        .height = 600,
        .sample_count = 4,
        .window_title = "ufbx example",
        .logger = { slog_func },
    };
}

