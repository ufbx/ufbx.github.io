// $dep skinned_human.fbx
// $ cargo build
// $ cargo run --quiet -- skinned_human.fbx Female > result.obj

// -- EXAMPLE_SOURCE --

struct Bone {
    node_index: u32,
    geometry_to_bone: ufbx::Matrix,
}

// Non-indexed triangulated skinned mesh
struct Mesh {
    bones: Vec<Bone>,
    vertices: Vec<Vertex>,
} 

fn process_skinned_mesh(mesh: &ufbx::Mesh, skin: &ufbx::SkinDeformer) -> Mesh {
    let mut vertices: Vec<Vertex> = Vec::new();

    // Triangulate the mesh, using `get_skinned_vertex()` to fetch each index.
    let mut tri_indices = vec![0u32; mesh.max_face_triangles * 3];
    for face_index in 0..mesh.num_faces {
        let face = mesh.faces[face_index];

        // Triangulate the face into `tri_indices[]`.
        let num_tris = mesh.triangulate_face(&mut tri_indices, face);
        let num_tri_corners = (num_tris * 3) as usize;
        for &index in &tri_indices[..num_tri_corners] {
            vertices.push(get_skinned_vertex(mesh, skin, index as usize));
        }
    }
    assert!(vertices.len() == mesh.num_triangles * 3);

    // Create bone descriptions
    let bones = skin.clusters
        .iter()
        .map(|cluster| Bone{
            node_index: cluster.bone_node.as_ref().unwrap().element.typed_id,
            geometry_to_bone: cluster.geometry_to_bone,
        })
        .collect();

    Mesh{ bones, vertices }
}

fn matrix_add(dst: &mut ufbx::Matrix, src: &ufbx::Matrix, weight: f32) {
    // Rust bindings currently have no way of accessing columns/values
    // ergonomically..
    let weight = weight as f64;
    dst.m00 += src.m00 * weight;
    dst.m10 += src.m10 * weight;
    dst.m20 += src.m20 * weight;
    dst.m01 += src.m01 * weight;
    dst.m11 += src.m11 * weight;
    dst.m21 += src.m21 * weight;
    dst.m02 += src.m02 * weight;
    dst.m12 += src.m12 * weight;
    dst.m22 += src.m22 * weight;
    dst.m03 += src.m03 * weight;
    dst.m13 += src.m13 * weight;
    dst.m23 += src.m23 * weight;
}

// Example implementation that dumps the skinned mesh as an .obj file
fn dump_mesh(mesh: &Mesh, scene: &ufbx::Scene) {
    // Resolve skinning (mesh geometry to world matrices)
    let geometry_to_world: Vec<ufbx::Matrix> = mesh.bones
        .iter()
        .map(|bone| {
            let node = &scene.nodes[bone.node_index as usize];
            ufbx::matrix_mul(&node.node_to_world, &bone.geometry_to_bone)
        })
        .collect();

    // Process each vertex
    for vertex in &mesh.vertices {
        // Weighted sum of the influencing bone matrices
        // Rust inherits zero default from C, not identity as default
        let mut vertex_to_world = ufbx::Matrix::default();
        for i in 0..MAX_WEIGHTS {
            let bone_ix = vertex.bones[i] as usize;
            let weight = vertex.weights[i];
            matrix_add(&mut vertex_to_world, &geometry_to_world[bone_ix], weight);
        }

        let p = ufbx::transform_position(&vertex_to_world, vertex.position);
        println!("v {:.3} {:.3} {:.3}", p.x, p.y, p.z);
    }

    // Just a flat list of non-indexed triangle faces
    for tri in 0 .. mesh.vertices.len() / 3 {
        let i = tri * 3;
        println!("f {} {} {}", i + 1, i + 2, i + 3);
    }
}

fn main() {
    let path = std::env::args().nth(1).expect("expected .fbx path");
    let node_name = std::env::args().nth(2).expect("expected node name");

    let scene = ufbx::load_file(&path, Default::default()).expect("failed to load scene");
    let node = scene.find_node(&node_name).expect("could not find node");
    let mesh = node.mesh.as_ref().expect("node did not contain a mesh");
    let skin = mesh.skin_deformers.get(0).expect("no skin deformer in mesh");

    let result = process_skinned_mesh(mesh, skin);
    dump_mesh(&result, &scene);
}
