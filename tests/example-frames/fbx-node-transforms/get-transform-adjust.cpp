// $dep ufbx example_math complex_transforms.fbx
// $ clang++ ufbx.c main.cpp -lm -o example
// $ ./example complex_transforms.fbx
#include <stdio.h>
#include <assert.h>
#include "ufbx.h"
#include "example_math.h"

// -- EXAMPLE_SOURCE --

static bool failed = false;

void print_adjust_vec3(const char *label, ufbx_vec3 adjust)
{
    if (adjust.x != 0.0f || adjust.y != 0.0f || adjust.z != 0.0f) {
        printf("  %s: (%.2f, %.2f, %.2f)\n", label, adjust.x, adjust.y, adjust.z);
    }
}

void print_adjust_quat(const char *label, ufbx_quat adjust)
{
    if (adjust.x != 0.0f || adjust.y != 0.0f || adjust.z != 0.0f || adjust.w != 1.0f) {
        printf("  %s: (%.2f, %.2f, %.2f, %.2f)\n", label, adjust.x, adjust.y, adjust.z, adjust.w);
    }
}

void print_adjust_scale(const char *label, ufbx_real scale)
{
    if (scale != 1.0f) {
        printf("  %s: %.2f\n", label, scale);
    }
}

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
    if (argc < 1) {
        fprintf(stderr, "Usage: ./example <filename.fbx>\n");
        return 1;
    }

    ufbx_load_opts opts = { };

    // Z-up inches, like 3ds Max
    opts.target_axes = ufbx_axes_right_handed_z_up;
    opts.target_unit_meters = 0.0254f;

    // Affects: adjust_pre_rotation, adjust_translation_scale
    opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
    // Affects: adjust_pre_translation
    opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
    // Affects: adjust_post_scale
    opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_COMPENSATE;
    // Affects: adjust_post_rotation
    opts.target_light_axes = ufbx_axes_right_handed_z_up;

    ufbx_scene *scene = ufbx_load_file(argv[1], &opts, NULL);
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

        print_adjust_vec3("adjust_pre_translation", node->adjust_pre_translation);
        print_adjust_quat("adjust_pre_rotation", node->adjust_pre_rotation);
        print_adjust_scale("adjust_pre_scale", node->adjust_pre_scale);
        print_adjust_quat("adjust_post_rotation", node->adjust_post_rotation);
        print_adjust_scale("adjust_post_scale", node->adjust_post_scale);
        print_adjust_scale("adjust_translation_scale", node->adjust_translation_scale);

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
