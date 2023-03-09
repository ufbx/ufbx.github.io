// $dep skinned_human.fbx
// $crate glam
// $ cargo build
// $ cargo run --quiet -- skinned_human.fbx Female > result.obj
use ufbx;
use glam::*;

trait AsGlam<T> {
    fn as_glam(self) -> T;
}

impl AsGlam<DVec3> for ufbx::Vec3 {
    fn as_glam(self) -> DVec3 {
        DVec3::new(self.x, self.y, self.z)
    }
}

impl AsGlam<DMat4> for ufbx::Matrix {
    fn as_glam(self) -> DMat4 {
        DMat4::from_cols(
            DVec4::new(self.m00, self.m10, self.m20, 0.0),
            DVec4::new(self.m01, self.m11, self.m21, 0.0),
            DVec4::new(self.m02, self.m12, self.m22, 0.0),
            DVec4::new(self.m03, self.m13, self.m23, 1.0))
    }
}

// -- EXAMPLE_SOURCE --

fn main() {
    let path = std::env::args().nth(1).expect("expected .fbx path");
    let node_name = std::env::args().nth(2).expect("expected node name");

    let scene = ufbx::load_file(&path, Default::default()).expect("failed to load scene");
    let node = scene.find_node(&node_name).expect("could not find node");
    let mesh = node.mesh.as_ref().expect("node did not contain a mesh");
    let skin = mesh.skin_deformers.get(0).expect("no skin deformer in mesh");
    for i in 0..mesh.num_vertices {
        let vertex = transform_vertex(&mesh, &skin, i);
        println!("v {:.5} {:.5} {:.5}", vertex.x, vertex.y, vertex.z);
    }

    // Output faces
    for face in &mesh.faces {
        print!("f");
        for j in 0..face.num_indices {
            print!(" {}", mesh.vertex_indices[(face.index_begin + j) as usize] + 1);
        }
        println!("");
    }
}
