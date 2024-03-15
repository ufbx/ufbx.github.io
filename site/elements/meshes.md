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

Mesh data is stored in attributes, such as `ufbx_mesh.vertex_position`, `ufbx_mesh.vertex_normal`, and `ufbx_mesh.vertex_uv`.
Each attribute has its own indices (`ufbx_vertex_attrib.indices[]`) and data (`ufbx_vertex_attrib.data[]`).
Values for given index can be read from `data[indices[index]]`,
or alternatively using helpers like `ufbx_get_vertex_vec3()`.
C++ and Rust also support the shorthand `attrib[index]`.

Example below draws a mesh with a hypotetical immediate-mode polygon API:

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

```c
// ufbx-doc-example: meshes/mesh-parts

typedef struct {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
} Vertex;

typedef struct {
    VertexBuffer vertex_buffer;
    IndexBuffer index_buffer;
} MeshPart;

MeshPart convert_mesh_part(ufbx_mesh *mesh, ufbx_mesh_part *part)
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

            // Convert ufbx vertex into custom types.
            Vertex *v = &vertices[num_vertices++];
            v->position = Vector3_ufbx(ufbx_get_vertex_vec3(&mesh->vertex_position, index));
            v->normal = Vector3_ufbx(ufbx_get_vertex_vec3(&mesh->vertex_normal, index));
            v->uv = Vector2_ufbx(ufbx_get_vertex_vec2(&mesh->vertex_uv, index));
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

    MeshPart result;
    result.vertex_buffer = create_vertex_buffer(vertices, num_vertices);
    result.index_buffer = create_index_buffer(indices, num_indices);

    free(indices);
    free(vertices);

    return result;
}
```

```cpp
// ufbx-doc-example: meshes/convert-mesh-part

struct Vertex {
    ufbx_vec3 position;
    ufbx_vec3 normal;
    ufbx_vec2 uv;
};

void convert_mesh_part(ufbx_mesh *mesh, ufbx_mesh_part *part)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> tri_indices;
    tri_indices.resize(mesh->max_face_triangles);

    // Iterate over each face using the specific material.
    for (uint32_t face_index : part->face_indices) {
        ufbx_face face = mesh->faces[face_index];

        // Triangulate the face into `tri_indices[]`.
        uint32_t num_tris = ufbx_triangulate_face(
            tri_indices.data(), tri_indices.size(), mesh, face);

        // Iterate over each triangle corner contiguously.
        for (size_t i = 0; i < num_tris * 3; i++) {
            uint32_t index = tri_indices[i];

            // Write the vertex. In actual implementation you'd want to convert to custom
            // vector types or other more compact formats.
            Vertex v;
            v.position = mesh->vertex_position[index];
            v.normal = mesh->vertex_normal[index];
            v.uv = mesh->vertex_uv[index];
            vertices.push_back(v);
        }
    }

    // Should have written all the vertices.
    assert(vertices.size() == mesh->num_triangles * 3);

    // Generate the index buffer.
    ufbx_vertex_stream streams[1] = {
        { vertices.data(), vertices.size(), sizeof(Vertex) },
    };
    std::vector<uint32_t> indices;
    indices.resize(mesh->num_triangles * 3);

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
// ufbx-doc-example: meshes/convert-mesh-part

#[repr(C)]
struct Vertex {
    position: ufbx::Vec3,
    normal: ufbx::Vec3,
    uv: ufbx::Vec2,
};

fn convert_mesh_part(mesh: &ufbx::Mesh, part: &ufbx::MeshPart) {
    let mut vertices: Vec<Vertex> = Vec::new();
    let mut tri_indices = vec![0u32; mesh.max_face_triangles];

    // Iterate over each face using the specific material.
    for face_index in part.face_indices {
        let face = mesh.faces[face_index];

        // Triangulate the face into `tri_indices[]`.
        let num_tris = mesh.triangulate_face(&mut tri_indices, face);

        // Iterate over each triangle corner contiguously.
        for index in tri_indices[0 .. num_tris*3] {
            // Write the vertex. In actual implementation you'd want to convert to custom
            // vector types or other more compact formats.
            vertices.push(Vertex {
                position: mesh.vertex_position[index],
                normal: mesh.vertex_normal[index],
                uv: mesh.vertex_uv[index],
            });
        }
    }

    // Should have written all the vertices.
    assert!(vertices.len() == mesh.num_triangles * 3);

    // Generate the index buffer.
    let streams = [
        ufbx::VertexStream::new(&mut vertices),
    ];
    let mut indices = vec![0u32, mesh.num_triangles * 3];
    indices.resize();

    // This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
    // indices are written in `indices[]` and the number of unique vertices is returned.
    let num_vertices = ufbx_generate_indices(streams, indices, AllocatorOpts::default())
        .expect("expected to generate indices");

    // Trim to only unique vertices.
    vertices.resize(num_vertices);

    create_vertex_buffer(vertices);
    create_index_buffer(indices);
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
