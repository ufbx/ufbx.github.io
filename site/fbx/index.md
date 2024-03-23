---
title: FBX
pageTitle: FBX
layout: "layouts/guide"
eleventyNavigation:
  key: FBX
  order: 100
  url: false
---

Pages under this section detail the inner workings of the FBX format.
These should be necessary only for advanced use cases, as *ufbx* handles most quirks of the FBX format internally.

One should be very cautious when interfacing with the FBX format, as it has many semi-rarely used features,
meaning that any new file may require a feature that you must account for,
if aiming for correctness across any files in the wild.
Less care is required if you are in control both of the importer and the files,
as then you only need to support the necessary ysubset of features.
