// $dep ufbx example_math complex_transforms.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example complex_transforms.fbx
#include <stdio.h>
#include <assert.h>
#include "ufbx.h"
#include "example_math.h"

// -- EXAMPLE_SOURCE --

static bool failed = false;

void check_column(const char *label, Vector3 col, Vector3 ref)
{
    float error = 0.0f;
    error += fabsf(col.x - ref.x);
    error += fabsf(col.y - ref.y);
    error += fabsf(col.z - ref.z);
    if (error >= 0.001f) {
        failed = true;
    }

    printf("  %s: (%+.2f, %+.2f, %+.2f), reference (%+.2f, %+.2f, %+.2f)\n",
        label, col.x, col.y, col.z, ref.x, ref.y, ref.z);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: ./example <filename.fbx>\n");
        return 1;
    }
    ufbx_scene *scene = ufbx_load_file(argv[1], NULL, NULL);
    assert(scene);

    const char *prop_names[] = {
        "Lcl Translation",
        "Lcl Scaling",
        "Lcl Rotation",
        "RotationPivot",
        "ScalingPivot",
        "RotationOffset",
        "ScalingOffset",
        "PreRotation",
        "PostRotation",
    };

    for (ufbx_node *node : scene->nodes) {
        if (node->is_root) continue;

        printf("\n%s:\n", node->name.data);

        for (const char *name : prop_names) {
            ufbx_vec3 v = ufbx_find_vec3(&node->props, name, ufbx_zero_vec3);
            if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f) {
                printf("  %s: (%.2f, %.2f, %.2f)\n", name, v.x, v.y, v.z);
            }
        }

        int64_t order = ufbx_find_int(&node->props, "RotationOrder", 0);
        if (order != 0) {
            printf("  RotationOrder: %d\n", (int)order);
        }

        Matrix4 mat = get_transform(node);

        Vector3 col_x = { mat.m00, mat.m10, mat.m20 };
        Vector3 col_y = { mat.m01, mat.m11, mat.m21 };
        Vector3 col_z = { mat.m02, mat.m12, mat.m22 };
        Vector3 col_w = { mat.m03, mat.m13, mat.m23 };

        Vector3 ref_x = node->node_to_parent.cols[0];
        Vector3 ref_y = node->node_to_parent.cols[1];
        Vector3 ref_z = node->node_to_parent.cols[2];
        Vector3 ref_w = node->node_to_parent.cols[3];

        check_column("X", col_x, ref_x);
        check_column("Y", col_y, ref_y);
        check_column("Z", col_z, ref_z);
        check_column("W", col_w, ref_w);
    }

    if (failed) {
        fprintf(stderr, "Error: get_transform() does not match ufbx\n");
        return 1;
    }

    return 0;
}
