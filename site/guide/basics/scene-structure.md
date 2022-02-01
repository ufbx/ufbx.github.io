---
title: scene-structure
pageTitle: Scene structure
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Scene structure
  order: 1
---

## Elements

A `ufbx_scene` consists of multiple **elements**[^1]: Meshes, materials, animated properties, etc. are all represented as elements.
Every element has a type, a name, properties, and connections. The latter two are handled internally by ufbx but advanced users
can still access them through `ufbx_element.props` and `ufbx_element.connections_src/dst`.

`ufbx_scene.elements` contains all the elements but there are separated lists for each element type, eg. `ufbx_scene.meshes`.
`ufbx_element.element_id` refers to the index in the first list and `ufbx_element.typed_id` in the second one. These
indices are stable between loading the _same_ file multiple times, but may change between re-exports.

[^1]: FBX calls these "objects", but ufbx uses "element" to avoid confusion with 3D objects.


### Overview of element types

- <strong>Scene hierarchy</strong>
  - Object&emsp;`ufbx_node`
- <strong>Node attachments</strong>
  - Mesh geometry&emsp;`ufbx_mesh`
  - Light source&emsp;`ufbx_light`
  - Camera&emsp;`ufbx_camera`
  - Bone&emsp;`ufbx_bone`
  - Empty / Null&emsp;`ufbx_empty`
  - Curves&emsp;`ufbx_line_curve`&ensp;`ufbx_nurbs_curve`
  - Surfaces&emsp;`ufbx_nurbs_surface`&ensp;`ufbx_nurbs_trim_surface`&ensp;`ufbx_nurbs_trim_boundary`
  - Oddities&emsp;`ufbx_procedural_geometry`&ensp;`ufbx_camera_stereo`&ensp;`ufbx_camera_switcher`&ensp;`ufbx_lod_group`
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

Some things about scene structure

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
