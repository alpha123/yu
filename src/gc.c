/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "gc.h"

YU_INLINE
int gc_root_list_ptr_cmp(struct boxed_value *a, struct boxed_value *b) {
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return (x > y) - (x < y);
}

YU_INLINE
int gc_arena_gray_cmp(struct arena_handle *a, struct arena_handle *b) {
    u64 x = arena_gray_count(a), y = arena_gray_count(b);
    return (x > y) - (x < y);
}

YU_SPLAYTREE_IMPL(root_list, struct boxed_value *, gc_root_list_ptr_cmp, true)
YU_QUICKHEAP_IMPL(arena_heap, struct arena_handle *, gc_arena_gray_cmp, YU_QUICKHEAP_MAXHEAP)

YU_ERR_RET gc_init(struct gc_info *gc, yu_memctx_t *mctx) {
    YU_ERR_DEFVAR

    root_list_init(&gc->roots, mctx);

    arena_heap_init(&gc->a_gray, GC_NUM_GENERATIONS, mctx);

    gc->mem_ctx = mctx;

    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        YU_CHECK_ALLOC(gc->arenas[i] = arena_new(mctx));
    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        gc->arenas[i]->next_gen = i < GC_NUM_GENERATIONS-1 ? gc->arenas[i+1] : NULL;

    gc->alloc_pressure_score = 0;
    gc->active_gray = NULL;

    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void gc_free(struct gc_info *gc) {
    root_list_free(&gc->roots);
    arena_heap_free(&gc->a_gray);
    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        arena_free(gc->arenas[i]);
}

struct boxed_value *gc_alloc_val(struct gc_info *gc, value_type type) {
    struct boxed_value *v = arena_alloc_val(gc->arenas[0]);
    boxed_value_set_type(v, type);
    boxed_value_set_gray(v, type == VALUE_TABLE);
    return v;
}

void gc_root(struct gc_info *gc, struct boxed_value *v) {
    bool already_rooted = root_list_insert(&gc->roots, v, NULL);
    assert(!already_rooted);
    gc_mark(gc, v);
}

void gc_unroot(struct gc_info *gc, struct boxed_value *v) {
    bool was_rooted = root_list_remove(&gc->roots, v, NULL);
    assert(was_rooted);
}

void gc_barrier(struct gc_info *gc, struct boxed_value *v) {
    if (!boxed_value_is_gray(v))
        gc_set_gray(gc, v);
}

void gc_set_gray(struct gc_info *gc, struct boxed_value *v) {
    struct arena_handle *a = boxed_value_owner(v);
    boxed_value_set_gray(v, true);
    arena_push_gray(a, v);
    if (arena_gray_count(a) == 1)
        arena_heap_push(&gc->a_gray, a);
}

static
void queue_mark(struct gc_info *gc, struct boxed_value *v) {
    arena_mark(boxed_value_owner(v), v);
    gc_set_gray(gc, v);
}

static
u32 mark_table(value_t key, value_t val, void *data) {
    if (value_is_ptr(key))
        queue_mark((struct gc_info *)data, value_to_ptr(key));
    if (value_is_ptr(val))
        queue_mark((struct gc_info *)data, value_to_ptr(val));
    return 0;
}

void gc_mark(struct gc_info *gc, struct boxed_value *v) {
    queue_mark(gc, v);
    switch (boxed_value_get_type(v)) {
    case VALUE_TABLE:
        value_table_iter(v->v.tbl, mark_table, gc);
        break;
    default:
	break;
    }
}

struct boxed_value *gc_next_gray(struct gc_info *gc) {
    if (gc->active_gray == NULL) {
        if ((gc->active_gray = arena_heap_pop(&gc->a_gray, NULL)) == NULL)
            return NULL;
    }

    struct boxed_value *v = arena_pop_gray(gc->active_gray);
    if (arena_gray_count(gc->active_gray) == 0)
        gc->active_gray = NULL;
    return v;
}

// Rescan the root set (mostly the Yu stack) before the final sweep
// to avoid a write barrier on stack pushes/pops.
static
void rescan_roots(struct gc_info *gc) {
    struct root_list_nodelist *n = gc->roots.nodes;
    while (n) {
        gc_mark(gc, n->n->dat);
        n = n->next;
    }
}

static
bool scan_step(struct gc_info *gc) {
    struct boxed_value *v = gc_next_gray(gc);
    if (v && !arena_is_marked(boxed_value_owner(v), v))
        gc_mark(gc, v);
    return !!v;
}

bool gc_scan_step(struct gc_info *gc) {
    if (!scan_step(gc)) {
        rescan_roots(gc);
        while (scan_step(gc)) { }
        gc_sweep(gc);
        return true;
    }
    return false;
}

void gc_sweep(struct gc_info *gc) {
    gc->arenas[GC_NUM_GENERATIONS-1] = arena_compact(gc->arenas[GC_NUM_GENERATIONS-1]);
    for (u8 i = GC_NUM_GENERATIONS-1; i > 0; i--) {
        arena_promote(gc->arenas[i-1]);
        arena_empty(gc->arenas[i-1]);
    }
}
