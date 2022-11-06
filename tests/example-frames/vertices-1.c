#include "ufbx.h"
#include <stdio.h>
#include <stdint.h>

void begin_polygon()
{
    printf("begin polygon\n");
}

void polygon_corner(ufbx_vec3 position, ufbx_vec3 normal)
{
    printf("  position (%.2f, %.2f, %.2f) normal (%.2f, %.2f, %.2f)\n",
        position.x, position.y, position.z,
        normal.x, normal.y, normal.z);
}

void end_polygon()
{
    printf("end polygon\n");
}

// -- EXAMPLE_SOURCE --

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file("my_scene.fbx", NULL, NULL);
    assert(scene);

    ufbx_node *node = ufbx_find_node(scene, "Cube");
    assert(node && node->mesh);

    list_vertices(node->mesh);

    return 0;
}
