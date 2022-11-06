use ufbx;
use std::vec::{Vec};

fn output_triangle(a: ufbx::Vec3, b: ufbx::Vec3, c: ufbx::Vec3) {
    println!("({:.2}, {:.2}, {:.2}) ({:.2}, {:.2}, {:.2}) ({:.2}, {:.2}, {:.2})",
        a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
}

// -- EXAMPLE_SOURCE --

fn main() {
    let scene = ufbx::load_file("my_scene.fbx", ufbx::LoadOpts::default()).unwrap();
    let node = scene.find_node("Cube").unwrap();
    list_triangles(node.mesh.as_ref().unwrap());
}
