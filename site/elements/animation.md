---
title: Animation
pageTitle: Animation
layout: "layouts/guide"
eleventyNavigation:
  parent: Elements
  key: Animation
  order: 3
---

Animations in FBX files are represented as stacks (`ufbx_anim_stack`) of layers (`ufbx_anim_layer`).
Each stack corresponds to a single animation clip, or a take, in the file.

In *ufbx*, animations are referred to using `ufbx_anim` descriptors,
this allows choosing between evaluating a composited stack of layers or a single layer,
using an uniform interface.
You can obtain a `ufbx_anim` instance from the following sources:

* `ufbx_scene.anim`: Default animation stack of the scene
* `ufbx_anim_stack.anim`: Animation stack, composited layers
* `ufbx_anim_layer.anim`: Individual animation layer
* `ufbx_create_anim()`: Custom animation descriptor

## Evaluation

Even though *ufbx* exposes the animation curves in the file,
interpreting the curves manually requires a lot of work:

* Cubic animation curves, where the time is non-linear
* Euler rotation curves, with configurable rotation order
* Pre/post rotation, pivots, offsets, detailed in [FBX Node Transforms](/fbx/node-transforms/)
* Compositing animation layers

Due to this complexity,
it is highly recommended to use the *ufbx* evaluation utilities instead,
as they handle everything above internally.
For most cases, [Animation baking](#animation-baking) is a good starting point,
as it transforms the complex FBX animation into an easily consumable form.

### Scene evaluation

The simplest way of approaching animation in ufbx is using `ufbx_evaluate_scene()`.
This is a heavyweight function that will return a new `ufbx_scene` where all animation is applied at a specified time.
You inspect the resulting scene as you would with a normally loaded `ufbx_scene`.

### Animation baking

Animation in an FBX file can be baked into a simpler format using `ufbx_bake_anim()`.
This function converts transform animation into simple linearly interpolated tracks:
translation, quaternion rotation, and scale.
Non-transform properties are also baked into linearly interpolated keyframes.

The baking algorithm is fairly sophisticated and won't simply blindly resample all tracks.
However, cubic interpolation and Euler rotation has to be resampled into quaternions.
The resampling framerate can be controlled via `ufbx_bake_opts.resample_rate`.
Many exporters also resample the animation internally,
to avoid double resampling *ufbx* will not resample frequent keys,
defined by `ufbx_bake_opts.minimum_sample_rate`.

```c
// ufbx-doc-example: animation/bake-anim

void bake_animation(ufbx_scene *scene, ufbx_anim *anim)
{
    ufbx_baked_anim *bake = ufbx_bake_anim(scene, anim, NULL, NULL);
    assert(bake);

    for (size_t i = 0; i < bake->nodes.count; i++) {
        ufbx_baked_node *bake_node = &bake->nodes.data[i];
        ufbx_node *scene_node = scene->nodes.data[bake_node->typed_id];

        printf("  node %s:\n", scene_node->name.data);
        printf("    translation: %zu keys\n", bake_node->translation_keys.count);
        printf("    rotation: %zu keys\n", bake_node->rotation_keys.count);
        printf("    scale: %zu keys\n", bake_node->scale_keys.count);
    }

    ufbx_free_baked_anim(bake);
}

void bake_animations(ufbx_scene *scene)
{
    // Iterate over every animation stack (aka. clip/take) in the file
    for (size_t i = 0; i < scene->anim_stacks.count; i++) {
        ufbx_anim_stack *stack = scene->anim_stacks.data[i];
        printf("stack %s:\n", stack->name.data);
        bake_animation(scene, stack->anim);
    }
}
```

```cpp
// ufbx-doc-example: animation/bake-anim

void bake_animation(ufbx_scene *scene, ufbx_anim *anim)
{
    ufbx_baked_anim *bake = ufbx_bake_anim(scene, anim, NULL, NULL);
    assert(bake);

    for (const ufbx_baked_node &bake_node : bake->nodes) {
        ufbx_node *scene_node = scene->nodes[bake_node.typed_id];

        printf("  node %s:\n", scene_node->name.data);
        printf("    translation: %zu keys\n", bake_node.translation_keys.count);
        printf("    rotation: %zu keys\n", bake_node.rotation_keys.count);
        printf("    scale: %zu keys\n", bake_node.scale_keys.count);
    }

    ufbx_free_baked_anim(bake);
}

void bake_animations(ufbx_scene *scene)
{
    // Iterate over every animation stack (aka. clip/take) in the file
    for (ufbx_anim_stack *stack : scene->anim_stacks) {
        printf("stack %s:\n", stack->name.data);
        bake_animation(scene, stack->anim);
    }
}
```

```rust
// ufbx-doc-example: animation/bake-anim

fn bake_animation(scene: &ufbx::Scene, anim: &ufbx::Anim) {
    let bake = ufbx::bake_anim(scene, anim, ufbx::BakeOpts::default())
        .expect("expected to bake animation");

    for bake_node in &bake.nodes {
        let scene_node = &scene.nodes[bake_node.typed_id as usize];

        println!("  node {}:", scene_node.element.name);
        println!("    translation: {} keys", bake_node.translation_keys.len());
        println!("    rotation: {} keys", bake_node.rotation_keys.len());
        println!("    scale: {} keys", bake_node.scale_keys.len());
    }
}

fn bake_animations(scene: &ufbx::Scene) {
    for stack in &scene.anim_stacks {
        println!("stack {}:", stack.element.name);
        bake_animation(scene, &stack.anim);
    }
}
```

### Transform/property evaluation

*ufbx* also provides lower level evaluation functions for evaluating the state of individual objects at a specified time:

* `ufbx_evaluate_transform()` evaluates node's translation, quaternion rotation, and scale.
* `ufbx_evaluate_blend_weight()` evaluates blend shape weight.
* `ufbx_evaluate_prop()` evaluates an arbitrary FBX property value.
* `ufbx_evaluate_props()` evaluates all the propeties of an element.

Even lower level utilities allow you to evaluate the aspects of animations:

* `ufbx_evaluate_anim_value_real/@(ufbx_evaluate_anim_value_)vec2/@(ufbx_evaluate_anim_value_)vec3()` evaluates a `ufbx_anim_value`
* `ufbx_evaluate_curve()` evaluates a single `ufbx_anim_curve`
