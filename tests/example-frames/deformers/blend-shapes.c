// $dep ufbx mushnub_evolved.fbx
// $ clang ufbx.c main.c -lm -o example
// $ ./example mushnub_evolved.fbx Blink=0.5 Smile=0.7 Spikes=0.4 > result.obj
#include "ufbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -- EXAMPLE_SOURCE --

typedef struct BlendShape {
    const char *name;
} BlendShape;

// Non-indexed triangulated skinned mesh
typedef struct Mesh {
    BlendShape *blends;
    size_t num_blends;
    Vertex *vertices;
    size_t num_vertices;
} Mesh;

Mesh process_blend_mesh(ufbx_mesh *mesh, ufbx_blend_deformer *blend)
{
    size_t num_triangles = mesh->num_triangles;
    Vertex *vertices = calloc(num_triangles * 3, sizeof(Vertex));
    size_t num_vertices = 0;

    // Triangulate the mesh, using `get_blend_vertex()` to fetch each index.
    size_t num_tri_indices = mesh->max_face_triangles * 3;
    uint32_t *tri_indices = calloc(num_tri_indices, sizeof(uint32_t));
    for (size_t face_ix = 0; face_ix < mesh->num_faces; face_ix++) {
        ufbx_face face = mesh->faces.data[face_ix];
        uint32_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];
            vertices[num_vertices++] = get_blend_vertex(mesh, blend, index);
        }
    }
    free(tri_indices);
    assert(num_vertices == num_triangles * 3);

    // Create bone descriptions
    size_t num_blends = blend->channels.count;
    if (num_blends > MAX_BLENDS) {
        num_blends = 0;
    }
    BlendShape *blends = calloc(num_blends, sizeof(BlendShape));
    for (size_t i = 0; i < num_blends; i++) {
        blends[i].name = blend->channels.data[i]->name.data;
    }

    Mesh result = {
        .blends = blends,
        .num_blends = num_blends,
        .vertices = vertices,
        .num_vertices = num_vertices,
    };
    return result;
}

void free_mesh(Mesh mesh)
{
    free(mesh.blends);
    free(mesh.vertices);
}

#define CONTROL_NAME_LENGTH 64
typedef struct Control {
    char name[CONTROL_NAME_LENGTH];
    float weight;
} Control;

size_t g_vertex_offset = 0;

// Example implementation that dumps the skinned mesh as an .obj file
void dump_mesh(Mesh mesh, ufbx_node *node, Control *controls, size_t num_controls)
{
    // Preload the control weights for the mesh blend shapes
    float weights[MAX_BLENDS] = { 0 };
    for (size_t i = 0; i < mesh.num_blends; i++) {
        const char *name = mesh.blends[i].name;
        for (size_t ci = 0; ci < num_controls; ci++) {
            if (!strcmp(controls[ci].name, name)) {
                weights[i] = controls[ci].weight;
                break;
            }
        }
    }

    // Process each vertex
    for (size_t vertex_ix = 0; vertex_ix < mesh.num_vertices; vertex_ix++) {
        Vertex vertex = mesh.vertices[vertex_ix];

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
    for (size_t vi = 0; vi < mesh.num_vertices; vi += 3) {
        size_t i = g_vertex_offset + vi;
        printf("f %zu %zu %zu\n", i + 1, i + 2, i + 3);
    }
    g_vertex_offset += mesh.num_vertices;
}

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file(argv[1], NULL, NULL);
    assert(scene);

    // Parse the blend shape controls, of form "Name=0.3"
    size_t num_controls = (size_t)(argc - 2);
    Control *controls = calloc(num_controls, sizeof(Control));
    for (size_t i = 0; i < num_controls; i++) {
        const char *arg = argv[2 + i];
        const char *split = strchr(arg, '=');
        assert(split);
        snprintf(controls[i].name, sizeof(controls[i].name), "%.*s", (int)(split - arg), arg);
        controls[i].weight = strtof(split + 1, NULL);
    }

    for (size_t mesh_ix = 0; mesh_ix < scene->meshes.count; mesh_ix++) {
        ufbx_mesh *mesh = scene->meshes.data[mesh_ix];
        if (mesh->blend_deformers.count == 0) continue;
        ufbx_blend_deformer *blend = mesh->blend_deformers.data[0];

        Mesh result = process_blend_mesh(mesh, blend);

        for (size_t inst_ix = 0; inst_ix < mesh->instances.count; inst_ix++) {
            ufbx_node *node = mesh->instances.data[inst_ix];
            dump_mesh(result, node, controls, num_controls);
        }

        free_mesh(result);
    }

    ufbx_free_scene(scene);
    return 0;
}
