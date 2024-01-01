---
title: Geometry
pageTitle: Geometry
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Geometry
  order: 1
---

Meshes (`ufbx_mesh`) are built from a list of polygons, also called faces
(`ufbx_face`). Each face contains a contiguous range of *indices*, for example
a mesh with two quads would contain the faces:

```
{ index_begin=0, num_indices=4 } // 0,1,2,3
{ index_begin=4, num_indices=4 } // 4,5,6,7
```

The geometry data itself is stored in attributes such as position, normal, UV,
color, etc. You can treat each attribute as a flat[^1] list of values defined for
each *index* (ie. polygon corner). So for the above example we would have positions,
UVs, normals ranging from 0 to 7: 0-3 belonging to the first face, 4-7 to the second one.

You might wonder why the attributes are not defined per vertex and then indexed by
the polygons (via `ufbx_mesh.vertex_indices[]`). The issue is that what we call a *vertex*
refers to essentially a selectable point in a 3D modeling program, if this point would lie
on an UV seam or a sharp corner then it would have two different UVs or normals. We'll get
back to vertices and why they matter when we get to deformation and skinning, but feel free
to ignore them for now.

```c

void draw_polygons(ufbx_mesh *mesh)
{
    for (size_t i = 0; i < mesh->faces.count; i++) {
        ufbx_face face = mesh->faces.data[i];

        begin_polygon();

        // Loop through the corners of the polygon.
        for (uint32_t i = 0; i < face.num_indices; i++) {
            uint32_t index = face.index_begin + i;

            // Retrieve the position and normal for the vertex.
            ufbx_vec3 position = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
            ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);

            polygon_corner(position, normal);
        }

        end_polygon();
    }
}
```

[^1]: Internally these lists are indexed (as `ufbx_vertex_attrib.values[ufbx_vertex_attrib.indices[index]]`). This
saves some space as you don't have to store duplicated values, but it can for example represent UV topology in some cases.

## Materials

One mesh may contain multiple materials for different faces, `ufbx_mesh.face_material`
provides an index into `ufbx_mesh.materials[]`. If the mesh is instanced different instances
may use different materials for each slot defined by `ufbx_node.materials[]`.

As renderers often need to split meshes along materials ufbx provides `ufbx_mesh.material_parts`,
containing for example `ufbx_mesh_part.face_indices[]`, that can be used to list of all faces using a specific material.

## Triangulation

Often you don't want to deal with arbitrarily sized triangles in meshes. ufbx has a utility function
`ufbx_triangulate_face()` that you can use to split a face into multiple triangles. If you want to
be able to handle any mesh make sure to use `ufbx_mesh.max_face_triangles` to determine the size for the output.

```c

void output_triangle(ufbx_vec3 a, ufbx_vec3 b, ufbx_vec3 c);

void list_triangles(ufbx_mesh *mesh)
{
    uint32_t indices[64];

    for (size_t i = 0; i < mesh->faces.count; i++) {
        ufbx_face face = mesh->faces.data[i];

        uint32_t num_triangles = ufbx_triangulate_face(indices, 64, mesh, face);

        for (uint32_t tri_ix = 0; tri_ix < num_triangles; tri_ix++) {
            uint32_t a = indices[tri_ix*3 + 0];
            uint32_t b = indices[tri_ix*3 + 1];
            uint32_t c = indices[tri_ix*3 + 2];

            ufbx_vec3 pos_a = ufbx_get_vertex_vec3(&mesh->vertex_position, a);
            ufbx_vec3 pos_b = ufbx_get_vertex_vec3(&mesh->vertex_position, b);
            ufbx_vec3 pos_c = ufbx_get_vertex_vec3(&mesh->vertex_position, c);

            output_triangle(pos_a, pos_b, pos_c);
        }
    }
}
```

<div class="doc-viewer doc-viewer-xtall">
<div data-dv-popout id="container-cube" class="dv-inline">
<div class="dv-top">
{% include "viewer.md",
  id: "cube",
  class: "dv-normal",
%}

<div class="g-inline">

Below you can see the vertices of the selected *mesh*. Notice that for example indices
**3** and **4** refer to the same vertex **2** but they still have different normals. You
can see this visualized if you hover over those lines.

</div>

<div data-viewer-id="cube" class="dv-vertices" style="min-height: 14rem"></div>
</div>
</div>
</div>

<script>
viewerDescs["cube"] = {
  scene: "/static/models/cube.fbx",
  camera: {
    pitch: 30.0,
    yaw: -40.0,
    distance: 10,
    offset: { x: 0.0, y: 0.0, z: 0.0 },
  },
  outliner: {
    showMaterials: true,
  },
  selectedNode: 2,
  selectedElement: 1,
}
</script>

You can retrieve the geometry data from `ufbx_mesh` through vertex attributes.

```c

void list_vertices(ufbx_mesh *mesh)
{
    for (size_t i = 0; i < mesh->faces.count; i++) {
        ufbx_face face = mesh->faces.data[i];

        begin_polygon();

        // Loop through the corners of the polygon.
        for (uint32_t corner = 0; corner < face.num_indices; corner++) {

            // Faces are defined by consecutive indices, one for each corner.
            uint32_t index = face.index_begin + corner;

            // Retrieve the position and normal for the vertex.
            ufbx_vec3 position = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
            ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);

            polygon_corner(position, normal);
        }

        end_polygon();
    }
}
```

```cpp
void list_vertices(ufbx_mesh *mesh)
{
    for (ufbx_face face : mesh->faces) {
        begin_polygon();

        // Loop through the corners of the polygon.
        for (uint32_t corner = 0; corner < face.num_indices; corner++) {

            // Faces are defined by consecutive indices, one for each corner.
            uint32_t index = face.index_begin + corner;

            // Retrieve the position and normal for the vertex.
            ufbx_vec3 position = mesh->vertex_position[index];
            ufbx_vec3 normal = mesh->vertex_normal[index];

            polygon_corner(position, normal);
        }

        end_polygon();
    }
}
```

```rust
fn list_vertices(mesh: &ufbx::Mesh)
{
    for face in &mesh.faces {
        begin_polygon();

        // Loop through the corners of the polygon.
        for corner in 0..face.num_indices {

            // Faces are defined by consecutive indices, one for each corner.
            let index = face.index_begin + corner;

            // Retrieve the position and normal for the vertex.
            let position = mesh.vertex_position[index as usize];
            let normal = mesh.vertex_normal[index as usize];

            polygon_corner(position, normal);
        }

        end_polygon();
    }
}
```

In many cases you can't do much with arbitrarily large polygons so meshes need to be triangulated.
You can use the `ufbx_triangulate_face()` utility function to chop a polygon into triangles.

```cpp

void output_triangle(ufbx_vec3 a, ufbx_vec3 b, ufbx_vec3 c);

void list_triangles(ufbx_mesh *mesh)
{
    std::vector<uint32_t> indices;
    indices.resize(mesh->max_face_triangles * 3);

    for (ufbx_face face : mesh->faces) {
        uint32_t num_triangles = ufbx_triangulate_face(
            indices.data(), indices.size(), mesh, face);

        for (uint32_t tri_ix = 0; tri_ix < num_triangles; tri_ix++) {
            uint32_t a = indices[tri_ix*3 + 0];
            uint32_t b = indices[tri_ix*3 + 1];
            uint32_t c = indices[tri_ix*3 + 2];

            ufbx_vec3 pos_a = mesh->vertex_position[a];
            ufbx_vec3 pos_b = mesh->vertex_position[b];
            ufbx_vec3 pos_c = mesh->vertex_position[c];

            output_triangle(pos_a, pos_b, pos_c);
        }
    }
}
```

```rust

fn list_triangles(mesh: &ufbx::Mesh)
{
    let mut indices: Vec<u32> = Vec::new();
    indices.resize(mesh.max_face_triangles * 3, 0);

    for face in &mesh.faces {
        let num_triangles = ufbx::triangulate_face(&mut indices, mesh, *face) as usize;
        for tri_ix in 0..num_triangles {
            let a = indices[tri_ix*3 + 0];
            let b = indices[tri_ix*3 + 1];
            let c = indices[tri_ix*3 + 2];

            let pos_a = mesh.vertex_position[a as usize];
            let pos_b = mesh.vertex_position[b as usize];
            let pos_c = mesh.vertex_position[c as usize];

            output_triangle(pos_a, pos_b, pos_c);
        }
    }
}
```

## Attributes

Here's a brief summary of all the geometry data that `ufbx_mesh` contains:

- For each index, indexable up to `ufbx_mesh.num_indices`:
    - `ufbx_mesh.vertex_position`: Positions of the vertices
    - `ufbx_mesh.vertex_normal`: Normal vector
    - `ufbx_mesh.vertex_tangent`: Tangent space UV.x
    - `ufbx_mesh.vertex_bitangent`: Tangent space UV.y
    - `ufbx_mesh.vertex_uv`: Texture coordinate *(first set, see `ufbx_mesh.color_sets` for rest)*
    - `ufbx_mesh.vertex_color`: Vertex color *(first set, see `ufbx_mesh.color_sets` for rest)*
    - `ufbx_mesh.vertex_crease`: Vertex crease for subdivision
- For each face, indexable up to `ufbx_mesh.num_faces`:
    - `ufbx_mesh.face_material`: Per-face material
    - `ufbx_mesh.face_group`: Polygon groups
    - `ufbx_mesh.face_smoothing`: Boolean flag whether generated normals should be smooth
    - `ufbx_mesh.face_hole`: If true the polygon should be treated as a hole
- For each edge, indexable up to `ufbx_mesh.num_edges`:
    - `ufbx_mesh.edge_smoothing`: Smooth flag for generating normals
    - `ufbx_mesh.edge_visibility`: Edge visibility for editing
    - `ufbx_mesh.edge_crease`: Edge crease for subdivision
