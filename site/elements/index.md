---
title: Elements
pageTitle: Elements
layout: "layouts/guide"
eleventyNavigation:
  key: Elements
  order: 50
  url: false
---

Nearly everything in *ufbx* consist of elements[^1] represented by a "base-class" like `struct ufbx_element`.
Elements contain common properties such as `@(ufbx_element.)name` and `@(ufbx_element.)props` for FBX key/value properties.

"Derived" types such as `ufbx_node`, `ufbx_mesh`, or `ufbx_material` embed a `ufbx_element` header as an anynomous `union`,
allowing you to cast it to a `ufbx_element` or access the common `ufbx_element` properties directly.

```c
void list_nodes(ufbx_scene *scene)
{
    for (size_t i = 0; i < scene->nodes.count; i++) {
        ufbx_node *node = scene->nodes.data[i];

        printf("Node %u: '%s'\n", node->element.name.data);

        // Equivalent shorthand:
        printf("Node '%s'\n", node->name.data);
    }
}
```

## Element IDs

`ufbx_scene` is structured using nested pointers for convenience, but it is often
useful to have identifiers:

* `ufbx_element.element_id` is a contiguous index unique for each element
* `ufbx_element.typed_id` is a contiguous index of the element within its type

You can retrieve the elements back by indexing the arrays in `ufbx_scene` using the indices:

```c
ufbx_scene *scene;
ufbx_node *node;
ufbx_mesh *mesh;
// ufbx-doc-snip

assert(node == scene->nodes.data[node->typed_id]);
assert(node == (ufbx_node*)scene->elements.data[node->element_id]);

assert(mesh == scene->meshes.data[mesh->typed_id]);
assert(mesh == (ufbx_mesh*)scene->elements.data[mesh->element_id]);
```

```cpp
ufbx_scene *scene;
ufbx_node *node;
ufbx_mesh *mesh;
// ufbx-doc-snip

assert(node == scene->nodes[node->typed_id]);
assert(node == (ufbx_node*)scene->elements[node->element_id]);

assert(mesh == scene->meshes[mesh->typed_id]);
assert(mesh == (ufbx_mesh*)scene->elements[mesh->element_id]);
```

```rust
assert!(ptr::eq(&node, scene.nodes[node.element.typed_id]));
assert!(ptr::eq(&node.element, scene.elements[node.element.element_id]));

assert!(ptr::eq(&mesh, scene.nodes[mesh.element.typed_id]));
assert!(ptr::eq(&mesh.element, scene.elements[mesh.element.element_id]));
```

These indices are stable between loading the same file multiple times but not necessarily stable even if the file is only re-exported.

## Properties

Modern FBX files contain generic key/value properties for every element that *ufbx* exposes through `ufbx_element.props`.

In most cases *ufbx* will internally interpret these to fields such as `ufbx_node.local_transform` or `ufbx_light.intensity`
but there are some cases where you want to use `ufbx_props` directly:

* Manually interpreting animation curves via `ufbx_anim_curve`
* Custom user-defined properties
* Non-standard materials

```c
// ufbx-doc-example: elements/light-intensity
void print_intensity(ufbx_light *light)
{
    // `light->props` is shorthand for `light->element.props`
    ufbx_prop *prop = ufbx_find_prop(&light->props, "Intensity");
    assert(prop);

    printf("ufbx_light.intensity: %.2f\n", light->intensity);
    printf("ufbx_props.Intensity: %.2f\n", prop->value_real);
}
```

```cpp
// ufbx-doc-example: elements/light-intensity
void print_intensity(ufbx_light *light)
{
    // `light->props` is shorthand for `light->element.props`
    ufbx_prop *prop = ufbx_find_prop(&light->props, "Intensity");
    assert(prop);

    printf("ufbx_light.intensity: %.2f\n", light->intensity);
    printf("ufbx_props.Intensity: %.2f\n", prop->value_real);
}
```

```rust
// ufbx-doc-example: elements/light-intensity
fn print_intensity(light: &ufbx::Light) {
    let prop = light.element.props.find_prop("Intensity")
        .expect("expected to find 'Intensity'");

    println!("ufbx_light.intensity: {:.2}", light.intensity);
    println!("ufbx_props.Intensity: {:.2}", prop.value_vec3.x);
}
```

Warning: *ufbx* does a lot of work to present the values of the FBX file in an
intuitive manner but if you use `ufbx_props` directly you need to be aware of
the quirks. For example with the `"Intensity"` above: FBX files tend to store
light intensity multiplied by 100 compared to what was input in a given 3D
modeling program.

*ufbx* tries its best to report values as they were before exporting so for
a scene made in Blender with a light having intensity `2.0` the above example
would output:

```
ufbx_light.intensity: 2.00
ufbx_props.Intensity: 200.00
```

[^1]: FBX files format refers to these as Objects, but *ufbx* calls them elements as "object" is a very overloaded word in the domain of 3D models.
