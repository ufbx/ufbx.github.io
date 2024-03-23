// $dep units_*.fbx
// $ cargo build
// $ cargo run --quiet

pub fn check_file(path: &str, prefer_blender: bool) {
    // -- EXAMPLE_SOURCE --

    // Remember `space_conversion` as `load_file()` consumes `opts`.
    let space_conversion = opts.space_conversion;

    let scene = ufbx::load_file(path, opts).expect("expected to load scene");

    let node = scene.find_node("Cube").expect("expected to find 'Cube'");
    let mesh = node.mesh.as_ref().expect("expected 'Cube' to have a mesh");

    let scale = node.local_transform.scale;
    let node_size = scale.x.max(scale.y).max(scale.z);

    let mesh_size = mesh.vertices
        .iter()
        .map(|v| v.x.max(v.y).max(v.z))
        .reduce(f64::max)
        .unwrap_or(0.0);

    let mode = match space_conversion {
        ufbx::SpaceConversion::AdjustTransforms => "ADJUST_TRANSFORMS",
        ufbx::SpaceConversion::ModifyGeometry => "MODIFY_GEOMETRY",
        _ => panic!("unexpected space conversion: {:?}", space_conversion),
    };

    let comment = if (node_size - 1.0).abs() < 0.001 && (mesh_size - 1.0).abs() < 0.001 {
        "clean"
    } else {
        ".."
    };

    println!("  {:<20} node_size {:<8.2} mesh_size {:<8.2} {}",
        mode, node_size, mesh_size, comment);
}

fn main() {
    let filenames = [
        "units_blender_meters.fbx",
        "units_blender_default.fbx",
        "units_maya_default.fbx",
    ];

    for path in &filenames {
        println!("\n{}:", path);
        check_file(path, false);
        check_file(path, true);
    }
}
