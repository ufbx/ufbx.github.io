// $dep ufbx material_parts.fbx
// $ clang ufbx.c main.c -lm -o example
// $ ./example > result.obj
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "ufbx.h"

void create_vertex_buffer(const void *vertices, size_t count)
{
    printf(".. %zu vertices\n", count);
}

void create_index_buffer(const uint32_t *indices, size_t count)
{
    printf(".. %zu indices\n", count);
}

// -- EXAMPLE_SOURCE --

int main(int argc, char **argv)
{
    ufbx_scene *scene = ufbx_load_file("material_parts.fbx", NULL, NULL);
    assert(scene);

    for (size_t mesh_ix = 0; mesh_ix < scene->meshes.count; mesh_ix++) {
        ufbx_mesh *mesh = scene->meshes.data[mesh_ix];
        printf("mesh '%s'\n", mesh->name.data);

        for (size_t part_ix = 0; part_ix < mesh->material_parts.count; part_ix++) {
            ufbx_mesh_part *part = &mesh->material_parts.data[part_ix];
            ufbx_material *material = NULL;
            if (part_ix < mesh->materials.count) {
                material = mesh->materials.data[part_ix];
            }
            printf(". [%zu] material: %s\n", part_ix,
                material ? material->name.data : "(none)");
            convert_mesh_part(mesh, part);
        }
    }

    return 0;
}
