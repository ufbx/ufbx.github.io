// $dep ufbx light.fbx
// $ clang ufbx.c main.c -lm -o example
// $ ./example
#include <stdio.h>
#include <assert.h>
#include "ufbx.h"

// -- EXAMPLE_SOURCE --

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file("light.fbx", NULL, NULL);
    assert(scene);

    ufbx_node *node = ufbx_find_node(scene, "Light");
    assert(node);
    assert(node->light);

    print_intensity(node->light);

    return 0;
}
