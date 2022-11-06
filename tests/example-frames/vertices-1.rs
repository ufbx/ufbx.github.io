use ufbx;

fn begin_polygon() {
    println!("begin polygon");
}

fn polygon_corner(position: ufbx::Vec3, normal: ufbx::Vec3) {
    println!("  position ({:.2}, {:.2}, {:.2}) normal ({:.2}, {:.2}, {:.2})",
        position.x, position.y, position.z,
        normal.x, normal.y, normal.z);
}

fn end_polygon() {
    println!("end polygon");
}

// -- EXAMPLE_SOURCE --

fn main() {
    let scene = ufbx::load_file("my_scene.fbx", ufbx::LoadOpts::default()).unwrap();
    let node = scene.find_node("Cube").unwrap();
    list_vertices(node.mesh.as_ref().unwrap());
}
