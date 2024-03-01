---
title: Nodes
pageTitle: Nodes
layout: "layouts/guide"
eleventyNavigation:
  parent: Elements
  key: Nodes
  order: 1
---

Nodes form the basis for the scene graph in FBX files.

<div class="doc-viewer doc-viewer-tall">
<div data-dv-popout id="container-stack" class="dv-inline">
<div class="dv-top">
{% include "viewer.md",
  id: "stack",
  class: "dv-normal",
%}
</div>
</div>
</div>

<script>
viewerDescs["stack"] = {
    scene: "/static/models/stack.fbx",
    camera: {"yaw":-305.90381493505913,"pitch":18.233766233766076,"distance":6.176932629506754,"offset":{"x":0.6376895853405128,"y":1.627634482859347,"z":0.24936775827133878}},
    outliner: {
    },
    props: {
        show: true,
    },
    widgetScale: 15.0,
}
</script>

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

