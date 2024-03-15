// $dep ufbx cube.fbx
// $ clang ufbx.c main.c -lm -o example
// $ ./example > result.obj
#include <stdio.h>
#include <assert.h>
#include "ufbx.h"

uint32_t g_index_base = 0;

void create_vertex_buffer(const void *data, size_t size)
{
}

void create_index_buffer(const uint32_t *data, size_t size)
{
}

// -- EXAMPLE_SOURCE --

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file("cube.fbx", NULL, NULL);
    assert(scene);

    for (uint32_t mesh_ix = 0; mesh_ix < scene->meshes.count; mesh_ix++) {
        ufbx_mesh *mesh = scene->meshes.data[mesh_ix];
        for (uint32_t node_ix = 0; node_ix < mesh->instances.count; node_ix++) {
            ufbx_node *node = mesh->instances.data[node_ix];

            printf("# mesh '%s', node '%s'\n\n", mesh->name.data, node->name.data);

            // Set the current transformation matrices
            g_mesh_to_world = node->geometry_to_world;
            g_mesh_normal_to_world = ufbx_matrix_for_normals(&g_mesh_to_world);

            draw_polygons(mesh);
        }
    }

    return 0;
}

