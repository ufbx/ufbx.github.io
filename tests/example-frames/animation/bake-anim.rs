// $dep cube_animations.fbx
// $ cargo build
// $ cargo run --quiet
use ufbx;

// -- EXAMPLE_SOURCE --

fn main() {
    let scene = ufbx::load_file("cube_animations.fbx", Default::default()).expect("failed to load scene");
    bake_animations(&scene);
}

