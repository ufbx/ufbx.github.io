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

arena_t *arena_make(arena_t *parent);
bool arena_init(arena_t *arena, arena_t *parent);
void arena_free(arena_t *arena);

void *arena_defer_size(arena_t *arena, arena_defer_fn *fn, size_t size, const void *data);
#define arena_defer(arena, fn, type, data) (type*)arena_defer_size((arena), (fn), sizeof(type), (data))
void arena_cancel_retain(arena_t *arena, void *ptr);
void arena_cancel(arena_t *arena, void *ptr);

size_t arena_ext_defer(arena_t *arena, arena_defer_fn *fn, const void *data);
void arena_ext_redefer(arena_t *arena, size_t slot, arena_defer_fn *fn, const void *data);
void arena_ext_cancel(arena_t *arena, size_t slot);

void *aalloc_uninit_size(arena_t *arena, size_t size, size_t count);
void *aalloc_size(arena_t *arena, size_t size, size_t count);
void *aalloc_copy_size(arena_t *arena, size_t size, size_t count, const void *data);
char *aalloc_copy_str(arena_t *arena, const char *str);
#define aalloc_uninit(arena, type, count) ((type*)aalloc_uninit_size((arena), sizeof(type), (count)))
#define aalloc_copy(arena, type, count, data) ((type*)aalloc_copy_size((arena), sizeof(type), (count), (data)))
#define aalloc(arena, type, count) ((type*)aalloc_size((arena), sizeof(type), (count)))

static void *arealloc_size(arena_t *arena, size_t size, size_t count, void *ptr) {
	void *arenaimp_arealloc_size(arena_t *arena, size_t size, size_t count, void *ptr);
    if (ptr && ((size_t*)ptr)[-1] >= size * count) {
        return ptr;
    } else {
		return arenaimp_arealloc_size(arena, size, count, ptr);
    }
}
#define arealloc(arena, type, count, ptr) ((type*)arealloc_size((arena), sizeof(type), (count), (ptr)))

void afree(arena_t *arena, void *ptr);

