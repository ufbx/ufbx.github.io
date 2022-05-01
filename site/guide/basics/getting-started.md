---
title: Getting started
pageTitle: Scene structure
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Getting started
  order: 0
---

## Setup

### C / C++

*ufbx* is a single source library which means all you need is two files `ufbx.c` and `ufbx.h`.
The simplest way to get started is to download them from [https://github.com/bqqbarbhg/ufbx](https://github.com/bqqbarbhg/ufbx).

Unlike single *header* libraries you'll need to compile `ufbx.c` along the rest of your code.
Alternatively you can `#include "ufbx.c"` in a single file to compile it.

*ufbx* has no dependencies outside of libc, though you may need to pass `-lm` to link the C
standard math library.

```
gcc -lm ufbx.c main.c -o main
clang -lm ufbx.c main.c -o main
```

### Rust

*ufbx* has Rust bindings as the crate [ufbx](https://crates.io/crates/ufbx).

```
cargo install ufbx
```

## Loading a scene

To get started with *ufbx* we need to first load a scene.
After loading a scene you can get pretty far by just inspecting the returned `ufbx_scene` structure.

Let's load a scene from a file and print the names of all the nodes (aka objects in the scene hierarchy).

```c
#include <stdio.h>
#include "ufbx.h"

int main()
{
    ufbx_scene *scene = ufbx_load_file("my_scene.fbx", NULL, NULL);

    for (size_t i = 0; i < scene->nodes.count; i++) {
        ufbx_node *node = scene->nodes.data[i];
        printf("%s\n", node->name.data);
    }

    ufbx_free_scene(scene);
    return 0;
}
```

```cpp
#include <stdio.h>
#include "ufbx.h"

int main()
{
    ufbx_scene *scene = ufbx_load_file("my_scene.fbx", nullptr, nullptr);

    for (ufbx_node *node : scene->nodes) {
        printf("%s\n", node->name.data);
    }

    ufbx_free_scene(scene);
    return 0;
}
```

```rust
use ufbx;

fn main() {
    let scene = ufbx::load_file("my_scene.fbx", ufbx::LoadOpts::default()).unwrap();

    for node in &scene.nodes {
        println!("{}", node.element.name);
    }
}
```

In the above example we loaded the scene with default options and were pretty ruthless when it comes to error handling.
To fix this we can use `ufbx_error` to get information about any errors that happen during loading.

We can also pass in `ufbx_load_opts` for options.
FBX scenes have varying coordinate and unit systems and *ufbx* supports normalizing them at load time.
Here we request a right-handed Y up coordinate system with 1 meter units.

```c
#include <stdio.h>
#include "ufbx.h"

int main()
{
    ufbx_load_opts opts = {
        .target_axes = ufbx_axes_right_handed_y_up,
        .target_unit_meters = 1.0f,
    };

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file("my_scene.fbx", &opts, &error);
    if (!scene) {
        fprintf(stderr, "Failed to load scene: %s\n", error.description.data);
        return 1;
    }

    for (size_t i = 0; i < scene->nodes.count; i++) {
        ufbx_node *node = scene->nodes.data[i];
        printf("%s\n", node->name.data);
    }

    ufbx_free_scene(scene);
    return 0;
}
```

```cpp
#include <stdio.h>
#include "ufbx.h"

int main()
{
    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file("my_scene.fbx", &opts, &error);
    if (!scene) {
        fprintf(stderr, "Failed to load scene: %s\n", error.description.data);
        return 1;
    }

    for (ufbx_node *node : scene->nodes) {
        printf("%s\n", node->name.data);
    }

    ufbx_free_scene(scene);
    return 0;
}
```

```rust
use ufbx;

fn main() {
    let opts = ufbx::LoadOpts {
        target_axes: ufbx::CoordinateAxes::right_handed_y_up(),
        target_unit_meters: 1.0,
        ..Default::default()
    };

    let scene = match ufbx::load_file("my_scene.fbx", opts) {
        Ok(scene) => scene,
        Err(error) => panic!("Failed to load scene: {}", error.description),
    };

    for node in &scene.nodes {
        println!("{}", node.element.name);
    }
}
```

If you prefer you can also load scenes from memory using `ufbx_load_memory()` or even custom
streams using `ufbx_load_stream()`.

## Interactive viewers

This guide contains interactive viewers to demonstrate the concepts. You can select elements
from the left panel to inspect and adjust their properties.

- Left mouse button: Turn camera
- Shift + Left mouse button / Middle mouse button: Pan camera
- Shift + Scroll: Zoom camera

{% include "viewer.md",
  id: "blender-default",
  class: "doc-viewer doc-viewer-tall",
%}
<script>
viewerDescs["blender-default"] = {
    scene: "/static/models/blender_default_cube.fbx",
    camera: {"yaw":-375.7609577922058,"pitch":29.80519480519472,"distance":23.37493738981497,"offset":{"x":4.814403983585144,"y":1.6022970913598036,"z":-0.11125223900915396}},
    props: {
        show: true,
    },
}
</script>
