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
In the example below, you can see that both indices **3** and **4** refer to the same vertex **2**,
yet have different normals.

<div class="doc-viewer doc-viewer-tall">
<div data-dv-popout id="container-cube" class="dv-inline">
<div class="dv-top">
{% include "viewer.md",
  id: "cube",
  class: "dv-normal",
%}

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

Mesh data is stored in attributes, such as `ufbx_mesh.vertex_position`, `ufbx_mesh.vertex_normal`, and `ufbx_mesh.vertex_uv`.
Each attribute has its own indices (`ufbx_vertex_attrib.indices[]`) and values (`ufbx_vertex_attrib.values[]`).
Values for given index can be read from `data[indices[index]]`,
or alternatively using helpers like `ufbx_get_vertex_vec3()`.
C++ and Rust also support the shorthand `attrib[index]`.

Example below draws a mesh with a hypothetical immediate-mode polygon API:

```c
// ufbx-doc-example: meshes/draw-polygons

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
// ufbx-doc-example: meshes/draw-polygons

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
// ufbx-doc-example: meshes/draw-polygons

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

## Materials

A single FBX mesh may contain multiple materials for different parts.
`ufbx_mesh.face_material[]` provides indices to the materials,
which can be used to index into `ufbx_mesh.materials[]`.
However, to be fully correct, you should prefer `ufbx_node.materials[]` where possible,
as meshes may be instanced with different materials.

Game engines often need to split meshes on material boundaries.
To help with this case, *ufbx* stores material part information in `ufbx_mesh.material_parts[]`.
These contain information about the number of faces/triangles and a list of faces using the material.
Even if the mesh has no materials assigned, there will be a single material part for convenience.

## Example

Below is an example of a common use case: converting mesh data to a GPU-friendly indexed format.
The example below uses *ufbx* helper functions:
`ufbx_triangulate_face()` generates an array of indices for triangles in a face,
and `ufbx_generate_indices()` deduplicates an array of vertices.

```c
// ufbx-doc-example: meshes/mesh-parts

// GPU vertex representation.
// In practice you would need to use more compact custom vector types as
// by default `ufbx_real` is 64 bits wide.
typedef struct Vertex {
    ufbx_vec3 position;
    ufbx_vec3 normal;
    ufbx_vec2 uv;
} Vertex;

void convert_mesh_part(ufbx_mesh *mesh, ufbx_mesh_part *part)
{
    size_t num_triangles = part->num_triangles;
    Vertex *vertices = calloc(num_triangles * 3, sizeof(Vertex));
    size_t num_vertices = 0;

    // Reserve space for the maximum triangle indices.
    size_t num_tri_indices = mesh->max_face_triangles * 3;
    uint32_t *tri_indices = calloc(num_tri_indices, sizeof(uint32_t));

    // Iterate over each face using the specific material.
    for (size_t face_ix = 0; face_ix < part->num_faces; face_ix++) {
        ufbx_face face = mesh->faces.data[part->face_indices.data[face_ix]];

        // Triangulate the face into `tri_indices[]`.
        uint32_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

        // Iterate over each triangle corner contiguously.
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];

            Vertex *v = &vertices[num_vertices++];
            v->position = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
            v->normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);
            v->uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, index);
        }
    }

    // Should have written all the vertices.
    free(tri_indices);
    assert(num_vertices == num_triangles * 3);

    // Generate the index buffer.
    ufbx_vertex_stream streams[1] = {
        { vertices, num_vertices, sizeof(Vertex) },
    };
    size_t num_indices = num_triangles * 3;
    uint32_t *indices = calloc(num_indices, sizeof(uint32_t));

    // This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
    // indices are written in `indices[]` and the number of unique vertices is returned.
    num_vertices = ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

    create_vertex_buffer(vertices, num_vertices);
    create_index_buffer(indices, num_indices);

    free(indices);
    free(vertices);
}
```

```cpp
// ufbx-doc-example: meshes/mesh-parts

// GPU vertex representation.
// In practice you would need to use more compact custom vector types as
// by default `ufbx_real` is 64 bits wide.
struct Vertex {
    ufbx_vec3 position;
    ufbx_vec3 normal;
    ufbx_vec2 uv;
};

void convert_mesh_part(ufbx_mesh *mesh, ufbx_mesh_part *part)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> tri_indices;
    tri_indices.resize(mesh->max_face_triangles * 3);

    // Iterate over each face using the specific material.
    for (uint32_t face_index : part->face_indices) {
        ufbx_face face = mesh->faces[face_index];

        // Triangulate the face into `tri_indices[]`.
        uint32_t num_tris = ufbx_triangulate_face(
            tri_indices.data(), tri_indices.size(), mesh, face);

        // Iterate over each triangle corner contiguously.
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];

            Vertex v;
            v.position = mesh->vertex_position[index];
            v.normal = mesh->vertex_normal[index];
            v.uv = mesh->vertex_uv[index];
            vertices.push_back(v);
        }
    }

    // Should have written all the vertices.
    assert(vertices.size() == part->num_triangles * 3);

    // Generate the index buffer.
    ufbx_vertex_stream streams[1] = {
        { vertices.data(), vertices.size(), sizeof(Vertex) },
    };
    std::vector<uint32_t> indices;
    indices.resize(part->num_triangles * 3);

    // This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
    // indices are written in `indices[]` and the number of unique vertices is returned.
    size_t num_vertices = ufbx_generate_indices(
        streams, 1, indices.data(), indices.size(), nullptr, nullptr);

    // Trim to only unique vertices.
    vertices.resize(num_vertices);

    create_vertex_buffer(vertices.data(), vertices.size());
    create_index_buffer(indices.data(), indices.size());
}
```

```rust
// ufbx-doc-example: meshes/mesh-parts

// GPU vertex representation.
// In practice you would need to use more compact custom vector types as
// by default `ufbx::Real` is 64 bits wide.
#[repr(C)]
#[derive(Clone, Copy)]
struct Vertex {
    position: ufbx::Vec3,
    normal: ufbx::Vec3,
    uv: ufbx::Vec2,
}

fn convert_mesh_part(mesh: &ufbx::Mesh, part: &ufbx::MeshPart) {
    let mut vertices: Vec<Vertex> = Vec::new();
    let mut tri_indices = vec![0u32; mesh.max_face_triangles * 3];

    // Iterate over each face using the specific material.
    for &face_index in &part.face_indices {
        let face = mesh.faces[face_index as usize];

        // Triangulate the face into `tri_indices[]`.
        let num_tris = mesh.triangulate_face(&mut tri_indices, face);
        let num_tri_corners = (num_tris * 3) as usize;

        // Iterate over each triangle corner contiguously.
        for &index in &tri_indices[..num_tri_corners] {
            vertices.push(Vertex {
                position: mesh.vertex_position[index as usize],
                normal: mesh.vertex_normal[index as usize],
                uv: mesh.vertex_uv[index as usize],
            });
        }
    }

    // Should have written all the vertices.
    assert!(vertices.len() == part.num_triangles * 3);

    // Generate the index buffer.
    let mut streams = [
        ufbx::VertexStream::new(&mut vertices),
    ];
    let mut indices = vec![0u32; part.num_triangles * 3];

    // This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
    // indices are written in `indices[]` and the number of unique vertices is returned.
    let num_vertices = ufbx::generate_indices(
            &mut streams, &mut indices, ufbx::AllocatorOpts::default())
        .expect("expected to generate indices");

    // Trim to only unique vertices.
    vertices.truncate(num_vertices);

    create_vertex_buffer(&vertices);
    create_index_buffer(&indices);
}
```

## Attributes

In addition to the attributes above FBX meshes may contain some other attributes.
Most attributes are defined per vertex (often, per index) with the `vertex_` prefix,
but there is also some data defined for each face and edge.
Edges are optional and defined between two indices in `ufbx_mesh.edges[]`.

- For each index, indexable up to `ufbx_mesh.num_indices`:
    - `ufbx_mesh.vertex_position`: Positions of the vertices
    - `ufbx_mesh.vertex_normal`: Normal vector
    - `ufbx_mesh.vertex_tangent`: Tangent space UV.x
    - `ufbx_mesh.vertex_bitangent`: Tangent space UV.y
    - `ufbx_mesh.vertex_uv`: Texture coordinate (first set, see `ufbx_mesh.color_sets` for rest)
    - `ufbx_mesh.vertex_color`: Vertex color (first set, see `ufbx_mesh.color_sets` for rest)
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
