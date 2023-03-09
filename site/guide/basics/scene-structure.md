---
title: Scene structure
pageTitle: Scene structure
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Scene structure
  order: 1
---

FBX scenes are built from a scene graph consisting of objects/nodes (represented by `ufbx_node`).
These may have attributes attached, such as meshes (`ufbx_mesh`), lights (`ufbx_light`), cameras (`ufbx_camera`).
The node determines the transformation of the attached attribute.

All of the above are more generically instances of `ufbx_element`, representing a "base class" that can have a name
and some generic properties. One handy property is `ufbx_element.typed_id` that contains an unique index for each
element of a type, ie. `ufbx_mesh.typed_id` refers to its position in `ufbx_scene.meshes[]`. You can use these indices
to refer to custom data related to the element.

The scene graph has one root node at `ufbx_scene.root`, each node has transformation specified in `ufbx_node.local_transform`
which affects any attributes attached to the node and its children `ufbx_node.children[]`. You can access common attributes
of a node via `ufbx_node.mesh`, `@(ufbx_node.)light`, `@(ufbx_node.)camera`, for other types you have to use the generic
`ufbx_node.attrib` and convert it to the correct type (eg. using `ufbx_as_nurbs_curve()`). You might also notice `ufbx_node.all_attribs[]`,
in theory FBX files allow you to attach multiple attributes to a node, but this is *exceedingly* rare and I recommend to ignore it unless
you know that you're dealing with some cursed/custom files. On the other hand one attribute may belong to multiple nodes in normal files,
see [Instancing](#Instancing).

## Instancing

Meshes and other attributes may belong to multiple `ufbx_node` instances, meaning clones of the mesh/light/curve/etc at different locations.
These instances may also have different materials using `ufbx_node.materials[]` but we'll go over that in more detail later in [Geometry](/guide/basics/geometry).
It is often easier to reverse your perspective: instead of iterating over nodes and their attributes you can iterate over attributes and their instances.

```c
@(
  ufbx_scene *scene;
)
// Node -> Mesh
for (size_t i = 0; i < scene->nodes.count; i++) {
    ufbx_node *node = scene->nodes.data[i];
    if (node->mesh) {
        process_mesh(node->mesh);
        process_instance(node, node->mesh);
    }
}

// Mesh -> Node
for (size_t i = 0; i < scene->meshes.count; i++) {
    ufbx_mesh *mesh = scene->meshes.data[i];
    process_mesh(mesh);
    for (size_t j = 0; j < mesh->instances.count; j++) {
        ufbx_node *node = mesh->instances.data[j];
        process_instance(node, mesh);
    }
}
```

```cpp
@(
  ufbx_scene *scene;
)

// Node -> Mesh
for (ufbx_node *node : scene->nodes) {
    if (node->mesh) {
        process_mesh(mesh);
        process_instance(node, mesh);
    }
}

// Mesh -> Node
for (ufbx_mesh *mesh : scene->meshes) {
    process_mesh(mesh);
    for (ufbx_node *node : mesh->instances) {
        process_instance(node, mesh);
    }
}
```

<!-- OLD

## Elements

A `ufbx_scene` consists of multiple **elements**[^1]: Meshes, materials, animated properties, etc. are all represented as elements.

You can access all the elements of a scene through `ufbx_scene.elements` and more conveniently through typed lists of
elements such as `ufbx_scene.meshes`. Each element stores it's index in the shared list (`ufbx_element.element_id`) and
in the per-type list (`ufbx_element.typed_id`), these indices are stable when loading the _same_[^2] file multiple times.

[^1]: FBX calls these "objects", but ufbx uses "element" to avoid confusion with 3D objects.

[^2]: The indices may change between multiple re-exports of the same file!

### Overview of element types

Here's a preview of all supported element types in ufbx. We will go over these in detail later so feel free to skim
the list to get a feel of what is possible.

- <strong>Scene hierarchy</strong>
  - Object&emsp;`ufbx_node`
- <strong>Node attachments</strong>
  - Mesh geometry&emsp;`ufbx_mesh`
  - Light source&emsp;`ufbx_light`
  - Camera&emsp;`ufbx_camera`
  - Bone&emsp;`ufbx_bone`
  - Empty / Null&emsp;`ufbx_empty`&ensp;`ufbx_marker`
  - Curves&emsp;`ufbx_line_curve`&ensp;`ufbx_nurbs_curve`
  - Surfaces&emsp;`ufbx_nurbs_surface`&ensp;`ufbx_nurbs_trim_surface`&ensp;`ufbx_nurbs_trim_boundary`
  - Oddities&emsp;`ufbx_procedural_geometry`&ensp;`ufbx_stereo_camera`&ensp;`ufbx_camera_switcher`&ensp;`ufbx_lod_group`
- <strong>Geometry deformers</strong>
  - Skinning&emsp;`ufbx_skin_deformer`&ensp;`ufbx_skin_cluster`
  - Blend shape&emsp;`ufbx_blend_deformer`&ensp;`ufbx_blend_channel`&ensp;`ufbx_blend_shape`
  - Geometry cache&emsp;`ufbx_cache_deformer`&ensp;`ufbx_cache_file`
- <strong>Shading</strong>
  - Material&emsp;`ufbx_material`
  - Texture&emsp;`ufbx_texture`&ensp;`ufbx_video`
  - Shading model&emsp;`ufbx_shader`&ensp;`ufbx_shader_binding`
- <strong>Animation</strong>
  - Stack / Take&emsp;`ufbx_anim_stack`
  - Layer&emsp;`ufbx_anim_layer`
  - Curves&emsp;`ufbx_anim_value`&ensp;`ufbx_anim_curve`
- <strong>Authoring</strong>
  - Collections&emsp;`ufbx_display_layer`&ensp;`ufbx_selection_set`&ensp;`ufbx_selection_node`
  - Constraints / Rigging&emsp;`ufbx_character`&ensp;`ufbx_constraint`&ensp;`ufbx_pose`
  - Metadata&emsp;`ufbx_metadata_object`
  - Unknown&emsp;`ufbx_unknown`

## Node hierarchy

Nodes (`ufbx_node`) represent objects in the scene. By themselves they only have name and transformation but may contain
contain *attributes*, such as a mesh (`ufbx_mesh`). A node may also have child nodes that inherit the parent's transformation.
The scene contains a single root `ufbx_scene.root_node` that contains all the top-level nodes.

```c
void visit_node(ufbx_node *node)
{
    // Print the name and local position
    ufbx_vec3 pos = node->local_transform.translation;
    printf("Node %s: (%f, %f, %f)\n", node->name.data,
        pos.x, pos.y, pos.z);

    // Recursively visit the children
    for (size_t i = 0; i < node->children.count; i++) {
        ufbx_node *child = node->children.data[i];
        visit_node(child);
    }
}

ufbx_scene *scene;
visit_node(scene->root_node);
```

Alternatively we can use `ufbx_scene.nodes` that contains a flattened list of all nodes.
The nodes are sorted by depth so you can apply parent transformations without recursion!

Here's an example how to compute `ufbx_node.node_to_world`.

```c
ufbx_scene *scene;
ufbx_matrix node_to_world[128];

for (size_t i = 0; i < scene->nodes.count; i++) {
    ufbx_node *node = scene->nodes.data[i];

    // Resolve the parent transform (use identity if root)
    ufbx_matrix parent_to_world = ufbx_identity_matrix;
    if (node->parent) {
        uint32_t parent_id = node->parent->typed_id;

        // NOTE: `parent_id < i` due to nodes being sorted by depth
        parent_to_world = node_to_world[parent_id];
    }

    // NOTE: `i == node->typed_id` as that's the order we iterate in
    node_to_world[i] = ufbx_matrix_mul(&parent_to_world, &node->node_to_parent);
}
```

Transformation is represented using `ufbx_node.local_transform`

{% include "viewer.md",
  id: "blender-default",
  class: "doc-viewer doc-viewer-mid",
%}
<script>
viewerDescs["blender-default"] = {
  scene: "/static/models/blender_default_cube.fbx",
  camera: {
    yaw: 0,
    pitch: 0,
    distance: 30,
    offset: { x: 0, y: 0, z: 0 },
  },
  outliner: {
    showMaterials: true,
  },
  props: {
    show: true,
  },
}
</script>

Here is another scene you might care about_!

{% include "viewer.md",
  id: "blender-default-alt",
  class: "doc-viewer doc-viewer-mid",
%}
<script>
viewerDescs["blender-default-alt"] = {
  scene: "/static/models/blender_default_cube.fbx",
  camera: {
    yaw: 0,
    pitch: 0,
    distance: 30,
    offset: { x: 0, y: 0, z: 0 },
  },
}
</script>

HEE

{% include "viewer.md",
  id: "blender-default-alt3",
  class: "doc-viewer doc-viewer-mid",
%}
<script>
viewerDescs["blender-default-alt3"] = {
  scene: "/static/models/blender_default_cube.fbx",
  camera: {
    yaw: 0,
    pitch: 0,
    distance: 30,
    offset: { x: 0, y: 0, z: 0 },
  },
}
</script>

-->
