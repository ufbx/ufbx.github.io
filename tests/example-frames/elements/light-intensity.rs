// $dep light.fbx
// $ cargo build
// $ cargo run --quiet
use ufbx;

// -- EXAMPLE_SOURCE --

fn main() {
    let scene = ufbx::load_file("light.fbx", Default::default()).expect("failed to load scene");
    let node = scene.find_node("Light").expect("could not find node");
    let light = node.light.as_ref().expect("node did not contain a light");
    print_intensity(light);
}
