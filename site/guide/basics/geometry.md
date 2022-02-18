---
title: Geometry
pageTitle: Geometry
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Geometry
  order: 1
---

Some things about geometry

{% include "viewer.md",
  id: "blender-default",
  class: "doc-viewer doc-viewer-mid",
%}
<script>
viewerDescs["blender-default"] = {
  scene: "/static/models/blender_default_cube.fbx",
  camera: {
    pitch: 20.0,
    yaw: -60.0,
    distance: 20,
    offset: { x: 2.7, y: 2.4, z: 2.9 },
  },
  outliner: {
    showMaterials: true,
  },
}
</script>

<div data-viewer-id="blender-default" style="width:100%;height:600px"></div>
