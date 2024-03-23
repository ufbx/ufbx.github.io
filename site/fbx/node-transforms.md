---
title: Node Transforms
pageTitle: Node Transforms
layout: "layouts/guide"
fbxWarning: true
eleventyNavigation:
  parent: FBX
  key: Node Transforms
  order: 1
---

FBX node transforms are formed by a chain of transformations.
If you don't have a specific need to use the FBX transformation model it is highly recommended to use the *ufbx* transform representation instead,
see the [Transforms](/elements/nodes/#transforms) section for information.

Node transforms in FBX are fundamentally expressed as properties:

* `"Lcl Translation"`: Translation relative to the parent node
* `"Lcl Scaling"`: Non-uniform scale relative to the parent node
* `"Lcl Rotation"`: Rotation relative to the parent node, expressed as Euler angles
* `"RotationOrder"`: Euler rotation order of `"Lcl Rotation"`, see `ufbx_rotation_order`

In addition to these, a bunch of offsets and pivots must be considered:

* `"ScalingPivot"`: Point around which scaling is applied
* `"ScalingOffset"`: Translation offset applied after scaling
* `"RotationPivot"`: Point around which rotation is applied
* `"RotationOffset"`: Translation offset applied after rotation

And two extra Euler rotation in addition to `"Lcl Rotation"`:

* `"PreRotation"`: Euler rotation applied before `"Lcl Rotation"`, always `XYZ` order.
* `"PostRotation"`: **Inverse** Euler rotation applied after `"Lcl Rotation"`, always `XYZ` order.

If you use specific *ufbx* load-time conversion functionality you must also account for various adjust transforms,
which are described below.
Ignoring the *ufbx*-specific transforms, the whole transform chain can be computed as:

```cpp
// ufbx-doc-example: fbx-node-transforms/get-transform

// Manually computes `ufbx_node.node_to_parent`.
// NOTE: Does not account for ufbx-specific adjust transforms.
Matrix4 get_transform(ufbx_node *node)
{
    ufbx_props *props = &node->props;

    // Fetch the transform properties.
    // In practice these could be for example manually evaluated from animation curves.
    int64_t rotation_order  = ufbx_find_int(props, "RotationOrder", 0);
    Vector3 lcl_translation = ufbx_find_vec3(props, "Lcl Translation", ufbx_zero_vec3);
    Vector3 lcl_scaling     = ufbx_find_vec3(props, "Lcl Scaling", ufbx_zero_vec3);
    Vector3 lcl_rotation    = ufbx_find_vec3(props, "Lcl Rotation", ufbx_zero_vec3);
    Vector3 rotation_pivot  = ufbx_find_vec3(props, "RotationPivot", ufbx_zero_vec3);
    Vector3 scaling_pivot   = ufbx_find_vec3(props, "ScalingPivot", ufbx_zero_vec3);
    Vector3 rotation_offset = ufbx_find_vec3(props, "RotationOffset", ufbx_zero_vec3);
    Vector3 scaling_offset  = ufbx_find_vec3(props, "ScalingOffset", ufbx_zero_vec3);
    Vector3 pre_rotation    = ufbx_find_vec3(props, "PreRotation", ufbx_zero_vec3);
    Vector3 post_rotation   = ufbx_find_vec3(props, "PostRotation", ufbx_zero_vec3);

    // Convert rotations from Euler to quaternions.
    // `enum EulerOrder` here matches `ufbx_rotation_order`.
    // Pre/post rotation always use XYZ rotation order.
    Quaternion lcl_quat = Quaternion_euler(lcl_rotation, (EulerOrder)rotation_order);
    Quaternion pre_quat = Quaternion_euler(pre_rotation, EulerOrder_XYZ);
    Quaternion post_quat = Quaternion_euler(post_rotation, EulerOrder_XYZ);

    Matrix4 m = Matrix4_identity;

    // Local scaling with pivots and offsets
    m = Matrix4_translate(-scaling_pivot) * m;
    m = Matrix4_scale_nonuniform(lcl_scaling) * m;
    m = Matrix4_translate(scaling_pivot) * m;
    m = Matrix4_translate(scaling_offset) * m;

    // Rotations with pivot, PostRotation is always inverted.
    m = Matrix4_translate(-rotation_pivot) * m;
    m = Matrix4_rotate(Quaternion_inverse(post_quat)) * m;
    m = Matrix4_rotate(lcl_quat) * m;
    m = Matrix4_rotate(pre_quat) * m;
    m = Matrix4_translate(rotation_pivot) * m;
    m = Matrix4_translate(rotation_offset) * m;

    // Finally, translation
    m = Matrix4_translate(lcl_translation) * m;

    return m;
}
```

## Adjust-transforms

In most cases, if you are working at the level of FBX properties you probably should not use most of *ufbx*'s convenience functionality.
However, if you are using the following functionality you also must account for adjust transforms in `ufbx_node`:

    UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS -> @(ufbx_node.)adjust_pre_rotation, @(ufbx_node.)adjust_pre_scale
    UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY   -> @(ufbx_node.)adjust_pre_rotation, @(ufbx_node.)adjust_translation_scale
    UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT     -> @(ufbx_node.)adjust_pre_translation
    UFBX_INHERIT_MODE_HANDLING_COMPENSATE   -> @(ufbx_node.)adjust_post_scale
    ufbx_load_opts.camera_axes/@(ufbx_load_opts.)light_axes   -> @(ufbx_node.)adjust_post_rotation

Nodes that have a non-identity adjust transform have `ufbx_node.has_adjust_transform` set to `true`,
but it is always safe to apply the adjust transforms even if unnecessary.
Augmented example of `get_transform()`, which accounts for all of the *ufbx*-specific adjust transforms:

```cpp
// ufbx-doc-example: fbx-node-transforms/get-transform-adjust

// Manually computes `ufbx_node.node_to_parent`, accounting for ufbx adjust transforms.
Matrix4 get_transform(ufbx_node *node)
{
    ufbx_props *props = &node->props;

    // Fetch the transform properties.
    // In practice these could be for example manually evaluated from animation curves.
    int64_t rotation_order  = ufbx_find_int(props, "RotationOrder", 0);
    Vector3 lcl_translation = ufbx_find_vec3(props, "Lcl Translation", ufbx_zero_vec3);
    Vector3 lcl_scaling     = ufbx_find_vec3(props, "Lcl Scaling", ufbx_zero_vec3);
    Vector3 lcl_rotation    = ufbx_find_vec3(props, "Lcl Rotation", ufbx_zero_vec3);
    Vector3 rotation_pivot  = ufbx_find_vec3(props, "RotationPivot", ufbx_zero_vec3);
    Vector3 scaling_pivot   = ufbx_find_vec3(props, "ScalingPivot", ufbx_zero_vec3);
    Vector3 rotation_offset = ufbx_find_vec3(props, "RotationOffset", ufbx_zero_vec3);
    Vector3 scaling_offset  = ufbx_find_vec3(props, "ScalingOffset", ufbx_zero_vec3);
    Vector3 pre_rotation    = ufbx_find_vec3(props, "PreRotation", ufbx_zero_vec3);
    Vector3 post_rotation   = ufbx_find_vec3(props, "PostRotation", ufbx_zero_vec3);

    // ufbx-specific transform adjustments
    Vector3 adjust_pre_translation = node->adjust_pre_translation;
    Quaternion adjust_pre_rotation = node->adjust_pre_rotation;
    Quaternion adjust_post_rotation = node->adjust_post_rotation;
    float adjust_pre_scale = (float)node->adjust_pre_scale;
    float adjust_post_scale = (float)node->adjust_post_scale;
    float adjust_translation_scale = (float)node->adjust_translation_scale;

    // Convert rotations from Euler to quaternions.
    // `enum EulerOrder` here matches `ufbx_rotation_order`.
    // Pre/post rotation always use XYZ rotation order.
    Quaternion lcl_quat = Quaternion_euler(lcl_rotation, (EulerOrder)rotation_order);
    Quaternion pre_quat = Quaternion_euler(pre_rotation, EulerOrder_XYZ);
    Quaternion post_quat = Quaternion_euler(post_rotation, EulerOrder_XYZ);

    Matrix4 m = Matrix4_identity;

    // ufbx post-adjustments
    m = Matrix4_rotate(adjust_post_rotation) * m;
    m = Matrix4_scale(adjust_post_scale) * m;

    // Local scaling with pivots and offsets
    m = Matrix4_translate(-scaling_pivot) * m;
    m = Matrix4_scale_nonuniform(lcl_scaling) * m;
    m = Matrix4_translate(scaling_pivot) * m;
    m = Matrix4_translate(scaling_offset) * m;

    // Rotations with pivot, PostRotation is always inverted.
    m = Matrix4_translate(-rotation_pivot) * m;
    m = Matrix4_rotate(Quaternion_inverse(post_quat)) * m;
    m = Matrix4_rotate(lcl_quat) * m;
    m = Matrix4_rotate(pre_quat) * m;
    m = Matrix4_translate(rotation_pivot) * m;
    m = Matrix4_translate(rotation_offset) * m;

    // Finally, translation, using the potentially scaled local translaiton.
    m = Matrix4_translate(lcl_translation) * m;

    // ufbx pre-adjustments
    m = Matrix4_translate(adjust_pre_translation) * m;
    m = Matrix4_rotate(adjust_pre_rotation) * m;
    m = Matrix4_scale(adjust_pre_scale) * m;

    // `adjust_translation_scale` is a tricky one: it must be applied only to
    // the translation of the resulting matrix or transform.
    m.m03 *= adjust_translation_scale;
    m.m13 *= adjust_translation_scale;
    m.m23 *= adjust_translation_scale;

    return m;
}
```

It is also possible to compute the transformation chain into separate translation/rotation/scale components.
For reference, see how it is implemented in *ufbx* in [`ufbxi_get_transform()`](https://github.com/ufbx/ufbx/blob/6dd7771ee215c7489a211b7aa259f1056ce14354/ufbx.c?ts=4#L21497-L21558).

## Geometric transforms

Geometric transforms (referred to as geometry transforms in *ufbx*) allow transforming the content of a node independent of its children.
See [Geometry transforms](/elements/nodes/#geometry-transforms) for an overview and the approaches to use them in *ufbx*.

Fortunately, even though geometric transforms are difficult to represent within many scene graphs,
they are quite straightforward to interpret from the file.

* `"GeometricTranslation"`: Translation of the node content (eg. mesh) relative to the node
* `"GeometricScaling"`: Scaling of the node content
* `"GeometricRotation"`: Euler rortation of the node content, always `XYZ` order

```cpp
// ufbx-doc-example: fbx-node-transforms/get-geometry-transform

// Manually computes `ufbx_node.geometry_to_node`.
Matrix4 get_geometry_transform(ufbx_node *node)
{
    ufbx_props *props = &node->props;

    // Fetch the transform properties.
    // In practice these could be for example manually evaluated from animation curves.
    Vector3 geo_translation = ufbx_find_vec3(props, "GeometricTranslation", ufbx_zero_vec3);
    Vector3 geo_scaling     = ufbx_find_vec3(props, "GeometricScaling", ufbx_zero_vec3);
    Vector3 geo_rotation    = ufbx_find_vec3(props, "GeometricRotation", ufbx_zero_vec3);

    // Convert the rotation from Euler to quaternions.
    Quaternion geo_quat = Quaternion_euler(geo_rotation, EulerOrder_XYZ);

    Matrix4 m = Matrix4_identity;

    // Combine the T/R/S translation
    m = Matrix4_scale_nonuniform(geo_scaling) * m;
    m = Matrix4_rotate(geo_quat) * m;
    m = Matrix4_translate(geo_translation) * m;

    return m;
}
```