---
title: Build Options
pageTitle: Build Options
layout: "layouts/guide"
eleventyNavigation:
  parent: Advanced
  key: Build Options
  order: 1
---

Building *ufbx* is primarily customized via preprocessor macros.

You can either define the macros globally using compiler flags, before including `"ufbx.h"` or `"ufbx.c"`,
or by defining `UFBX_CONFIG_HEADER="my-config.h"` or `UFBX_CONFIG_SOURCE="my-source-config.h"`,
which makes *ufbx* include the specified files in `ufbx.h` and `ufbx.c` respectively.

## Header

Nearly all configuration macros need to be only visible to `"ufbx.c"`.
However, some defines should be visible to `"ufbx.h"`, as they change the types or are used in inline functions:

```c
// Override the default assert.
// This can be defined either for the header or the source, but if you define it
// visible in the header, inline operations will also use it.
#define ufbx_assert(cond) my_assert(cond)

// Defines ufbx_real as float, instead of double.
// This define must be visible in the header.
#define UFBX_REAL_IS_FLOAT
```

## Optional features

To reduce the size of *ufbx*, you can selectively disable some optional features:

```c
// Mesh subdivision
//   ufbx_subdivide_mesh()
#define UFBX_NO_SUBDIVISION

// NURBS tessellation
//   ufbx_tessellate_nurbs_curve()
//   ufbx_tessellate_nurbs_surface()
#define UFBX_NO_TESSELLATION

// Geometry caches
//   ufbx_load_geometry_cache()
#define UFBX_NO_GEOMETRY_CACHE

// Scene evaluation
//   ufbx_evaluate_scene()
#define UFBX_NO_SCENE_EVALUATION

// Skinning evaluation
#define UFBX_NO_SKINNING_EVALUATION

// Animation baking
//   ufbx_bake_anim()
#define UFBX_NO_ANIMATION_BAKING

// Face triangulation
//   ufbx_triangulate_face()
#define UFBX_NO_TRIANGULATION

// Index generation
//   ufbx_generate_indices()
#define UFBX_NO_INDEX_GENERATION

// .obj file format support
#define UFBX_NO_FORMAT_OBJ
```

## Memory allocation

*ufbx* uses the C standard `malloc()` interface for memory allocation.
You can override the default memory allocator used by *ufbx* in three ways:
either by preprocessor define, external functions, or disabling it completely.

Overriding the allocator can be done with the following defines:

```c
// Hooks to custom allocator API
#define ufbx_malloc(size) my_alloc(size)
#define ufbx_realloc(ptr, old_size, new_size) my_realloc(ptr, old_size, new_size)
#define ufbx_free(ptr, size) my_free(ptr, size)

#include "ufbx.c"
```

Alternatively you can define `UFBX_EXTERNAL_MALLOC` and define[^1] the following functions:

```c
// Hooks to custom allocator API
void *ufbx_malloc(size_t size);
void *ufbx_realloc(void *ptr, size_t old_size, size_t new_size);
void ufbx_free(void *ptr, size_t size);
```

Finally, if you don't need to use the default memory allocator,
you can define `UFBX_NO_MALLOC`, which will make any allocating API fail if you do not pass a user-provided `ufbx_allocator`.

## File I/O

*ufbx* uses the C standard `FILE` API for loading files when you call `ufbx_load_file()`.
You can override this either via external functions or disabling the default file API.

If `UFBX_EXTERNAL_STDIO` is defined, *ufbx* will use externally defined standard file functions:

```c
// Open a file at `path` and return a handle.
// Return `NULL` if not found.
// `path` is null terminated and `path_len` bytes long (excluding the '\0')
void *ufbx_stdio_open(const char *path, size_t path_len);

// Read `size` bytes from `file` to `data`.
// Return the number of bytes read.
size_t ufbx_stdio_read(void *file, void *data, size_t size);

// Skip `size` bytes forward in `file`.
// Return `true` if there was no error.
// HINT: If impossible, you can internally call `ufbx_stdio_read()` into a dummy buffer.
bool ufbx_stdio_skip(void *file, size_t size);

// Return the estimated size of `file` in bytes.
// You can return 0 if the size cannot be estimated or SIZE_MAX if an error occurred.
uint64_t ufbx_stdio_size(void *file);

// Close a previously opened `file`.
void ufbx_stdio_close(void *file);
```

Alternatively, you can define `UFBX_NO_STDIO`, which will make all functionality requiring standard I/O fail,
such as `ufbx_load_file()` without a custom `ufbx_load_opts.open_file_cb`.

## Math

*ufbx* needs some functions from the standard `<math.h>` library.
The implementation of these may vary depending on the platform,
so if you need bit-exact results on all environments you can override the math library.

If you define `UFBX_EXTERNAL_MATH`, *ufbx* will use the following externally defined functions instead:

```c
double ufbx_sqrt(double x);
double ufbx_sin(double x);
double ufbx_cos(double x);
double ufbx_tan(double x);
double ufbx_asin(double x);
double ufbx_acos(double x);
double ufbx_atan(double x);
double ufbx_atan2(double y, double x);
double ufbx_pow(double x, double y);
double ufbx_fmin(double a, double b);
double ufbx_fmax(double a, double b);
double ufbx_fabs(double x);
double ufbx_copysign(double x, double y);
double ufbx_nextafter(double x, double y);
double ufbx_rint(double x);
double ufbx_ceil(double x);
int ufbx_isnan(double x);
```

You can either define these yourself or use [`ufbx_math.c`](https://github.com/ufbx/ufbx/blob/master/extra/ufbx_math.c).

## C standard library

You can fully disable most uses of the standard library by defining `UFBX_NO_LIBC`.
By default, this implies `UFBX_EXTERNAL_MALLOC`, `UFBX_EXTERNAL_STDIO` and `UFBX_EXTERNAL_MATH`.
You can either define these or use `UFBX_NO_MALLOC` and `UFBX_NO_STDIO` to disable the functionality.

Additionally, you need to define the following `<string.h>` functions (or use [`ufbx_libc.c`](https://github.com/ufbx/ufbx/blob/master/extra/ufbx_libc.c)):

```c
size_t ufbx_strlen(const char *str);
void *ufbx_memcpy(void *dst, const void *src, size_t count);
void *ufbx_memmove(void *dst, const void *src, size_t count);
void *ufbx_memset(void *dst, int ch, size_t count);
const void *ufbx_memchr(const void *ptr, int value, size_t count);
int ufbx_memcmp(const void *a, const void *b, size_t count);
int ufbx_strcmp(const char *a, const char *b);
int ufbx_strncmp(const char *a, const char *b, size_t count);
```

Note that *ufbx* still needs the following standard headers to function:

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
```

These are usually available even in freestanding scenarios.
If not, you can define `UFBX_NO_LIBC_TYPES`,
but in that case you _must_ define the contents of these files manually both to `"ufbx.h"` and `"ufbx.c"`.

[^1]: These must be defined using the default calling convention and using `extern "C"` linkage in C++.
