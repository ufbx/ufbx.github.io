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

All you need to get started is to call one of the `ufbx_load_*()` functions to obtain an `ufbx_scene`.
There are a couple of helper functions that assist in evaluating animations.

```c
// Load a scene from a file
ufbx_scene *scene = ufbx_load_file("my_scene.fbx", NULL, NULL);
// .. or from a memory buffer
ufbx_scene *scene = ufbx_load_memory(my_data, my_size, NULL, NULL);
// .. you can also load from a custom stream but let's look into it later

// Make sure to free the scene when you're done with it
ufbx_free_scene(scene);
```

You can also provide additional options via `ufbx_load_opts` or retrieve error information using `ufbx_error`.

```c
// Zeroed defaults are equivalent to calling with NULL options.
ufbx_load_opts opts = {
    // Allow ufbx to look for external files such as geometry caches
    .load_external_files = true,
};

ufbx_error error;
ufbx_scene *scene = ufbx_load_file("my_scene.fbx", &opts, &error);
if (!scene) {
    fprintf(stderr, "Failed to load: %s\n", error.description);
}

for (size_t i = 0; i < 10; i++) {
    printf("%zu\n", i);
}

if (foo) {
    ufbx_node *node = ufbx_find_node("Cube");
    printf("%s\n", node->name.data);
} else {
    printf("%s\n", node->name.data);
}
```

<script>

let prevHoverId = null
let prevHoverDecls = []
let prevHoverRefs = []

let decls = { }
let refs = { }

function updateHover(id) {
    const prevId = prevHoverId
    if (prevHoverId !== null) {
        for (const elem of decls[prevId]) elem.classList.remove("c-hover")
        for (const elem of refs[prevId]) elem.classList.remove("c-hover")
    }

    for (const elem of decls[id]) elem.classList.add("c-hover")
    for (const elem of refs[id]) elem.classList.add("c-hover")

    prevHoverId = id
}

function endHover(id) {
    if (id === prevHoverId) {
        for (const elem of decls[id]) elem.classList.remove("c-hover")
        for (const elem of refs[id]) elem.classList.remove("c-hover")
        prevHoverId = null
    }
}

for (const elem of document.querySelectorAll("[data-ref-id]")) {
    const id = elem.dataset.refId
    if (!decls[id]) decls[id] = []
    if (!refs[id]) refs[id] = []
    refs[id].push(elem)
    elem.addEventListener("mouseover", () => updateHover(id))
    elem.addEventListener("mouseout", () => endHover(id))
}
for (const elem of document.querySelectorAll("[data-decl-id]")) {
    const id = elem.dataset.declId
    if (!decls[id]) decls[id] = []
    if (!refs[id]) refs[id] = []
    decls[id].push(elem)
    elem.addEventListener("mouseover", () => updateHover(id))
    elem.addEventListener("mouseout", () => endHover(id))
}
</script>
