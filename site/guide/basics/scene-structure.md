---
title: scene-structure
pageTitle: Scene structure
eleventyNavigation:
  parent: Basics
  key: Basics
  order: 0
  excerpt: Scene structure
---

{% include interactive.md %}

Some things about scene structure

{% include viewer.md,
  id: "blender-default"
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
}
</script>

Here is another scene you might care about_!

{% include viewer.md,
  id: "blender-default-alt"
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

