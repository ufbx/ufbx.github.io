---
title: Material
pageTitle: Material
layout: "layouts/guide"
eleventyNavigation:
  parent: Basics
  key: Material
  order: 5
---

Materials are represented using `ufbx_material` which are collections of "maps" (`ufbx_material_map`).
Each map represents a material attribute such as diffuse color, roughness, etc. and may have a constant
value or an attached texture (`ufbx_texture`). FBX has a built-in legacy Phong-style material model `ufbx_material.fbx` and
a heterogeneous mess of proprietary PBR materials that ufbx unifies as `ufbx_material.pbr`.

### Legacy model

- `@(ufbx_material_fbx_maps.)diffuse_color`
- `@(ufbx_material_fbx_maps.)specular_color`
- `@(ufbx_material_fbx_maps.)specular_exponent`

### PBR model

- `@(ufbx_material_pbr_maps.)base_color`
- `@(ufbx_material_pbr_maps.)roughness`
- `@(ufbx_material_pbr_maps.)metalness`

## Textures

Textures often contain absolute and relative filenames and may contain embedded content via `ufbx_texture.content`.
`ufbx_texture.filename` is a path to where the texture is expected to be found according to the path of the FBX file
and relative filename.
