---
title: memory-allocation
pageTitle: Memory allocation
layout: "layouts/guide"
eleventyNavigation:
  parent: Advanced
  key: Memory allocation
  order: 0
---

## Custom allocators

**All** functions that might allocate memory in ufbx take in a `ufbx_allocator` structure.
You can provide one through using the `ufbx_$TYPE_opts` structs such as `ufbx_load_opts` or `ufbx_evaluate_opts`.

If you return `NULL` from allocation functions ufbx will gracefully fail with `UFBX_ERROR_OUT_OF_MEMORY`.

```c
// Dummy "allocator" that just keeps track of current size.
typedef struct {
    size_t total_size;
} my_allocator;

// Allocate `size` bytes of memory.
void *my_alloc(void *user, size_t size) {
    my_allocator *ator = (my_allocator*)user;

    ator->total_size += size;
    return malloc(size);
}

// Free `size` bytes of memory starting from pointer `ptr`.
void my_free(void* user, void* ptr, size_t size) {
    my_allocator *ator = (my_allocator*)user;

    ator->total_size -= size;
    free(ptr);
}

// This function is called when ufbx does not need the allocator anymore.
// - All memory is guaranteed to be freed before this function is called.
// - This function gets called even in all failure cases.
void my_free_allocator(void *user) {
    my_allocator *ator = (my_allocator*)user;

    // Free the allocator itself
    assert(ator->total_size == 0);
    free(ator);
}

// Set the correct function pointers and allocate a `my_allocator` structure.
void setup_allocator(ufbx_allocator *allocator) {
    allocator->alloc_fn = &my_alloc;
    allocator->free_fn = &my_free;
    allocator->free_allocator_fn = &my_free_allocator;
    allocator->user = calloc(1, sizeof(my_allocator));
}

ufbx_scene *load_file(const char *filename, ufbx_error *error)
{
    ufbx_load_opts opts = { ... };
    setup_allocator(&opts.temp_allocator);
    setup_allocator(&opts.result_allocator);
    return ufbx_load_file(filename, &opts, &error);
}
```

You can omit either `ufbx_allocator.alloc_fn` or `ufbx_allocator.free_fn` and they will
be routed to `ufbx_allocator.realloc_fn` with a zero `new_size` or `old_size` respectively.

```c
void *minimal_realloc(void *user, void *ptr, size_t old_size, size_t new_size)
{
    if (new_size == 0) {
        my_free(ptr);
        return NULL;
    } else if (old_size == 0) {
        return my_alloc(ptr);
    } else {
        return my_realloc(ptr, old_size, new_size);
    }
}

ufbx_scene *load_file(const char *filename, ufbx_error *error)
{
    ufbx_load_opts opts = { ... };
    opts.temp_allocator.allocator.realloc_fn = &minimal_realloc;
    opts.result_allocator.allocator.realloc_fn = &minimal_realloc;
    return ufbx_load_file(filename, &opts, &error);
}
```

## Memory limits

In addition to providing allocation callbacks `ufbx_allocator` contains memory limits.
Use `ufbx_allocator_opts.memory_limit` to set maximum amount of bytes to allocate.
If the file exceeds the memory limit loading will fail with `UFBX_ERROR_MEMORY_LIMIT`.

```c
ufbx_load_opts opts = { ... };

// Limit the working memory to 128MB and the final scene to 64MB.
opts.temp_allocator.memory_limit = 128*1024*1024;
opts.result_allocator.memory_limit = 64*1024*1024;
```

## Debugging

If you are using [AddressSanitizer (ASAN)][asan] you might want to set `ufbx_allocator_opts.huge_threshold` to 1.
This causes ufbx to allocate small allocations separately, which lets ASAN catch more potential memory overflows.

```c
ufbx_load_opts opts = { ... };

// Use individual allocations if AddressSanitizer is enabled.
#if defined(USE_ASAN)
    opts.temp_allocator.huge_threshold = 1;
    opts.result_allocator.huge_threshold = 1;
#endif
```


[asan]: https://github.com/google/sanitizers/wiki/AddressSanitizer
