// $dep ufbx cube.fbx
// $ clang ufbx.c main.c -lm -o example
// $ ./example > result.obj
#include <stdio.h>
#include <assert.h>
#include "ufbx.h"

ufbx_matrix g_mesh_to_world;
ufbx_matrix g_mesh_normal_to_world;
uint32_t g_index_base = 0;
uint32_t g_face_size = 0;

void begin_polygon(void)
{
}

void polygon_corner(ufbx_vec3 position, ufbx_vec3 normal, ufbx_vec2 uv)
{
    position = ufbx_transform_position(&g_mesh_to_world, position);
    normal = ufbx_transform_direction(&g_mesh_normal_to_world, normal);

    printf("v %.2f %.2f %.2f\n", position.x, position.y, position.z);
    printf("vn %.2f %.2f %.2f\n", normal.x, normal.y, normal.z);
    printf("vt %.2f %.2f\n", uv.x, uv.y);
    g_face_size++;
}

void end_polygon(void)
{
    printf("f");
    for (uint32_t i = 0; i < g_face_size; i++) {
        uint32_t ix = 1 + g_index_base + i;
        printf(" %u/%u/%u", ix, ix, ix);
    }
    printf("\n\n");
    g_index_base += g_face_size;
    g_face_size = 0;
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
