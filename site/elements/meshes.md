---
title: Meshes
pageTitle: Meshes
layout: "layouts/guide"
eleventyNavigation:
  parent: Elements
  key: Meshes
  order: 2
---

Meshes (`ufbx_mesh`) contain the polygonal geometry data.

*ufbx* uses the following terminology:

* **Vertex:** Vertex with position, essentially matches a selectable vertex in a modeling program.
* **Index:** Combination of a vertex with attributes, such as UV, normal, or color.
* **Face:** Range of indices representing a planar face (triangle, quad, N-gon).

The same vertex may be referred to by multiple faces,
each reference having a different index,
allowing each face to define a unique UV coordinate for the same vertex.

```c
// ufbx-doc-example: nodes/draw-polygons

void draw_polygons(ufbx_mesh *mesh)
{
    for (size_t i = 0; i < mesh->faces.count; i++) {
        ufbx_face face = mesh->faces.data[i];
        begin_polygon();

        // Loop through the corners of the polygon.
        for (uint32_t corner = 0; corner < face.num_indices; corner++) {

            // Faces are defined by consecutive indices, one for each corner.
            uint32_t index = face.index_begin + corner;

            // Retrieve the position, normal and uv for the vertex.
            ufbx_vec3 position = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
            ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);
            ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, index);

            polygon_corner(position, normal, uv);
        }

        end_polygon();
    }
}
```

```cpp
// ufbx-doc-example: nodes/draw-polygons

void draw_polygons(ufbx_mesh *mesh)
{
    for (ufbx_face face : mesh->faces) {
        begin_polygon();

        // Loop through the corners of the polygon.
        for (uint32_t corner = 0; corner < face.num_indices; corner++) {

            // Faces are defined by consecutive indices, one for each corner.
            uint32_t index = face.index_begin + corner;

            // Retrieve the position, normal and uv for the vertex.
            ufbx_vec3 position = mesh->vertex_position[index];
            ufbx_vec3 normal = mesh->vertex_normal[index];
            ufbx_vec2 uv = mesh->vertex_uv[index];

            polygon_corner(position, normal, uv);
        }

        end_polygon();
    }
}
```

```rust
fn draw_polygons(mesh: &ufbx::Mesh)
{
    for face in &mesh.faces {
        begin_polygon();

        // Loop through the corners of the polygon.
        for corner in 0..face.num_indices {

            // Faces are defined by consecutive indices, one for each corner.
            let index = face.index_begin + corner;

            // Retrieve the position, normal and uv for the vertex.
            let position = mesh.vertex_position[index as usize];
            let normal = mesh.vertex_normal[index as usize];
            let uv = mesh.vertex_uv[index as usize];

            polygon_corner(position, normal, uv);
        }

        end_polygon();
    }
}
```