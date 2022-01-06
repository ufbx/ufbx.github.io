#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct arena_t {
    union {
        size_t align_size;
        void *align_ptr;
        char size[512];
    } opaque;
} arena_t;

typedef void arena_defer_fn(void *user);

arena_t *arena_create(arena_t *parent);
bool arena_init(arena_t *arena, arena_t *parent);
void arena_free(arena_t *arena);

void *arena_defer_size(arena_t *arena, arena_defer_fn *fn, size_t size, const void *data);
#define arena_defer(arena, fn, type, data) (type*)arena_defer_size((arena), (fn), sizeof(type), (data))
void arena_cancel_retain(arena_t *arena, void *ptr, bool run_defer);
void arena_cancel(arena_t *arena, void *ptr, bool run_defer);

size_t arena_ext_defer(arena_t *arena, arena_defer_fn *fn, const void *data);
void arena_ext_redefer(arena_t *arena, size_t slot, arena_defer_fn *fn, const void *data);
void arena_ext_cancel(arena_t *arena, size_t slot, bool run_defer);

void *aalloc_uninit_size(arena_t *arena, size_t size, size_t count);
void *aalloc_size(arena_t *arena, size_t size, size_t count);
void *aalloc_copy_size(arena_t *arena, size_t size, size_t count, const void *data);
char *aalloc_copy_str(arena_t *arena, const char *str);
#define aalloc_uninit(arena, type, count) ((type*)aalloc_uninit_size((arena), sizeof(type), (count)))
#define aalloc_copy(arena, type, count, data) ((type*)aalloc_copy_size((arena), sizeof(type), (count), (data)))
#define aalloc(arena, type, count) ((type*)aalloc_size((arena), sizeof(type), (count)))

void afree(arena_t *arena, void *ptr);

size_t aalloc_capacity_bytes(void *ptr);
#define alloc_capacity(type, ptr) (aalloc_byte_capacity(ptr) / sizeof(type))

void *arealloc_size(arena_t *arena, size_t size, size_t count, void *ptr);
#define arealloc(arena, type, count, ptr) ((type*)arealloc_size((arena), sizeof(type), (count), (ptr)))

#define alist_t(type) struct { type *data; size_t count; }

void *alist_push_size(arena_t *arena, size_t size, void *p_list, void *p_item);
void *alist_pop_size(size_t size, void *p_list);
void *alist_push_n_size(arena_t *arena, size_t size, void *p_list, size_t n, void *p_item);
void *alist_pop_n_size(size_t size, void *p_list, size_t n);
bool alist_remove_size(size_t size, void *p_list, size_t index);

#define alist_push(arena, type, p_list) ((type*)alist_push_size((arena), sizeof(type), (p_list), NULL))
#define alist_push_copy(arena, type, p_list, p_item) ((type*)alist_push_size((arena), sizeof(type), (p_list), (p_item)))
#define alist_pop(type, p_list) ((type*)alist_pop_size(sizeof(type), (p_list)))
#define alist_push_n(arena, type, p_list, n) ((type*)alist_push_n_size((arena), sizeof(type), (p_list), (n), NULL))
#define alist_push_n_copy(arena, type, p_list, n, p_item) ((type*)alist_push_n_size((arena), sizeof(type), (p_list), (n), (p_item)))
#define alist_pop_n(type, p_list, n) ((type*)alist_pop_n_size(sizeof(type), (p_list), (n)))
#define alist_remove(type, p_list) (alist_remove_size(sizeof(type), (p_list)))
