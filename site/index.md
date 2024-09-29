---
title: ufbx - single source FBX loader
pageTitle: ufbx documentation
layout: "layouts/guide"
noTitle: true
plainTitle: true
eleventyNavigation:
  key: Overview
  order: 0
---

<img class="logo" alt="ufbx logo" src="/static/ufbx-cube.svg" />

[ufbx][gh-ufbx] is a single source file FBX loader.

The library aims to provide a user-friendly interface to easily interpret the
FBX file format while exhaustively supporting advanced features for those who
need them.

* Single C99/C++11 source file
* Handles invalid files and out-of-memory conditions gracefully
* Strongly typed API with bounds-checked slices in C++ and Rust bindings
* Extensively [tested and fuzzed][gh-ufbx-testing]

```c
// ufbx-doc-example: overview/hello-ufbx
ufbx_load_opts opts = { 0 }; // Optional, pass NULL for defaults
ufbx_error error; // Optional, pass NULL if you don't care about errors
ufbx_scene *scene = ufbx_load_file("my_scene.fbx", &opts, &error);
if (!scene) {
    fprintf(stderr, "Failed to load: %s\n", error.description.data);
    exit(1);
}

// Use and inspect `scene`, it's just plain data!

// Let's just list all objects within the scene for example:
for (size_t i = 0; i < scene->nodes.count; i++) {
    ufbx_node *node = scene->nodes.data[i];
    if (node->is_root) continue;

    printf("Object: %s\n", node->name.data);
    if (node->mesh) {
        printf("-> mesh with %zu faces\n", node->mesh->faces.count);
    }
}

ufbx_free_scene(scene);
```

```cpp
// ufbx-doc-example: overview/hello-ufbx
ufbx_load_opts opts = { }; // Optional, pass NULL for defaults
ufbx_error error; // Optional, pass NULL if you don't care about errors
ufbx_scene *scene = ufbx_load_file("my_scene.fbx", &opts, &error);
if (!scene) {
    fprintf(stderr, "Failed to load: %s\n", error.description.data);
    exit(1);
}

// Use and inspect `scene`, it's just plain data!

// Let's just list all objects within the scene for example:
for (ufbx_node *node : scene->nodes) {
    if (node->is_root) continue;

    printf("Object: %s\n", node->name.data);
    if (node->mesh) {
        printf("-> mesh with %zu faces\n", node->mesh->faces.count);
    }
}

ufbx_free_scene(scene);
```

```rust
// ufbx-doc-example: overview/hello-ufbx
let opts = ufbx::LoadOpts::default();
let scene = match ufbx::load_file("my_scene.fbx", opts) {
    Ok(scene) => scene,
    Err(error) => panic!("Failed to load scene: {}", error.description),
};

// Use and inspect `scene`, it's just plain data!

// Let's just list all objects within the scene for example:
for node in &scene.nodes {
    if node.is_root { continue }

    println!("Object: {}", node.element.name);
    if let Some(mesh) = &node.mesh {
        println!("-> mesh with {} faces", mesh.faces.len());
    }
}
```

[gh-ufbx]: https://github.com/ufbx/ufbx
[gh-ufbx-testing]: https://github.com/ufbx/ufbx#Testing
