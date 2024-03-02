// $dep cube.fbx
// $ cargo build
// $ cargo run --quiet
use ufbx;
use std::cell::RefCell;

#[derive(Default)]
struct PolygonCtx {
    pub mesh_to_world: ufbx::Matrix,
    pub mesh_normal_to_world: ufbx::Matrix,
    pub index_base: u32,
    pub face_size: u32,
}

thread_local! {
    static CTX: RefCell<PolygonCtx> = RefCell::new(PolygonCtx::default());
}

fn begin_polygon() { }

fn polygon_corner(position: ufbx::Vec3, normal: ufbx::Vec3, uv: ufbx::Vec2) {
    CTX.with(|c| {
        let mut c = c.borrow_mut();
        let position = ufbx::transform_position(&c.mesh_to_world, position);
        let normal = ufbx::transform_direction(&c.mesh_normal_to_world, normal);

        println!("v {:.2} {:.2} {:.2}", position.x, position.y, position.z);
        println!("vn {:.2} {:.2} {:.2}", normal.x, normal.y, normal.z);
        println!("vt {:.2} {:.2}", uv.x, uv.y);
        c.face_size += 1;
    });
}

fn end_polygon() {
    CTX.with(|c| {
        let mut c = c.borrow_mut();
        print!("f");
        for i in 0..c.face_size {
            let ix = 1 + c.index_base + i;
            print!(" {0}/{0}/{0}", ix);
        }
        println!("\n");
        c.index_base += c.face_size;
        c.face_size = 0;
    });
}

// -- EXAMPLE_SOURCE --

fn main() {
    let scene = ufbx::load_file("cube.fbx", Default::default()).expect("failed to load scene");

    for mesh in &scene.meshes {
        for node in &mesh.element.instances {
            println!("# mesh '{}', node '{}'\n", mesh.element.name, node.element.name);

            // Set the current transformation matrices
            CTX.with(|c| {
                let mut c = c.borrow_mut();
                c.mesh_to_world = node.geometry_to_world;
                c.mesh_normal_to_world = ufbx::matrix_for_normals(&c.mesh_to_world);
            });

            draw_polygons(mesh);
        }
    }
}
