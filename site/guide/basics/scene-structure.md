---
title: scene-structure
pageTitle: Scene structure
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Scene structure
  order: 0
---

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
