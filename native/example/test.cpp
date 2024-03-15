#include "example_graphics.h"

#define SOKOL_DEBUG
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_glue.h"

#include <vector>
#include <stdlib.h>

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
};

typedef struct {
    VertexBuffer vertex_buffer;
    IndexBuffer index_buffer;
} MeshPart;

MeshPart create_mesh_part(ufbx_mesh *mesh, ufbx_mesh_part *part)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> tri_indices;
    tri_indices.resize(mesh->max_face_triangles * 3);

    // Iterate over each face using the specific material.
    for (uint32_t face_index : part->face_indices) {
        ufbx_face face = mesh->faces[face_index];

        // Triangulate the face into `tri_indices[]`.
        uint32_t num_tris = ufbx_triangulate_face(
            tri_indices.data(), tri_indices.size(), mesh, face);

        // Iterate over each triangle corner contiguously.
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];

            // Write the vertex. In actual implementation you'd want to convert to custom
            // vector types or other more compact formats.
            Vertex v;
            v.position = mesh->vertex_position[index];
            v.normal = mesh->vertex_normal[index];
            v.uv = mesh->vertex_uv[index];
            vertices.push_back(v);
        }
    }

    // Should have written all the vertices.
    assert(vertices.size() == part->num_triangles * 3);

    // Generate the index buffer.
    ufbx_vertex_stream streams[1] = {
        { vertices.data(), vertices.size(), sizeof(Vertex) },
    };
    std::vector<uint32_t> indices;
    indices.resize(part->num_triangles * 3);

    // This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
    // indices are written in `indices[]` and the number of unique vertices is returned.
    size_t num_vertices = ufbx_generate_indices(
        streams, 1, indices.data(), indices.size(), nullptr, nullptr);

    // Trim to only unique vertices.
    vertices.resize(num_vertices);

    MeshPart result = { };
    result.vertex_buffer = create_vertex_buffer(vertices.data(), vertices.size(), sizeof(Vertex));
    result.index_buffer = create_index_buffer(indices.data(), indices.size(), sizeof(uint32_t));
    return result;
}


// -- Scene representation

struct Mesh {
    std::vector<MeshPart> parts;
};

struct Material {
    Vector3 base_color;
};

struct Node {
    uint32_t mesh_index;
    Matrix4 node_to_world;

    std::vector<uint32_t> material_indices;
};

struct Scene {
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Node> nodes;
};

Mesh create_mesh(ufbx_mesh *mesh)
{
    size_t num_parts = mesh->material_parts.count;

    Mesh result = { };
    for (ufbx_mesh_part &part : mesh->material_parts) {
        result.parts.push_back(create_mesh_part(mesh, &part));
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

    Node result = { };
    result.node_to_world = Matrix4_ufbx(node->node_to_world);
    result.mesh_index = node->mesh ? node->mesh->typed_id : UINT32_MAX;
    for (ufbx_material *material : node->materials) {
        result.material_indices.push_back(material->typed_id);
    }
    return result;
}

Scene create_scene(ufbx_scene *scene)
{
    Scene result = { };
    for (ufbx_mesh *mesh : scene->meshes) {
        result.meshes.push_back(create_mesh(mesh));
    }
    for (ufbx_material *material : scene->materials) {
        result.materials.push_back(create_material(material));
    }
    for (ufbx_node *node : scene->nodes) {
        result.nodes.push_back(create_node(node));
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

const char *vertex_shader = R"(
    #version 330

    uniform mat4 object_to_world;
	uniform mat4 world_to_clip;

	layout(location=0) in vec3 position;
	layout(location=1) in vec3 normal;
	layout(location=2) in vec2 uv;

	out vec3 v_normal;
	out vec2 v_uv;

	void main() {
		vec3 world_position = (object_to_world * vec4(position, 1.0)).xyz;
		gl_Position = world_to_clip * vec4(world_position, 1.0);
		v_normal = (object_to_world * vec4(normal, 0.0)).xyz;
		v_uv = uv;
	}
)";

const char *fragment_shader = R"(
    #version 330

	uniform vec3 base_color;

	in vec3 v_normal;
	in vec2 v_uv;

	out vec4 o_color;

	void main() {
		float l = dot(normalize(v_normal), normalize(vec3(1.0))) * 0.5 + 0.5;
		o_color = vec4(base_color * l, 1.0);
	}
)";

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

    sg_pipeline_desc mesh_desc = pipeline_default_solid();
    mesh_desc.shader = load_shader_data(vertex_shader, fragment_shader);
    mesh_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // Vertex.position
    mesh_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; // Vertex.normal
    mesh_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2; // Vertex.uv
    mesh_desc.index_type = SG_INDEXTYPE_UINT32;
    state.mesh_pipe = sg_make_pipeline(&mesh_desc);

    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0;
    opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
    opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file(state.filename, &opts, &error);
    if (!scene) {
        char error_desc[512];
        ufbx_format_error(error_desc, sizeof(error_desc), &error);
        fprintf(stderr, error_desc);
        exit(1);
    }

    state.scene = create_scene(scene);

    arcball_setup(&state.arcball, scene);

    ufbx_free_scene(scene);
}

void frame(void)
{
    Scene scene = state.scene;

    VertexUniforms vertex_uniforms = { };
    FragmentUniforms fragment_uniforms = { };
    vertex_uniforms.world_to_clip = arcball_world_to_clip(&state.arcball);

    begin_main_pass(0x6495ed);

    sg_apply_pipeline(state.mesh_pipe);

    for (const Node &node : scene.nodes) {
        if (node.mesh_index == UINT32_MAX) {
            continue;
        }

        vertex_uniforms.object_to_world = node.node_to_world;

        const Mesh &mesh = scene.meshes[node.mesh_index];
        for (size_t part_ix = 0; part_ix < mesh.parts.size(); part_ix++) {
            const MeshPart &part = mesh.parts[part_ix];
            if (part.index_buffer.count == 0) continue;

            if (part_ix < node.material_indices.size()) {
                Material material = scene.materials[node.material_indices[part_ix]];
                fragment_uniforms.base_color = material.base_color;
            } else {
                fragment_uniforms.base_color = Vector3_new(0.8f, 0.8f, 0.8f);
            }

            sg_bindings bindings = { };
			bindings.vertex_buffers[0] = { part.vertex_buffer.buffer };
			bindings.index_buffer = part.index_buffer.buffer;

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

    sapp_desc desc = { };
	desc.init_cb = init;
	desc.frame_cb = frame;
	desc.event_cb = event;
	desc.cleanup_cb = cleanup;
	desc.width = 800;
	desc.height = 600;
	desc.sample_count = 4;
	desc.window_title = "ufbx example";
	desc.logger = { slog_func };
    return desc;
}

