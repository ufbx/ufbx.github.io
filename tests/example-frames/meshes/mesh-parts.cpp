// $dep ufbx material_parts.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <vector>
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

    for (ufbx_mesh *mesh : scene->meshes) {
        printf("mesh '%s'\n", mesh->name.data);

        for (ufbx_mesh_part &part : mesh->material_parts) {
            ufbx_material *material = NULL;
            if (part.index < mesh->materials.count) {
                material = mesh->materials.data[part.index];
            }
            printf(". [%u] material: %s\n", part.index,
                material ? material->name.data : "(none)");
            convert_mesh_part(mesh, &part);
        }
    }

    return 0;
}

