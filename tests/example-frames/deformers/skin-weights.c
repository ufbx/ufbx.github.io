// $dep ufbx skinned_human.fbx
// $ clang ufbx.c main.c -lm -o example
// $ ./example skinned_human.fbx Female > result.obj
#include "ufbx.h"
#include <stdio.h>
#include <stdlib.h>

// -- EXAMPLE_SOURCE --

typedef struct Bone {
    uint32_t node_index;
    ufbx_matrix geometry_to_bone;
} Bone;

// Non-indexed triangulated skinned mesh
typedef struct Mesh {
    Bone *bones;
    size_t num_bones;
    Vertex *vertices;
    size_t num_vertices;
} Mesh;

Mesh process_skinned_mesh(ufbx_mesh *mesh, ufbx_skin_deformer *skin)
{
    size_t num_triangles = mesh->num_triangles;
    Vertex *vertices = calloc(num_triangles * 3, sizeof(Vertex));
    size_t num_vertices = 0;

    // Triangulate the mesh, using `get_skinned_vertex()` to fetch each index.
    size_t num_tri_indices = mesh->max_face_triangles * 3;
    uint32_t *tri_indices = calloc(num_tri_indices, sizeof(uint32_t));
    for (size_t face_ix = 0; face_ix < mesh->num_faces; face_ix++) {
        ufbx_face face = mesh->faces.data[face_ix];
        uint32_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];
            vertices[num_vertices++] = get_skinned_vertex(mesh, skin, index);
        }
    }
    free(tri_indices);
    assert(num_vertices == num_triangles * 3);

    // Create bone descriptions
    size_t num_bones = skin->clusters.count;
    Bone *bones = calloc(num_bones, sizeof(Bone));
    for (size_t i = 0; i < num_bones; i++) {
        ufbx_skin_cluster *cluster = skin->clusters.data[i];
        bones[i].node_index = cluster->bone_node->typed_id;
        bones[i].geometry_to_bone = cluster->geometry_to_bone;
    }

    Mesh result = {
        .bones = bones,
        .num_bones = num_bones,
        .vertices = vertices,
        .num_vertices = num_vertices,
    };
    return result;
}

void matrix_add(ufbx_matrix *dst, const ufbx_matrix *src, float weight)
{
    // `ufbx_matrix` structs are affine 3x4 matrices
    for (size_t i = 0; i < 3*4; i++) {
        dst->v[i] += src->v[i] * weight;
    }
}

// Example implementation that dumps the skinned mesh as an .obj file
void dump_mesh(Mesh mesh, ufbx_scene *scene)
{
    // Resolve skinning (mesh geometry to world matrices)
    ufbx_matrix *geometry_to_world = calloc(mesh.num_bones, sizeof(ufbx_matrix));

    for (size_t i = 0; i < mesh.num_bones; i++) {
        Bone bone = mesh.bones[i];

        // This would really come from some custom scene graph or animation
        ufbx_node *node = scene->nodes.data[bone.node_index];
        ufbx_matrix bone_to_world = node->node_to_world;

        // Composite bind matrix and current transform
        geometry_to_world[i] = ufbx_matrix_mul(&bone_to_world, &bone.geometry_to_bone);
    }

    // Process each vertex
    for (size_t vertex_ix = 0; vertex_ix < mesh.num_vertices; vertex_ix++) {
        Vertex vertex = mesh.vertices[vertex_ix];

        // Weighted sum of the influencing bone matrices
        ufbx_matrix vertex_to_world = { 0 };
        for (size_t i = 0; i < MAX_WEIGHTS; i++) {
            uint32_t bone_ix = vertex.bones[i];
            float weight = vertex.weights[i];
            matrix_add(&vertex_to_world, &geometry_to_world[bone_ix], weight);
        }

        ufbx_vec3 p = ufbx_transform_position(&vertex_to_world, vertex.position);
        printf("v %.3f %.3f %.3f\n", p.x, p.y, p.z);
    }

    // Just a flat list of non-indexed triangle faces
    for (size_t i = 0; i < mesh.num_vertices; i += 3) {
        printf("f %zu %zu %zu\n", i + 1, i + 2, i + 3);
    }

    free(geometry_to_world);
}

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file(argv[1], NULL, NULL);
    assert(scene);

    ufbx_node *node = ufbx_find_node(scene, argv[2]);
    assert(node && node->mesh);

    ufbx_mesh *mesh = node->mesh;
    assert(mesh->skin_deformers.count > 0);
    ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];

    Mesh result = process_skinned_mesh(mesh, skin);
    dump_mesh(result, scene);

    return 0;
}
