---
title: nurbs
pageTitle: NURBS
layout: "layouts/guide"
eleventyNavigation:
  parent: Advanced
  key: NURBS
  order: 2
---

FBX supports both NURBS curves (`ufbx_nurbs_curve`) and surfaces (`ufbx_nurbs_surface`).
Bézier curves are represented as NURBS curves with uniform spacing and W=1.

## Tessellation

For previewing ufbx supports tessellating curves and surfaces using `ufbx_tessellate_nurbs_curve()`
and `ufbx_tessellate_nurbs_surface()` respectively.
 