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
        gc->arenas[i]->next_gen = i < GC_NUM_GENERATIONS - 1 ? gc->arenas[i] : NULL;

    gc->alloc_pressure_score = 0;

    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void gc_free(struct gc_info *gc) {
    root_list_free(&gc->roots);
    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        arena_free(gc->arenas[i]);
}

void gc_root(struct gc_info *gc, struct boxed_value *v) {
    root_list_insert(&gc->roots, v, NULL);
}

void gc_unroot(struct gc_info *gc, struct boxed_value *v) {
    root_list_remove(&gc->roots, v, NULL);
}
