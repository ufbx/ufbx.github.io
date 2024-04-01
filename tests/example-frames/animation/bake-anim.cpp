// $dep ufbx cube_animations.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example
#include <stdio.h>
#include <assert.h>
#include "ufbx.h"

// -- EXAMPLE_SOURCE --

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file("cube_animations.fbx", NULL, NULL);
    assert(scene);

    bake_animations(scene);

    return 0;
}

