#include "ufbx.h"
#include <stdio.h>
#include <stdint.h>
#include <vector>

void output_triangle(ufbx_vec3 a, ufbx_vec3 b, ufbx_vec3 c)
{
    printf("(%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
        a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
}

// -- EXAMPLE_SOURCE --

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file("my_scene.fbx", NULL, NULL);
    assert(scene);

    ufbx_node *node = ufbx_find_node(scene, "Cube");
    assert(node && node->mesh);

    list_triangles(node->mesh);

    return 0;
}

