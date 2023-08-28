// $dep ufbx example_base skinned_human.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example skinned_human.fbx Female > result.obj
#include "ufbx.h"
#include "example_base.h"
#include <stdio.h>

// -- EXAMPLE_SOURCE --

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file(argv[1], NULL, NULL);
    assert(scene);

    ufbx_node *node = ufbx_find_node(scene, argv[2]);
    assert(node && node->mesh);

    ufbx_mesh *mesh = node->mesh;
    assert(mesh->skin_deformers.count > 0);
    ufbx_skin_deformer *skin = mesh->skin_deformers[0];

    for (uint32_t i = 0; i < mesh->num_vertices; i++) {
        Vector3 vertex = transform_vertex(mesh, skin, i);
        printf("v %.5f %.5f %.5f\n", vertex.x, vertex.y, vertex.z);
    }

    // Output faces
    for (ufbx_face face : mesh->faces) {
        printf("f");
        for (uint32_t j = 0; j < face.num_indices; j++) {
            printf(" %u", mesh->vertex_indices[face.index_begin + j] + 1);
        }
        printf("\n");
    }

    return 0;
}
