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
    a->meta = ah;
    ah->self = a;
    ah->mem_ctx = mctx;
    
    return ah;

    yu_err_handler:
    if (a) yu_free(mctx, a);
    yu_global_fatal_handler(yu_local_err);
    return NULL;
}

void arena_free(struct arena_handle *a) {
    if (a->next)
        arena_free(a->next);
    yu_memctx_t *mctx = a->mem_ctx;
    yu_free(mctx, a->self);
    yu_free(mctx, a);
}

struct boxed_value *arena_alloc_val(struct arena_handle *a) {
    struct arena *ar = a->self;
    if (YU_UNLIKELY(ar->next == ar->objs + GC_ARENA_NUM_OBJECTS)) {
        struct arena_handle *next = arena_new(a->mem_ctx);
        a->self = next->self;
        next->self->meta = a;
        next->self = ar;
        ar->meta = next;
        next->next_gen = a->next_gen;
        next->next = a->next;
        a->next = next;
        ar = a->self;
    }
    struct boxed_value *val = ar->next++;
    return val;
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
            u64 x = __builtin_ctzll(ar->graymap[i]);
            ar->graymap[i] &= ~(UINT64_C(1) << x);
            return ar->objs + idx + x;
        }
        idx += 64;
    }
    // If there are no gray objects in this arena,
    // recursively look in the next one.
    return arena_pop_gray(a->next);
}

static
u64 get_value_arena_idx(struct arena_handle *a, struct boxed_value *v, struct arena **ar_out) {
    struct arena *ar = a->self;
    while ((uintptr_t)v < (uintptr_t)ar->objs ||
            (uintptr_t)v >= (uintptr_t)ar->objs + GC_ARENA_NUM_OBJECTS * sizeof(struct boxed_value)) {
        if ((a = a->next) == NULL) {
            *ar_out = NULL;
            return 0;
        }
        ar = a->self;
    }
    *ar_out = ar;
    return ((uintptr_t)v - (uintptr_t)ar->objs) / sizeof(struct boxed_value);
}

// TODO Standardize division/modulo syntax?
// I use i / 64 instead of i >> 6 but i & 63 instead of i % 64.
// I don't really know why. I find i & 63 no less clear than
// modulo, but perhaps too much C has poisoned my brain. ;-)
// The compiler will optimize both of them to bitwise operators
// anyway since they're always constants, so it just comes down
// to clarity.

void arena_push_gray(struct arena_handle *a, struct boxed_value *v) {
    struct arena *ar;
    u64 idx = get_value_arena_idx(a, v, &ar);
    assert(ar != NULL);
    ar->graymap[idx / 64] |= UINT64_C(1) << (idx & 63);
}

void arena_mark(struct arena_handle *a, struct boxed_value *v) {
    struct arena *ar;
    u64 idx = get_value_arena_idx(a, v, &ar);
    assert(ar != NULL);
    ar->markmap[idx / 64] |= UINT64_C(1) << (idx & 63);
}

void arena_unmark(struct arena_handle *a, struct boxed_value *v) {
    struct arena *ar;
    u64 idx = get_value_arena_idx(a, v, &ar);
    assert(ar != NULL);
    ar->markmap[idx / 64] &= ~(UINT64_C(1) << (idx & 63));
}

bool arena_is_marked(struct arena_handle *a, struct boxed_value *v) {
    struct arena *ar;
    u64 idx = get_value_arena_idx(a, v, &ar);
    assert(ar != NULL);
    return ar->markmap[idx / 64] & UINT64_C(1) << (idx & 63);
}

void arena_promote(struct arena_handle *a, arena_move_fn move_cb, void *data) {
    assert(a->next_gen != NULL);
    struct arena_handle *to = a->next_gen;
    struct arena *ar;
    struct boxed_value *dest;
    while (a) {
        ar = a->self;
        for (u64 i = 0; i < GC_ARENA_NUM_OBJECTS; i++) {
            if (ar->markmap[i / 64] & (UINT64_C(1) << (i & 63))) {
                dest = arena_alloc_val(to);
                memcpy(dest, ar->objs + i, sizeof(struct boxed_value));
                if (move_cb)
                    move_cb(ar->objs + i, dest, data);
// Makes testing *much* easier if we can verify that things
// have been zeroed.
#ifndef NDEBUG
                memset(ar->objs + i, 0, sizeof(struct boxed_value));
#endif
            }
        }
        a = a->next;
    }
}

void arena_empty(struct arena_handle *a) {
    struct arena *ar = a->self;
    ar->next = ar->objs;
    memset(ar->graymap, 0, sizeof(ar->graymap));
    memset(ar->markmap, 0, sizeof(ar->markmap));
#ifndef NDEBUG
    memset(ar->objs, 0, sizeof(ar->objs));
#endif
}

// TODO don't use 2x memory just to compact
struct arena_handle *arena_compact(struct arena_handle *a, arena_move_fn move_cb, void *data) {
    struct arena_handle *to = arena_new(a->mem_ctx);
    while (a) {
        struct arena *ar = a->self;
        u64 *marks = ar->markmap;
        for (u64 i = 0; i < GC_ARENA_NUM_OBJECTS; i++) {
            if (marks[i / 64] & (UINT64_C(1) << (i & 63))) {
                struct boxed_value *v = arena_alloc_val(to);
                memcpy(v, ar->objs + i, sizeof(struct boxed_value));
                if (move_cb)
                    move_cb(ar->objs + i, v, data);
#ifndef NDEBUG
                memset(ar->objs + i, 0, sizeof(struct boxed_value));
#endif
            }
        }
        a = a->next;
    }
    return to;
}
