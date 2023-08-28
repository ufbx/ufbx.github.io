---
title: Load options
pageTitle: Load options
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Load options
  order: 6
---

You can use `ufbx_load_opts` to influence file loading.
Many of the options mitigate complexity within the FBX format, for general use
something like the following is a good start:

```c
ufbx_load_opts opts = {
    // Normalize coordinates to right-handed +Y up, meter units
    .target_axes = ufbx_axes_right_handed_y_up,
    .target_unit_meters = 1.0f,

    // Adjust the node transforms directly to perform the above conversion
    .space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS,

    // Convert geometry transforms into helper nodes
    .geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES,
};
```

## Space conversion

FBX files may be authored in various coordinate/unit spaces, some exporter results for interest:

- Blender (default): centimeters (`unit_meters=0.01`), right-handed Y-up
- Blender (native): meters (`unit_meters=1.0`), right-handed Z-up
- Maya: centimeters (`unit_meters=0.01`), right-handed Y-up
- 3ds Max: inches (`unit_meters=0.0254`), right-handed Z-up

The coordinate space of the file is retained in `ufbx_scene.settings` fields `ufbx_scene_settings.axes`
and `@(ufbx_scene_settings.)unit_meters` which describe how you should interpret the scene.

ufbx can automatically convert the scene into any convention you want to use via `ufbx_load_opts.target_axes`
and `@(ufbx_load_opts.)target_unit_meters`. By default the transformation is written into the `@(ufbx_node.)local_transform`
of the root node (`UFBX_SPACE_CONVERSION_TRANSFORM_ROOT`) in an attempt to preserve the scene as authentically as possible.
If you specify `ufbx_load_opts.space_conversion` to `UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS` the transformation is propagated
to the `@(ufbx_node.)local_transform` of the top-level non-root nodes.

## Geometry transforms

Some FBX exporters (primarily 3ds Max) support "geometry transforms" (`ufbx_node.geometry_transform`) that only
transforms the directly attached mesh (or any other attribute) without affecting the child nodes.
If you don't care about the node hierarchy you can get away with using `ufbx_node.geometry_to_world` to transform
the geometry statically which has `ufbx_node.geometry_to_node` included.

Geometry transforms are not often supported in scene graphs so ufbx has options via `ufbx_load_opts.geometry_transform_handling`
for converting the geometry transforms to forms that are easier to handle:

- `@(UFBX_GEOMETRY_TRANSFORM_HANDLING_)PRESERVE`: (default) preserve the geometry transforms as-is
- `@(UFBX_GEOMETRY_TRANSFORM_HANDLING_)HELPER_NODES`: create synthetic intermediate nodes that handle the geometry transformation
- `@(UFBX_GEOMETRY_TRANSFORM_HANDLING_)MODIFY_GEOMETRY`: modify the geometry data itself if possible but fall back to `@(UFBX_GEOMETRY_TRANSFORM_HANDLING_)HELPER_NODES` if impossible (eg. if the mesh is instanced)
- `@(UFBX_GEOMETRY_TRANSFORM_HANDLING_)MODIFY_GEOMETRY_NO_FALLBACK`: like `@(UFBX_GEOMETRY_TRANSFORM_HANDLING_)MODIFY_GEOMETRY` but instead of falling back to helper nodes it retains the geometry transform in `ufbx_node.geometry_transform`
