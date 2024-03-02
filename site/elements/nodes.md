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
This results in a cleaner scene graph, but may fall back to helper nodes if the transormed object cannot be modified or is instanced.
`UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK` will never insert helper nodes but will result in incorrect transformations when loading tricky files.

## Inherit modes

FBX files allow non-standard transform inheritance, indicated by `ufbx_node.inherit_mode`.
The nodes may either not propagate the scale (`UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE`) or compose scales and rotations separately (`UFBX_INHERIT_MODE_COMPONENTWISE_SCALE`).

Like for geometry transforms, *ufbx* provides load-time options to convert these into a standard scene graph:

* `UFBX_INHERIT_MODE_HANDLING_PRESERVE`: Preserves the inherit modes, for correct loading you should account for it
* `UFBX_INHERIT_MODE_HANDLING_HELPER_NODES`: Create extra nodes to compensate for non-standard inheritance modes. May significantly change the scene graph.
* `UFBX_INHERIT_MODE_HANDLING_COMPENSATE`: Attempts to compensate parent scaling by inversely scaling children where possible, otherwise falls back to spawning helper nodes.
* `UFBX_INHERIT_MODE_HANDLING_IGNORE`: Converts all non-standard inherit modes to normal. This can be incorrect but matches what many other importers do and simplifies everything.

## Other properties

Nodes also contain a few non-transform-related properties.
Visibility (`ufbx_node.visible`) allows marking specific nodes as invisible.
Nodes may also override materials in meshes using `ufbx_node.materials[]`,
which can be used to have multiple copies of the same instanced mesh with different materials for each instance.

[^1]: FBX calls geometry transforms "Geometric" transforms, but *ufbx* calls them geometry transforms for clarity.