// $dep ufbx mushnub_evolved.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example mushnub_evolved.fbx Blink=0.5 Smile=0.7 Spikes=0.4 > result.obj
#include "ufbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

// -- EXAMPLE_SOURCE --

struct BlendShape {
    std::string name;
};

// Non-indexed triangulated skinned mesh
struct Mesh {
    std::vector<BlendShape> blends;
    std::vector<Vertex> vertices;
};

Mesh process_blend_mesh(ufbx_mesh *mesh, ufbx_blend_deformer *blend)
{
    Mesh result;

    // Triangulate the mesh, using `get_blend_vertex()` to fetch each index.
    size_t num_tri_indices = mesh->max_face_triangles * 3;
    std::vector<uint32_t> tri_indices(num_tri_indices);
    for (ufbx_face face : mesh->faces) {
        uint32_t num_tris = ufbx_triangulate_face(
            tri_indices.data(), tri_indices.size(), mesh, face);
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];
            result.vertices.push_back(get_blend_vertex(mesh, blend, index));
        }
    }
    assert(result.vertices.size() == mesh->num_triangles * 3);

    // Create bone descriptions
    size_t num_blends = blend->channels.count;
    if (num_blends > MAX_BLENDS) {
        num_blends = 0;
    }
    for (size_t i = 0; i < num_blends; i++) {
        result.blends.push_back(BlendShape{ blend->channels[i]->name.data });
    }

    return result;
}

struct Control {
    std::string name;
    float weight;
};

size_t g_vertex_offset = 0;

// Example implementation that dumps the skinned mesh as an .obj file
void dump_mesh(Mesh mesh, ufbx_node *node, const std::vector<Control> &controls)
{
    // Preload the control weights for the mesh blend shapes
    float weights[MAX_BLENDS] = { };
    size_t blend_ix = 0;
    for (const BlendShape &blend : mesh.blends) {
        for (const Control &control : controls) {
            if (control.name == blend.name) {
                weights[blend_ix] = control.weight;
                break;
            }
        }
        blend_ix++;
    }

    // Process each vertex
    for (const Vertex &vertex : mesh.vertices) {
        ufbx_vec3 p = vertex.position;
        for (size_t i = 0; i < MAX_BLENDS; i++) {
            ufbx_vec3 offset = vertex.blend_offsets[i];
            float weight = weights[i];
            p.x += offset.x * weight;
            p.y += offset.y * weight;
            p.z += offset.z * weight;
        }

        // The blend offsets are applied in local space, transform to global space
        p = ufbx_transform_position(&node->geometry_to_world, p);
        printf("v %.3f %.3f %.3f\n", p.x, p.y, p.z);
    }

    // Just a flat list of non-indexed triangle faces
    for (size_t vi = 0; vi < mesh.vertices.size(); vi += 3) {
        size_t i = g_vertex_offset + vi;
        printf("f %zu %zu %zu\n", i + 1, i + 2, i + 3);
    }
    g_vertex_offset += mesh.vertices.size();
}

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file(argv[1], NULL, NULL);
    assert(scene);

    // Parse the blend shape controls, of form "Name=0.3"
    std::vector<Control> controls;
    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];
        const char *split = strchr(arg, '=');
        assert(split);
        controls.push_back(Control{
            std::string(arg, split), strtof(split + 1, NULL),
        });
    }

    for (ufbx_mesh *mesh : scene->meshes) {
        if (mesh->blend_deformers.count == 0) continue;
        ufbx_blend_deformer *blend = mesh->blend_deformers[0];

        Mesh result = process_blend_mesh(mesh, blend);

        for (ufbx_node *node : mesh->instances) {
            dump_mesh(result, node, controls);
        }
    }

    ufbx_free_scene(scene);
    return 0;
}
