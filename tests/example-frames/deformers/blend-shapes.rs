// $dep mushnub_evolved.fbx
// $ cargo build
// $ cargo run --quiet -- mushnub_evolved.fbx Blink=0.5 Smile=0.7 Spikes=0.4 > result.obj

// `Vertex::normal` is not used in this example
#![allow(unused)]

// -- EXAMPLE_SOURCE --

struct BlendShape {
    name: String,
}

// Non-indexed triangulated skinned mesh
struct Mesh {
    blends: Vec<BlendShape>,
    vertices: Vec<Vertex>,
} 

fn process_blend_mesh(mesh: &ufbx::Mesh, blend: &ufbx::BlendDeformer) -> Mesh {
    let mut vertices: Vec<Vertex> = Vec::new();

    // Triangulate the mesh, using `get_blend_vertex()` to fetch each index.
    let mut tri_indices = vec![0u32; mesh.max_face_triangles * 3];
    for face_index in 0..mesh.num_faces {
        let face = mesh.faces[face_index];

        // Triangulate the face into `tri_indices[]`.
        let num_tris = mesh.triangulate_face(&mut tri_indices, face);
        let num_tri_corners = (num_tris * 3) as usize;
        for &index in &tri_indices[..num_tri_corners] {
            vertices.push(get_blend_vertex(mesh, blend, index as usize));
        }
    }
    assert!(vertices.len() == mesh.num_triangles * 3);

    // Create bone descriptions
    let blends = blend.channels
        .iter()
        .map(|channel| BlendShape{
            name: channel.element.name.to_owned(),
        })
        .take(MAX_BLENDS)
        .collect();

    Mesh{ blends, vertices }
}

struct Control {
    name: String,
    weight: f32,
}

// Example implementation that dumps the skinned mesh as an .obj file
fn dump_mesh(mesh: &Mesh, node: &ufbx::Node, controls: &[Control], vertex_offset: &mut usize) {
    // Preload the control weights for the mesh blend shapes
    let mut weights = [0.0f32; MAX_BLENDS];
    for (i, blend) in mesh.blends.iter().enumerate() {
        for control in controls {
            if control.name == blend.name {
                weights[i] = control.weight;
            }
        }
    }

    // Process each vertex
    for vertex in &mesh.vertices {
        let mut p = vertex.position;
        for i in 0..MAX_BLENDS {
            let offset = vertex.blend_offsets[i];
            let weight = weights[i] as f64;
            p.x += offset.x * weight;
            p.y += offset.y * weight;
            p.z += offset.z * weight;
        }

        // The blend offsets are applied in local space, transform to global space
        p = ufbx::transform_position(&node.geometry_to_world, p);

        println!("v {:.3} {:.3} {:.3}", p.x, p.y, p.z);
    }

    // Just a flat list of non-indexed triangle faces
    for tri in 0 .. mesh.vertices.len() / 3 {
        let i = *vertex_offset + tri * 3;
        println!("f {} {} {}", i + 1, i + 2, i + 3);
    }
    *vertex_offset += mesh.vertices.len();
}

fn main() {
    let path = std::env::args().nth(1).expect("expected .fbx path");

    let scene = ufbx::load_file(&path, Default::default()).expect("failed to load scene");

    // Parse the blend shape controls, of form "Name=0.3"
    let controls: Vec<Control> = std::env::args()
        .skip(2)
        .map(|arg| {
            let (name, value) = arg.split_once('=')
                .expect("expected a Blend=(amount) argument");
            let name = name.to_owned();
            let weight: f32 = value.parse()
                .expect("expected blend value to be a float");
            Control{ name, weight }
        })
        .collect();

    let mut vertex_offset = 0;
    for mesh in &scene.meshes {
        if mesh.blend_deformers.len() == 0 {
            continue;
        }
        let blend = &mesh.blend_deformers[0];

        let result = process_blend_mesh(mesh, blend);

        for node in &mesh.element.instances {
            dump_mesh(&result, node, &controls, &mut vertex_offset);
        }
    }
}
