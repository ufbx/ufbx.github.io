// $dep ufbx units_*.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "ufbx.h"

void check_file(const char *path, bool prefer_blender)
{
    // -- EXAMPLE_SOURCE --

    ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
    assert(scene);

    ufbx_node *node = ufbx_find_node(scene, "Cube");
    assert(node);
    ufbx_mesh *mesh = node->mesh;
    assert(mesh);

    ufbx_vec3 scale = node->local_transform.scale;
    ufbx_real node_size = fmaxf(scale.x, fmaxf(scale.y, scale.z));

    ufbx_real mesh_size = 0.0f;
    for (ufbx_vec3 v : mesh->vertices) {
        mesh_size = fmax(mesh_size, fabs(v.x));
        mesh_size = fmax(mesh_size, fabs(v.y));
        mesh_size = fmax(mesh_size, fabs(v.z));
    }

    const char *mode = "";
    switch (opts.space_conversion) {
        case UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS:
            mode = "ADJUST_TRANSFORMS";
            break;
        case UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY:
            mode = "MODIFY_GEOMETRY";
            break;
    }

    const char *comment = "..";
    if (fabs(node_size - 1.0f) < 0.001f && fabs(mesh_size - 1.0f) < 0.001f) {
        comment = "clean";
    }

    printf("  %-20s node_size %-8.2f mesh_size %-8.2f %s\n",
        mode, node_size, mesh_size, comment);

    ufbx_free_scene(scene);
}

int main(int argc, char **argv)
{
    const char *filenames[] = {
        "units_blender_meters.fbx",
        "units_blender_default.fbx",
        "units_maya_default.fbx",
    };
    size_t num_filenames = sizeof(filenames) / sizeof(*filenames);

    for (const char *path : filenames) {
        printf("\n%s:\n", path);
        check_file(path, false);
        check_file(path, true);
    }
}

