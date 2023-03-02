---
title: Subdivision
pageTitle: Subdivision
layout: "layouts/guide"
eleventyNavigation:
  parent: Advanced
  key: Subdivision
  order: 1
---

FBX supports [Catmull-Clark sudbivision](https://en.wikipedia.org/wiki/Catmull%E2%80%93Clark_subdivision_surface)
for meshes. Subdivision may be enabled if `ufbx_mesh.subdivision_preview_levels` or
`@(ufbx_mesh.)subdivision_render_levels` if specified.

## Creases

Subdivided meshes may contain semi-sharp features using `ufbx_mesh.edge_crease` and `@(ufbx_mesh).vertex_crease`.
These values generally range from 0 (smooth) to 1 (infinitely sharp). The common interpretation
is that you multiply the crease value by 10 to get the number of subdivision levels where you
use the crease rule[^1] for subdivision, if non-integer you should blend between the standard
subdivision and crease rules with the fractional weight.

So for example an edge crease value of `2.3` means that for the first two subdivision levels we
treat the edge as fully sharp (as if it were a boundary), for the third level we interpolate
between that and the standard subdivision rule at 30%, and for levels four and beyond we use
the standard smooth subdivision rule.

## Evaluation

ufbx supports evaluating subdivided meshes at a desired subdivision level using
`ufbx_subdivide_mesh()`. The function supports optionally evaluating skinning and
source vertex information.

```c
ufbx_mesh *subdivided = ufbx_subdivide_mesh(mesh, 2, NULL, NULL);
draw_mesh(subdivided);
ufbx_free_mesh(subdivided);
```

[^1]: See [Subdivision Surfaces in Character Animation](https://graphics.pixar.com/library/Geri/paper.pdf) for more information.
