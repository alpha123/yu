/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "arena.h"

struct arena_handle *arena_new(yu_memctx_t *mctx) {
    YU_ERR_DEFVAR
    struct arena *a = NULL;
    struct arena_handle *ah = NULL;
    size_t align = 1 << yu_ceil_log2(sizeof(struct arena));
    YU_CHECK(yu_alloc(mctx, (void **)&a, 1, sizeof(struct arena), align));
    YU_CHECK(yu_alloc(mctx, (void **)&ah, 1, sizeof(struct arena_handle), 0));

    a->next = a->objs;

    ah->self = a;
    ah->mem_ctx = mctx;
    
    return ah;

    yu_err_handler:
    if (a) yu_free(mctx, a);
    yu_global_fatal_handler(yu_local_err);
    return NULL;
}

void arena_free(struct arena_handle *a) {
    yu_memctx_t *mctx = a->mem_ctx;
    yu_free(mctx, a->self);
    yu_free(mctx, a);
}

struct boxed_value *arena_alloc_val(struct arena_handle *a) {
    struct arena *ar = a->self;
    if (YU_UNLIKELY(ar->next == ar->objs + GC_ARENA_NUM_OBJECTS)) {
        struct arena_handle *next = arena_new(a->mem_ctx);
        a->self = next->self;
        next->self = ar;
        next->next_gen = a->next_gen;
        next->next = a->next;
        a->next = next;
        ar = a->self;
    }
    return ar->next++;
}

u32 arena_allocated_count(struct arena_handle *a) {
    u32 cnt = 0;
    while (a) {
        cnt += ((uintptr_t)a->self->next - (uintptr_t)a->self->objs) / sizeof(struct boxed_value);
        a = a->next;
    }
    return cnt;
}

u32 arena_gray_count(struct arena_handle *a) {
    u32 cnt = 0;
    while (a) {
        for (u32 i = 0; i < elemcount(a->self->graymap); i++)
            cnt += __builtin_popcountll(a->self->graymap[i]);
        a = a->next;
    }
    return cnt;
}

struct boxed_value *arena_pop_gray(struct arena_handle *a) {
    if (a == NULL)
        return NULL;

    struct arena *ar = a->self;
    u32 idx = 0;
    for (u32 i = 0; i < elemcount(ar->graymap); i++) {
        if (ar->graymap[i] != 0) {
            u32 x = __builtin_ctzll(ar->graymap[i]);
            ar->graymap[i] &= ~(1 << x);
            return ar->objs + idx + x;
        }
        idx += 64;
    }
    // If there are no gray objects in this arena,
    // recursively look in the next one.
    return arena_pop_gray(a->next);
}

void arena_push_gray(struct arena_handle *a, struct boxed_value *v) {
    struct arena *ar = a->self;
    u32 idx_base = 0;
    while ((uintptr_t)v < (uintptr_t)ar->objs || (uintptr_t)v >= (uintptr_t)ar->objs + GC_ARENA_NUM_OBJECTS) {
        if ((a = a->next) == NULL)
            return;
        ar = a->self;
        ++idx_base;
    }
    u32 idx = ((uintptr_t)v - (uintptr_t)ar->objs) / sizeof(struct boxed_value);
    ar->graymap[idx_base] |= 1 << idx;
}

void arena_promote(struct arena_handle *a) {
}

void arena_empty(struct arena_handle *a) {
    struct arena *ar = a->self;
    ar->next = ar->objs;
    memset(ar->graymap, 0, sizeof(ar->graymap));
    memset(ar->markmap, 0, sizeof(ar->markmap));
}
