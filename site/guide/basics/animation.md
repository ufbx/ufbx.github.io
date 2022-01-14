---
title: Animation
pageTitle: Animation
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Animation
  order: 2
---

Some things about `ufbx_anim_stack`

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
