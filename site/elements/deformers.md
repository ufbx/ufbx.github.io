---
title: Deformers
pageTitle: Deformers
layout: "layouts/guide"
eleventyNavigation:
  parent: Elements
  key: Deformers
  order: 4
---

Mesh deformation in FBX is implemented via deformers.
*ufbx* supports three types of FBX deformers:

* `ufbx_skin_deformer` represents classic skinning, where vertices are bound to bones with weights
* `ufbx_blend_deformer` offsets vertices based on blend shapes (aka morph targets)
* `ufbx_cache_deformer` supports replacing the mesh data from a disk cache

Deformers are connected to meshes, eg. `ufbx_mesh.skin_deformers[]` or `ufbx_mesh.blend_deformers[]`.
If you need the exact order of deformers `ufbx_mesh.all_deformers[]` contains all deformers in an untyped list.

## Skin deformers

Skeletons in FBX are composed of standard nodes (`ufbx_node`) which often have a bone attribute (`ufbx_bone`).
The bone attribute is not necessary for skinning,
but may be useful for visualization or determining skeletons from the file.

Bone influences are defined in clusters (`ufbx_skin_cluster`), which binds the mesh to a bone.
`ufbx_skin_cluster.geometry_to_bone` represents the transformation from mesh geometry space to local bone space,
often referred to as the "inverse bind matrix".
The vertices affected by a cluster are defined in `ufbx_skin_cluster.vertices[]` and `@(ufbx_skin_cluster).weights[]`.

Resolving which bones affect a given vertex is a commonly needed operation though,
so *ufbx* provides an alternative utility to the above for it:
`ufbx_skin_deformer.vertices[]` and `@(ufbx_skin_deformer).weights[]` contain per-vertex weight information.
These weights are sorted by decreasing influence, so if you support a limited amount of weights (commonly 4 or 8),
you can only take the first `N` weights per vertex.

```c
// ufbx-doc-example: deformers/skin-weights

#define MAX_WEIGHTS 4

typedef struct Vertex {
    ufbx_vec3 position;
    ufbx_vec3 normal;
    float weights[MAX_WEIGHTS];
    uint32_t bones[MAX_WEIGHTS];
} Vertex;

Vertex get_skinned_vertex(ufbx_mesh *mesh, ufbx_skin_deformer *skin, size_t index)
{
    Vertex v = { 0 };
    v.position = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
    v.normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);

    // NOTE: This calculation below is the same for each `vertex`, we could
    // precalculate these up to `mesh->num_vertices`, and just load the results
    uint32_t vertex = mesh->vertex_indices.data[index];

    ufbx_skin_vertex skin_vertex = skin->vertices.data[vertex];
    size_t num_weights = skin_vertex.num_weights;
    if (num_weights > MAX_WEIGHTS) {
        num_weights = MAX_WEIGHTS;
    }

    float total_weight = 0.0f;
    for (size_t i = 0; i < num_weights; i++) {
        ufbx_skin_weight skin_weight = skin->weights.data[skin_vertex.weight_begin + i];
        v.bones[i] = skin_weight.cluster_index;
        v.weights[i] = (float)skin_weight.weight;
        total_weight += (float)skin_weight.weight;
    }

    // FBX does not guarantee that skin weights are normalized, and we may even
    // be dropping some, so we must renormalize them.
    for (size_t i = 0; i < num_weights; i++) {
        v.weights[i] /= total_weight;
    }

    return v;
}
```

```cpp
// ufbx-doc-example: deformers/skin-weights

#define MAX_WEIGHTS 4

typedef struct Vertex {
    ufbx_vec3 position;
    ufbx_vec3 normal;
    float weights[MAX_WEIGHTS];
    uint32_t bones[MAX_WEIGHTS];
} Vertex;

Vertex get_skinned_vertex(ufbx_mesh *mesh, ufbx_skin_deformer *skin, size_t index)
{
    Vertex v = { 0 };
    v.position = mesh->vertex_position[index];
    v.normal = mesh->vertex_normal[index];

    // NOTE: This calculation below is the same for each `vertex`, we could
    // precalculate these up to `mesh->num_vertices`, and just load the results.
    uint32_t vertex = mesh->vertex_indices[index];

    ufbx_skin_vertex skin_vertex = skin->vertices[vertex];
    size_t num_weights = skin_vertex.num_weights;
    if (num_weights > MAX_WEIGHTS) {
        num_weights = MAX_WEIGHTS;
    }

    float total_weight = 0.0f;
    for (size_t i = 0; i < num_weights; i++) {
        ufbx_skin_weight skin_weight = skin->weights[skin_vertex.weight_begin + i];
        v.bones[i] = skin_weight.cluster_index;
        v.weights[i] = (float)skin_weight.weight;
        total_weight += (float)skin_weight.weight;
    }

    // FBX does not guarantee that skin weights are normalized, and we may even
    // be dropping some, so we must renormalize them.
    for (size_t i = 0; i < num_weights; i++) {
        v.weights[i] /= total_weight;
    }

    return v;
}
```

```rust
// ufbx-doc-example: deformers/skin-weights

const MAX_WEIGHTS: usize = 4;

#[derive(Clone, Copy, Default)]
struct Vertex {
    position: ufbx::Vec3,
    normal: ufbx::Vec3,
    weights: [f32; MAX_WEIGHTS],
    bones: [u32; MAX_WEIGHTS],
}

fn get_skinned_vertex(mesh: &ufbx::Mesh, skin: &ufbx::SkinDeformer, index: usize) -> Vertex {
    let mut v = Vertex{
        position: mesh.vertex_position[index],
        normal: mesh.vertex_normal[index],
        ..Default::default()
    };

    // NOTE: This calculation below is the same for each `vertex`, we could
    // precalculate these up to `mesh->num_vertices`, and just load the results.
    let vertex = mesh.vertex_indices[index] as usize;

    let skin_vertex = skin.vertices[vertex];
    let num_weights = (skin_vertex.num_weights as usize).min(MAX_WEIGHTS);

    let mut total_weight: f32 = 0.0;
    for i in 0..num_weights {
        let skin_weight = skin.weights[skin_vertex.weight_begin as usize + i];
        v.bones[i] = skin_weight.cluster_index;
        v.weights[i] = skin_weight.weight as f32;
        total_weight += skin_weight.weight as f32;
    }

    // FBX does not guarantee that skin weights are normalized, and we may even
    // be dropping some, so we must renormalize them.
    for i in 0..num_weights {
        v.weights[i] /= total_weight;
    }

    v
}
```

