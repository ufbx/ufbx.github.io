---
title: Nodes
pageTitle: Nodes
layout: "layouts/guide"
eleventyNavigation:
  parent: Elements
  key: Nodes
  order: 1
---

Nodes (`ufbx_node`) form the scene graph of an FBX file.
A node by itself contains information about the transformation (`ufbx_node.local_transform`) and hierarchy (`ufbx_node.parent`/`ufbx_node.children[]`).

Nodes are specialized by attributes, such as `ufbx_mesh` or `ufbx_light`.
A single attribute can be referenced by multiple nodes,
allowing for example FBX files to have multiple instances of a mesh with different transforms.
Common attributes can be accessed directly from a node, such as `ufbx_node.mesh` or `ufbx_node.light`.
Less common attributes are stored in `ufbx_node.attrib`,
you can use helper functions such as `ufbx_as_bone()` to convert this to concrete types or `NULL`.

Conversely, instead of enumerating the attributes of a node it can be easier to consider the inverse.
Attributes (eg. `ufbx_mesh`) have a field `ufbx_element.instances[]`,
which contains a list of all nodes that use the element.

<div class="doc-viewer doc-viewer-tall">
<div data-dv-popout id="container-lamp" class="dv-inline">
<div class="dv-top">
{% include "viewer.md",
  id: "lamp",
  class: "dv-normal",
%}
</div>
</div>
</div>

<script>
viewerDescs["lamp"] = {
    scene: "/static/models/lamp.fbx",
    camera: {"yaw":-305.90381493505913,"pitch":18.233766233766076,"distance":6.176932629506754,"offset":{"x":0.6376895853405128,"y":1.627634482859347,"z":0.24936775827133878}},
    outliner: {
        includeRoot: true,
    },
    props: {
        show: true,
    },
    selectedElement: 9,
    selectedNode: 9,
    widgetScale: 0.5,
}
</script>

## Transforms

The local transform of a node can be represented with translation, rotation, and a scale.
*ufbx* stores these values in `ufbx_node.local_transform`,
which represents the transform relative to the parent node.
Nodes also store some convenience fields, such as matrices `ufbx_node.node_to_parent` and `ufbx_node.node_to_world`.

The internal FBX transforms are very complex, and *ufbx* contains a lot of functionality to mitigate them.
As long as you use the fields of `ufbx_node` and interfaces such as `ufbx_evaluate_transform()` or `ufbx_bake_anim()`,
you can ignore most of this complexity, especially when using the load options detailed below.
The internal representation of FBX transforms is detailed in [Node Transforms](/fbx/node-transforms/).

## Coordinate spaces

FBX files may be specified in arbitrary coordinate spaces,
meaning that the forward/right/up axes and the scale of a single unit can vary.
To accommodate for this you can read `@(ufbx_scene_settings.)axes` and `@(ufbx_scene_settings.)unit_meters` from `ufbx_scene.settings`.

*ufbx* also allows converting loaded scenes into preferred coordinate spaces by supplying `ufbx_load_opts.target_axes` and `ufbx_load_opts.target_unit_meters`.
There are multiple options for how this conversion should be performed, specified by `ufbx_load_opts.space_conversion`:

* `UFBX_SPACE_CONVERSION_TRANSFORM_ROOT`: Perform the space conversion in the root node
* `UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS`: Modify the transforms of nodes to compensate for the space conversion
* `UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY`: Bake the unit scaling into the geometry, axis conversion is still performed as in `@(UFBX_SPACE_CONVERSION_)ADJUST_TRANSFORMS`.

*ufbx* can also convert the handedness of the scene by mirroring, using `ufbx_load_opts.handedness_conversion_axis`.
In practice the overwhelming majority of FBX files are right-handed, so this is usually not necessary if you are using right-handed axes.
However, if loading scenes into a left-handed coordinate space, the mirroring is practically always necessary.

Additionally, *ufbx* also supports "fixing" the coordinate axes of cameras (face towards local +X) and lights (face towards local -Y by default).
These axes may be converted using `ufbx_load_opts.target_camera_axes` and `@(ufbx_load_opts.)target_light_axes`.

### Coordinate spaces in files

FBX coordinate spaces and exporters have been a major source of confusion for end-users,
as can be seen by searching *'FBX scale 100'* or *'FBX scale 0.01'*.
This is a problem in *ufbx* as well, but can be mitigated to some degree with some effort.

Most unit-aware FBXs are expressed in centimeters (`@(ufbx_scene_settings.)unit_meters = 0.01`),
as that is the de-facto default of FBX.
Blender by default works in Z-up meters, but exports FBX files in Y-up centimeters by applying a scale of `100` to all top-level nodes,
thus `UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS` will revert these transforms, removing any unnecessary scales by `100`.
Using `UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY` will scale the meshes by a factor of `0.01`,
thus it will result in the correct geometry, but the intermediate nodes will have annoying factors.

Maya files on the other hand are usually natively specified in centimeters and this tends to be baked into the meshes,
if `UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS` is used here, all top-level nodes will have a scale of `0.01` applied to them.
On the other hand, using `UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY` is preferable here as this will scale the scene without affecting node scales.

This means if you want consistent behavior across files there is no solution that handles every case.
If possible, exposing the option to select the conversion method may be useful for users.
You can also detect the exporter by loading the scene first cheaply using `ufbx_load_opts.ignore_all_content` and inspecting `ufbx_metadata.exporter`,
for example for setting the initial value of the option mentioned above in an editor.

If you are in control of the Blender exports, I highly recommend exporting with the setting `Apply Scalings` set to `FBX Units Scale`.
This causes the exported file to use meter units (`@(ufbx_scene_settings.)unit_meters = 1.0`) without any additional transforms.

```c
// ufbx-doc-example: nodes/space-conversion

// Convert the scene into meters, right-handed Y-up.
ufbx_load_opts opts = { 0 };
opts.target_axes = ufbx_axes_right_handed_y_up;
opts.target_unit_meters = 1.0f;
opts.target_camera_axes = ufbx_axes_right_handed_y_up;
opts.target_light_axes = ufbx_axes_right_handed_y_up;
if (prefer_blender) {
    opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
} else {
    opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
}
```

```cpp
// ufbx-doc-example: nodes/space-conversion

// Convert the scene into meters, right-handed Y-up.
ufbx_load_opts opts = { };
opts.target_axes = ufbx_axes_right_handed_y_up;
opts.target_unit_meters = 1.0f;
opts.target_camera_axes = ufbx_axes_right_handed_y_up;
opts.target_light_axes = ufbx_axes_right_handed_y_up;
if (prefer_blender) {
    opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
} else {
    opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
}
```

```rust
// ufbx-doc-example: nodes/space-conversion

// Convert the scene into meters, right-handed Y-up.
let opts = ufbx::LoadOpts {
    target_axes: ufbx::CoordinateAxes::right_handed_y_up(),
    target_unit_meters: 1.0,
    target_camera_axes: ufbx::CoordinateAxes::right_handed_y_up(),
    target_light_axes: ufbx::CoordinateAxes::right_handed_y_up(),
    space_conversion: if prefer_blender {
        ufbx::SpaceConversion::AdjustTransforms
    } else {
        ufbx::SpaceConversion::ModifyGeometry
    },
    ..Default::default()
};
```

## Geometry transforms

One complicated aspect of FBX files is the support for geometry[^1] transforms.
These transforms only apply to the immediate content of the node, such as a mesh,
but are not inherited to children.
This is not supported in most scene graphs, so *ufbx* provides some alternatives to handling it.

### Using geometry transforms

The geometry transforms are stored in `ufbx_node.geometry_transform`, specifying the translation, rotation, and scale.
There also exist helper matrices `ufbx_node.geometry_to_node` and `ufbx_node.geometry_to_world`.
The latter is especially useful if you only care about static meshes, converted to world space.

### Avoiding geometry transforms

As handling geometry transforms in non-static cases can be quite tricky,
*ufbx* can remove them during loading,
using the `ufbx_load_opts.geometry_transform_handling` option.
There are two approaches *ufbx* can use to handle geometry transforms:
helper nodes and modifying geometry.

Helper nodes (`UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES`) introduces "helper nodes" between nodes and their content where needed.
This method works in all cases,
but may result in extra nodes that are not present in the file.

Modifying the geometry (`UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY`) bakes the geometry transform into the vertices of meshes (and curves/surfaces).
This results in a cleaner scene graph, but may fall back to helper nodes if the transformed object cannot be modified or is instanced.
`UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK` will never insert helper nodes but will result in incorrect transformations when loading tricky files.

## Inherit modes

FBX files allow non-standard transform inheritance, indicated by `ufbx_node.inherit_mode`.
The nodes may either not propagate the scale (`UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE`) or compose scales and rotations separately (`UFBX_INHERIT_MODE_COMPONENTWISE_SCALE`).

Like for geometry transforms, *ufbx* provides load-time options to convert these into a standard scene graph:

* `UFBX_INHERIT_MODE_HANDLING_PRESERVE`: Preserves the inherit modes, for correct loading you should account for it
* `UFBX_INHERIT_MODE_HANDLING_HELPER_NODES`: Create extra nodes to compensate for non-standard inheritance modes. May significantly change the scene graph.
* `UFBX_INHERIT_MODE_HANDLING_COMPENSATE`: Attempts to compensate parent scaling by inversely scaling children where possible, otherwise falls back to spawning helper nodes.
* `UFBX_INHERIT_MODE_HANDLING_IGNORE`: Converts all non-standard inherit modes to normal. This can be incorrect but matches what many other importers do and simplifies everything.

## Pivots

FBX nodes allow setting individual pivots for rotation and scaling.
*ufbx* bakes the effects of rotation and scaling under these pivots into the translation of nodes by default.
In many cases where the rotation and scaling pivots are equal, *ufbx* can convert these pivots to geometry transforms.
This can be enabled by setting `ufbx_load_opts.pivot_handling` to `UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT`,
in which case you should also specify `ufbx_load_opts.geometry_transform_handling`,
for example to `UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY`.

## Other properties

Nodes also contain a few non-transform-related properties.
Visibility (`ufbx_node.visible`) allows marking specific nodes as invisible.
Nodes may also override materials in meshes using `ufbx_node.materials[]`,
which can be used to have multiple copies of the same instanced mesh with different materials for each instance.

[^1]: FBX calls geometry transforms "Geometric" transforms, but *ufbx* calls them geometry transforms for clarity.