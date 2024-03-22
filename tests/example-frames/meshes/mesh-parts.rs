// $dep material_parts.fbx
// $ cargo build
// $ cargo run --quiet
use ufbx;

fn create_vertex_buffer<T>(vertices: &[T]) {
    println!(".. {} vertices", vertices.len());
}

fn create_index_buffer(indices: &[u32]) {
    println!(".. {} indices", indices.len());
}

// -- EXAMPLE_SOURCE --

fn main() {
    let scene = ufbx::load_file("material_parts.fbx", Default::default()).expect("failed to load scene");

    for mesh in &scene.meshes {
        println!("mesh '{}'", mesh.element.name);

        for part in &mesh.material_parts {
            let material = mesh.materials.get(part.index as usize);
            let material_name = material
                .map(|m| m.element.name.as_ref())
                .unwrap_or("");

            println!(". [{}] material: {}", part.index, material_name);
            convert_mesh_part(mesh, part);
        }
    }
}
