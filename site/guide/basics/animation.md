---
title: Animation
pageTitle: Animation
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Animation
  order: 4
---

FBX scenes can contain multiple animations at a few different levels: animation
stacks (`ufbx_anim_stack`) represent unique animations within the file. For
example Blender exports Actions like `"Idle"` or `"Run"` as stacks. These stacks
may contain multiple layers (`ufbx_anim_layer`) that are blended together,
where each layer contains a list of animated properties.

ufbx can take care of this at multiple levels, you can either evaluate whole
stacks, individual layers, or dive deep and interpret the animation data yourself
using `ufbx_anim_curve`, though then you must be prepared to deal with the
peculiarities of FBX (which ufbx tries hard to shield you from).

## Evaluation

ufbx represents evaluatable animations as `ufbx_anim` descriptors, you can use predefined
animations in `ufbx_anim_stack.anim` and `ufbx_anim_layer.anim` for evaluating
stacks/layers. Scenes also contain a default animation `ufbx_scene.anim` that
you can use if you care only about a single animation stack.

For advanced use you can create customized `ufbx_anim` instances using `ufbx_create_anim()`,
which lets you define arbitrary layers and weights, and to override properties.

Note the animation timelines can often be offset, ie. they don't start at frame
0/1. You should consult `ufbx_anim.time_begin` and `@(ufbx_anim.)time_end` for
the time boundaries.

### Whole-scene evaluation

The simplest way of approaching animation in ufbx is using `ufbx_evaluate_scene()`.
This is a heavyweight function that will return a new `ufbx_scene` where all animation
is applied at a specified time. Then you can just inspect the result as you would with
a normally loaded `ufbx_scene`. This approach can be useful for example for rendering
a picture of a specific frame, where you don't care about the animation data itself.

### Transform evaluation

Animating transformed objects (especially bones that control skinning) is one of the
primary use cases for animated FBX files. ufbx provides the utility function `ufbx_evaluate_transform()`
that evaluates node transformation at a specified time. I highly recommend sampling transforms
using this function, as the internal FBX animation representation is quite complex and cumbersome
(you'll need to deal with animated Euler angles with arbitrary rotation orders, pre/post rotations, pivots, etc),
we'll look into how to do it manually in an advanced part of the guide.

// TODO: Resampling detection
