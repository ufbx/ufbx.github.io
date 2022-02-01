---
title: scene-structure
pageTitle: Scene structure
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Getting started
  order: 0
---

## Setup

The only files you will need are `ufbx.c/h`. You can get them from [https://github.com/bqqbarbhg/ufbx](https://github.com/bqqbarbhg/ufbx).
The library has no dependencies aside from libc (and libm on platforms where it's separate from libc).

Alternatively you can `#include "ufbx.c"` if you want to override some of the preprocessor defines.
In this case I recommend renaming the file to `ufbx.c.h` so it's purpose is clearer.

```c
#define ufbx_assert(cond) my_assert(cond)
#include "ufbx.c.h"
```

## Loading scenes

All you need to get started is to call one of the `ufbx_load_*()` functions to obtain a `ufbx_scene`.

```c
// Load a scene from a file
ufbx_scene *scene = ufbx_load_file("my_scene.fbx", NULL, NULL);
// .. or from a memory buffer
ufbx_scene *scene = ufbx_load_memory(my_data, my_size, NULL, NULL);
// .. you can also load from a custom stream but let's look into it later

// Make sure to free the scene when you're done with it
ufbx_free_scene(scene);
```

You can also provide additional options via `ufbx_load_opts` such as custom allocators, memory limits, or ignoring aspects of the file that you don't need.

Detailed error information is returned via the `ufbx_error` struct.
If you encounter a file that won't load please file an issue with the output of `ufbx_format_error()` included.

```c
// Zeroed defaults are equivalent to calling with NULL options.
ufbx_load_opts opts = {
    // We're only interested in animation
    .ignore_geometry = true,
    // Load external files referenced by the .fbx file
    .load_external_files = true,
};

ufbx_error error;
ufbx_scene *scene = ufbx_load_file("my_scene.fbx", &opts, &error);
if (!scene) {
    fprintf(stderr, "Failed to load: %s\n", error.description);
}
```

## Interactive viewers

This guide has interactive scene viewers to illustrate some concepts like the one below.

- Controls:
    - Left mouse button: Turn camera
    - Shift + Left mouse button / Middle mouse button: Pan camera
    - Shift + Scroll: Zoom camera

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
  props: {
      show: true,
  }
}
</script>

## Elements

Loaded scenes consist of **elements**[^1] that describe aspects of the scene such as geometry or animation. See `ufbx_element_type` for all
supported element types.

[^1]: FBX itself calls these **objects** but to avoid confusion with 3D objects ufbx uses **element**.

