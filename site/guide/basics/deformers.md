---
title: Deformers
pageTitle: Deformers
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Deformers
  order: 3
---

Geometry can be morphed using various deformers, FBX supports three types of deformers:

- `ufbx_skin_deformer`: Bone-based skinning
- `ufbx_blend_deformer`: Blend shapes (aka morph targets) containing per-vertex offsets
- `ufbx_cache_deformer`: Overrides geometry data from a cache file containing per-frame positions

Note that unlike what we've previously gone over in [Geometry](/guide/basics/geometry), deformers
affect *vertices* so we need to use `ufbx_mesh.vertex_indices[]` to map from indices to vertices to
handle all of the above deformers.

## Skinning

Let's start off with skinning which means deforming a mesh with a control skeleton. A skin deformer
is defined via `ufbx_skin_deformer` found from `ufbx_mesh.skin_deformers[]`, usually you only have a single one per mesh.
Vertices are attached to bones via `ufbx_skin_cluster` elements (found in `ufbx_skin_deformer.clusters[]`).
Each cluster contains a list of bound vertices `ufbx_skin_cluster.vertices[]` and a matrix `@(ufbx_skin_cluster).geometry_to_bone` that maps from mesh-space to bone-space often called inverse bind matrix.
One vertex may be influenced by multiple clusters with varying weights defined via `ufbx_skin_cluster.weights[]`.

Often you want to know which bones influence a given vertex, so ufbx provides `ufbx_skin_deformer.vertices[]` and `@(ufbx_skin_deformer.)weights[]`
which contains per-vertex information about clusters and weights.

```c
// ufbx-doc-example: deformers/skin-vertex
Vector3 transform_vertex(ufbx_mesh *mesh, ufbx_skin_deformer *skin, uint32_t vertex)
{
    Vector3 result = Vector3_new(0.0f, 0.0f, 0.0f);
    float total_weight = 0.0f;

    ufbx_skin_vertex vert = skin->vertices.data[vertex];
    for (uint32_t i = 0; i < vert.num_weights; i++) {
        ufbx_skin_weight weight = skin->weights.data[vert.weight_begin + i];
        ufbx_skin_cluster *cluster = skin->clusters.data[weight.cluster_index];
        ufbx_node *node = cluster->bone_node;

        Matrix4 geometry_to_bone = Matrix4_ufbx(cluster->geometry_to_bone);
        Matrix4 bone_to_world = Matrix4_ufbx(node->node_to_world);
        Matrix4 geometry_to_world = Matrix4_mul(bone_to_world, geometry_to_bone);

        Vector3 local_vertex = Vector3_ufbx(mesh->vertices.data[vertex]);
        Vector3 world_vertex = Matrix4_transform_point(geometry_to_world, local_vertex);
        result = Vector3_add(result, Vector3_mul(world_vertex, (float)weight.weight));
        total_weight += (float)weight.weight;
    }

    return Vector3_mul(result, 1.0f / total_weight);
}
```

```cpp
// ufbx-doc-example: deformers/skin-vertex

Vector3 transform_vertex(ufbx_mesh *mesh, ufbx_skin_deformer *skin, uint32_t vertex)
{
    Vector3 result = { };
    float total_weight = 0.0f;

    ufbx_skin_vertex vert = skin->vertices[vertex];
    for (uint32_t i = 0; i < vert.num_weights; i++) {
        ufbx_skin_weight weight = skin->weights[vert.weight_begin + i];
        ufbx_skin_cluster *cluster = skin->clusters[weight.cluster_index];
        ufbx_node *node = cluster->bone_node;

        Matrix4 geometry_to_bone = cluster->geometry_to_bone;
        Matrix4 bone_to_world = node->geometry_to_world;
        Matrix4 geometry_to_world = bone_to_world * geometry_to_bone;

        Vector3 local_vertex = mesh->vertices[vertex];
        Vector3 world_vertex = Matrix4_transform_point(geometry_to_world, local_vertex);
        result += world_vertex * (float)weight.weight;
        total_weight += (float)weight.weight;
    }

    return result * (1.0f / total_weight);
}
```

```rust
// ufbx-doc-example: deformers/skin-vertex

fn transform_vertex(mesh: &ufbx::Mesh, skin: &ufbx::SkinDeformer, vertex: usize) -> Vec3 {
    let mut result = Vec3::ZERO;
    let mut total_weight: f32 = 0.0;

    let vert = skin.vertices[vertex];
    for i in 0..vert.num_weights {
        let weight = skin.weights[(vert.weight_begin + i) as usize];
        let cluster = &skin.clusters[weight.cluster_index as usize];
        let node = cluster.bone_node.as_ref().unwrap();

        let geometry_to_bone: Mat4 = cluster.geometry_to_bone.as_glam().as_mat4();
        let bone_to_world: Mat4 = node.geometry_to_world.as_glam().as_mat4();
        let geometry_to_world = bone_to_world * geometry_to_bone;

        let local_vertex: Vec3 = mesh.vertices[vertex].as_glam().as_vec3();
        let world_vertex = geometry_to_world.transform_point3(local_vertex);
        result += world_vertex * weight.weight as f32;
        total_weight += weight.weight as f32;
    }

    return result * (1.0 / total_weight);
}
```

Typically for real time applications you would want to compute the `geometry_to_world` matrix on the CPU
for each bone and transform vertices in a vertex shader based on the matrices.

Each vertex may be influenced by one or more bones, represented as `ufbx_skin_cluster` elements. 

<div class="doc-viewer doc-viewer-xtall">
<div data-dv-popout id="container-skinning" class="dv-inline">
<div class="dv-top">
{% include "viewer.md",
  id: "skinning",
  class: "dv-normal",
%}
</div>
</div>
</div>

<script>
viewerDescs["skinning"] = {
  scene: "/static/models/skinned_human.fbx",
  camera: {
    pitch: 30.0,
    yaw: -40.0,
    distance: 10,
    offset: { x: 0.0, y: 0.0, z: 0.0 },
  },
  outliner: {
    showDeformers: true,
  },
  props: {
    show: true,
  },
}
</script>

```c
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
    uint32_t bone_index[4];
    float bone_weight[4];
};

struct Bone {
    uint32_t node_index;
    Matrix geometry_to_bone;
};

struct Mesh {
    size_t num_bones;
    Bone *bones;

    size_t num_vertices;
    Vertex *vertices;
};

Mesh process_skinned_mesh(ufbx_mesh *mesh)
{
    assert(mesh->skin_deformers.count > 0);
    ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];

    Mesh result;

    result.num_vertices = mesh->num_triangles * 3;
    result.vertices = (Vertex*)calloc(result.num_vertices, sizeof(Vertex));

    result.num_bones = skin->clusters.count;
    result.bones = (Bone*)calloc(result.num_bones, sizeof(Bone));

    size_t num_tri_indices = mesh->max_face_triangles;
    uint32_t *tri_indices = (uint32_t*)calloc(num_tri_indices, sizeof(uint32_t));

    Vertex *dst = result->vertices;

    for (size_t bone_ix = 0; bone_ix < skin->clusters.count; bone_ix++) {
        ufbx_skin_cluster *cluster = skin->clusters.data[bone_ix];
        Bone *bone = result.bones[bone_ix];

        bone->node_index = cluster->bone_node->typed_id;
        bone->geometry_to_bone = cluster->geometry_to_bone;
    }

    for (size_t face_ix = 0; face_ix < mesh->faces.count; face_ix++) {
        ufbx_face face = mesh->faces.data[face_ix];
        uint32_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

        for (uint32_t ti = 0; ti < num_tris * 3; ti++) {
            uint32_t ix = face.index_begin + ti;

            dst->position = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
            dst->normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
            dst->uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, ix);

            uint32_t vertex_ix = mesh->vertex_indices.data[ix];

            // Note: This is slightly inefficient, we could precompute `bone_index[]`
            // and `bone_weight[]` for each vertex before triangulating the mesh.
            ufbx_skin_vertex skin_vertex = skin->vertices.data[vertex_ix];
            for (uint32_t bi = 0; bi < 4 && bi < skin_vertex.num_weights; bi++) {
                ufbx_skin_weight weight = skin->weights.data[skin_vertex.weight_begin + bi];
                
                dst->bone_index[bi] = weight.cluster_index;
                dst->bone_weight[bi] = (float)weight.weight;
            }

            // Advance to the next vertex
            ++dst;
        }
    }

    free(tri_indices);
    return result;
}
```

### Skinning modes

If you want to get more advanced FBX supports multiple skinning modes via `ufbx_skin_deformer.skinning_method`:

- `UFBX_SKINNING_METHOD_RIGID`: No blending, using only single bone per vertex
- `UFBX_SKINNING_METHOD_LINEAR`: Classic linear blend skinning
- `UFBX_SKINNING_METHOD_DUAL_QUATERNION`: Dual quaternion
- `UFBX_SKINNING_METHOD_BLENDED_DQ_LINEAR`: This is the spicy one: Every vertex contains an extra "Dual quaternion weight"
that blends between `@(UFBX_SKINNING_METHOD_)LINEAR` and `@(UFBX_SKINNING_METHOD_)DUAL_QUATERNION` modes.

## Blend shapes

Blend shapes contain per-vertex offsets that can be scaled by per-shape weights.
A single `ufbx_blend_deformer` can contain multiple "channels" (`ufbx_blend_channel`),
each channel contains a blend shape (`ufbx_blend_channel.target_shape`) that can be controlled via a weight.

A blend shape (`ufbx_blend_shape`) is like a cluster and contains a list of vertex indices and offsets.
Unlike skins ufbx does not provide straightforward vertex to blend offset mapping as blend shapes often
tend to be sparse, however the offsets are sorted so you can use `ufbx_get_blend_shape_vertex_offset()`
to efficiently query per-vertex offsets.

```c
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;

    Vector3 blend_offset[4];
};

struct Mesh {
    size_t num_vertices;
    Vertex *vertices;
};

Mesh process_blend_mesh(ufbx_mesh *mesh)
{
    assert(mesh->blend_deformers.count > 0);
    ufbx_blend_deformer *blend = mesh->blend_deformers.data[0];

    Mesh result;

    result.num_vertices = mesh->num_triangles * 3;
    result.vertices = (Vertex*)calloc(result.num_vertices, sizeof(Vertex));

    size_t num_tri_indices = mesh->max_face_triangles;
    uint32_t *tri_indices = (uint32_t*)calloc(num_tri_indices, sizeof(uint32_t));

    Vertex *dst = result->vertices;

    for (size_t face_ix = 0; face_ix < mesh->faces.count; face_ix++) {
        ufbx_face face = mesh->faces.data[face_ix];
        uint32_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

        for (uint32_t ti = 0; ti < num_tris * 3; ti++) {
            uint32_t ix = face.index_begin + ti;

            dst->position = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
            dst->normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
            dst->uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, ix);

            uint32_t vertex_ix = mesh->vertex_indices.data[ix];

            for (size_t ci = 0; ci < blend->channels.count; ci++) {
                ufbx_blend_channel *channel = blend->channels.data[i];
                ufbx_blend_shape *shape = channel->target_shape;

                ufbx_vec3 offset = ufbx_get_blend_shape_vertex_offset(shape, vertex_ix);
                dst->blend_offset[ci] = offset;
            }

            // Advance to the next vertex
            ++dst;
        }
    }

    free(tri_indices);
    return result;
}
```

### Intermediate blend shapes

FBX supports an advanced blend shape mode where a single `ufbx_blend_channel` may contain
multiple intermediate blend steps stored in `ufbx_blend_channel.keyframes[]`, for example
consider the following:

```c
ufbx_blend_channel.keyframes[] = {
    { .shape=shapeA, .target_weight=0.5 },
    { .shape=shapeB, .target_weight=1.0 },
}
```

This channel would interpolate from the neutral pose to `shapeA` between `[0.0, 0.5]`
and from `shapeA` to `shapeB` between `[0.5, 1.0]`.

## Geometry caches

Geometry caches are external files that contain per-frame snapshots of geometry data.
For ufbx to load the actual cache information you need to use `ufbx_load_opts.load_external_files`
or load the caches manually using `ufbx_load_geometry_cache()` afterwards.

```c
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;

    Vector3 blend_offset[4];
};

struct Mesh {
    size_t num_vertices;
    Vertex *vertices;

    size_t num_frames;
    Vector3 **frames;
};

Mesh process_cache_mesh(ufbx_mesh *mesh)
{
    assert(mesh->cache_deformers.count > 0);
    ufbx_cache_deformer *deformer = mesh->cache_deformers.data[0];
    ufbx_geometry_cache *cache = deformer->external_cache;
    assert(cache);

    Mesh result;

    result.num_vertices = mesh->num_triangles * 3;
    result.vertices = (Vertex*)calloc(result.num_vertices, sizeof(Vertex));

    result.num_frames = cache->frames.count;
    result.frames = (Vertex*)calloc(result.num_frames, sizeof(Vector3*));

    size_t num_temp_pos = result.num_vertices;
    ufbx_vec3 *temp_pos = calloc(num_temp, sizeof(ufbx_vec3));

    for (size_t frame_ix = 0; frame_ix < result.num_frames; frame_ix++) {
        Vector3 *frame = calloc(result.num_vertices, sizeof(Vector3));

        ufbx_read_geometry_cache_vec3(
            cache->frames.data[frame_ix],
            temp_pos, num_temp_pos, NULL);

        for (size_t i = 0; i < result.num_vertices; i++) {
            frame[i] = temp_pos[i];
        }

        result.frames[frame_ix] = frame;
    }

    free(temp_pos);
    return result;
}
```

## Built-in evaluation

If you only care about the result of the deformed geometry you don't need to care about the above details.
Setting `ufbx_load_opts.evaluate_skinning` or `ufbx_evaluate_opts.evaluate_skinning` will populate the
`ufbx_mesh.skinned_position` and `ufbx_mesh.skinned_normal` attributes which will contain the result of all deformations applied.
`ufbx_mesh.skinned_is_local` indicates whether these are in local or world space.
