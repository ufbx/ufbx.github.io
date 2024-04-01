// $dep ufbx skinned_human.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example skinned_human.fbx Female > result.obj
#include "ufbx.h"
#include <stdio.h>
#include <vector>

// -- EXAMPLE_SOURCE --

typedef struct Bone {
    uint32_t node_index;
    ufbx_matrix geometry_to_bone;
} Bone;

// Non-indexed triangulated skinned mesh
typedef struct Mesh {
    std::vector<Bone> bones;
    std::vector<Vertex> vertices;
} Mesh;

Mesh process_skinned_mesh(ufbx_mesh *mesh, ufbx_skin_deformer *skin)
{
    Mesh result;

    // Triangulate the mesh, using `get_skinned_vertex()` to fetch each index.
    size_t num_tri_indices = mesh->max_face_triangles * 3;
    std::vector<uint32_t> tri_indices(num_tri_indices);
    for (size_t face_ix = 0; face_ix < mesh->num_faces; face_ix++) {
        ufbx_face face = mesh->faces[face_ix];
        uint32_t num_tris = ufbx_triangulate_face(
            tri_indices.data(), tri_indices.size(), mesh, face);
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];
            result.vertices.push_back(get_skinned_vertex(mesh, skin, index));
        }
    }
    assert(result.vertices.size() == mesh->num_triangles * 3);

    // Create bone descriptions
    for (ufbx_skin_cluster *cluster : skin->clusters) {
        result.bones.push_back(Bone{
            cluster->bone_node->typed_id, cluster->geometry_to_bone,
        });
    }

    return result;
}

void matrix_add(ufbx_matrix &dst, const ufbx_matrix &src, float weight)
{
    // `ufbx_matrix` structs are affine 3x4 matrices
    for (size_t i = 0; i < 3*4; i++) {
        dst.v[i] += src.v[i] * weight;
    }
}

// Example implementation that dumps the skinned mesh as an .obj file
void dump_mesh(const Mesh &mesh, ufbx_scene *scene)
{
    // Resolve skinning (mesh geometry to world matrices)
    std::vector<ufbx_matrix> geometry_to_world;
    for (const Bone &bone : mesh.bones) {
        // This would really come from some custom scene graph or animation
        ufbx_node *node = scene->nodes[bone.node_index];
        ufbx_matrix bone_to_world = node->node_to_world;
        // Composite bind matrix and current transform
        geometry_to_world.push_back(ufbx_matrix_mul(&bone_to_world, &bone.geometry_to_bone));
    }

    // Process each vertex
    for (const Vertex &vertex : mesh.vertices) {
        // Weighted sum of the influencing bone matrices
        ufbx_matrix vertex_to_world = { };
        for (size_t i = 0; i < MAX_WEIGHTS; i++) {
            uint32_t bone_ix = vertex.bones[i];
            float weight = vertex.weights[i];
            matrix_add(vertex_to_world, geometry_to_world[bone_ix], weight);
        }

        ufbx_vec3 p = ufbx_transform_position(&vertex_to_world, vertex.position);
        printf("v %.3f %.3f %.3f\n", p.x, p.y, p.z);
    }

    // Just a flat list of non-indexed triangle faces
    for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
        printf("f %zu %zu %zu\n", i + 1, i + 2, i + 3);
    }
}

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file(argv[1], NULL, NULL);
    assert(scene);

    ufbx_node *node = ufbx_find_node(scene, argv[2]);
    assert(node && node->mesh);

    ufbx_mesh *mesh = node->mesh;
    assert(mesh->skin_deformers.count > 0);
    ufbx_skin_deformer *skin = mesh->skin_deformers[0];

    Mesh result = process_skinned_mesh(mesh, skin);
    dump_mesh(result, scene);

    return 0;
}
