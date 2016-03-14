/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "gc.h"

YU_INLINE
int root_list_ptr_cmp(value_handle a, value_handle b) {
    uintptr_t x = (uintptr_t)value_deref(a), y = (uintptr_t)value_deref(b);
    return (x > y) - (x < y);
}

YU_INLINE
int arena_gray_cmp(struct arena_handle *a, struct arena_handle *b) {
    u64 x = arena_gray_count(a), y = arena_gray_count(b);
    return (x > y) - (x < y);
}

YU_SPLAYTREE_IMPL(root_list, value_handle, root_list_ptr_cmp, true)
YU_QUICKHEAP_IMPL(arena_heap, struct arena_handle *, arena_gray_cmp, YU_QUICKHEAP_MAXHEAP)

YU_ERR_RET gc_init(struct gc_info *gc, yu_memctx_t *mctx) {
    YU_ERR_DEFVAR

    root_list_init(&gc->roots, mctx);
    arena_heap_init(&gc->a_gray, GC_NUM_GENERATIONS, mctx);

    gc->mem_ctx = mctx;

    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        YU_CHECK_ALLOC(gc->arenas[i] = arena_new(mctx));
    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        gc->arenas[i]->next_gen = i < GC_NUM_GENERATIONS-1 ? gc->arenas[i+1] : NULL;

    gc->hs = yu_xalloc(gc->mem_ctx, 1, sizeof(struct gc_handle_set));

    gc->alloc_pressure_score = 0;
    gc->active_gray = NULL;

    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void gc_free(struct gc_info *gc) {
    root_list_free(&gc->roots);
    arena_heap_free(&gc->a_gray);
    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        arena_free(gc->arenas[i]);

    struct gc_handle_set *h = gc->hs, *next;
    while (h) {
        next = h->next;
        yu_free(gc->mem_ctx, h);
        h = next;
    }
}

value_handle gc_make_handle(struct gc_info *gc, struct boxed_value *v) {
    struct gc_handle_set *h = gc->hs, *prev = NULL;
    while (h) {
        for (u32 i = 0; i < elemcount(h->handles); i++) {
            if (h->handles[i] == v)
                return h->handles + i;
            else if (h->handles[i] == NULL) {
                h->handles[i] = v;
                return h->handles + i;
            }
        }
        prev = h;
        h = h->next;
    }
    h = yu_xalloc(gc->mem_ctx, 1, sizeof(struct gc_handle_set));
    prev->next = h;
    h->handles[0] = v;
    return h->handles;
}

value_handle gc_alloc_val(struct gc_info *gc, value_type type) {
    struct boxed_value *v = arena_alloc_val(gc->arenas[0]);
    boxed_value_set_type(v, type);
    boxed_value_set_gray(v, boxed_value_is_traversable(v));
    if (type == VALUE_TABLE) {
        v->v.tbl = yu_xalloc(gc->mem_ctx, 1, sizeof(value_table));
        value_table_init(v->v.tbl, 10, gc->mem_ctx);
    }
    else if (type == VALUE_TUPLE)
        v->v.tup[0] = v->v.tup[1] = v->v.tup[2] = value_empty();
    return gc_make_handle(gc, v);
}

void gc_root(struct gc_info *gc, value_handle v) {
    bool already_rooted = root_list_insert(&gc->roots, v, NULL);
    assert(!already_rooted);
    gc_mark(gc, value_deref(v));
}

void gc_unroot(struct gc_info *gc, value_handle v) {
    bool was_rooted = root_list_remove(&gc->roots, v, NULL);
    assert(was_rooted);
}

static
void push_gray(struct gc_info *gc, struct boxed_value *v) {
    struct arena_handle *a = boxed_value_owner(v);
    arena_push_gray(a, v);
    if (arena_gray_count(a) == 1)
        arena_heap_push(&gc->a_gray, a);
}

void gc_barrier(struct gc_info *gc, value_handle val) {
    struct boxed_value *v = value_deref(val);
    if (!boxed_value_is_gray(v)) {
        struct arena_handle *a = boxed_value_owner(v);
        boxed_value_set_gray(v, true);
        if (arena_is_marked(a, v)) {
            // Force a rescan of the written to object
            push_gray(gc, v);
            // TODO while it works just fine, it's not very clean nor
            // understandable to unmark the value here.
            arena_unmark(a, v);
        }
    }
}

void gc_set_gray(struct gc_info *gc, struct boxed_value *v) {
    if (boxed_value_is_traversable(v)) {
        boxed_value_set_gray(v, true);
        push_gray(gc, v);
    }
    else  // Non-traversable objects go straight from white -> black
        arena_mark(boxed_value_owner(v), v);
}

static
u32 mark_table(value_t key, value_t val, void *data) {
    if (value_is_ptr(key))
        gc_set_gray((struct gc_info *)data, value_get_ptr(key));
    if (value_is_ptr(val))
        gc_set_gray((struct gc_info *)data, value_get_ptr(val));
    return 0;
}

static
s32 mark_tuple(value_t val, void *data) {
    if (value_is_ptr(val))
        gc_set_gray((struct gc_info *)data, value_get_ptr(val));
    return 0;
}

void gc_mark(struct gc_info *gc, struct boxed_value *v) {
    arena_mark(boxed_value_owner(v), v);
    boxed_value_set_gray(v, false);
    switch (boxed_value_get_type(v)) {
    case VALUE_TABLE:
        value_table_iter(v->v.tbl, mark_table, gc);
        break;
    case VALUE_TUPLE:
        value_tuple_foreach(v, mark_tuple, gc);
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

static
bool scan_step(struct gc_info *gc) {
    struct boxed_value *v = gc_next_gray(gc);
    if (v && !arena_is_marked(boxed_value_owner(v), v))
        gc_mark(gc, v);
    return !!v;
}

bool gc_scan_step(struct gc_info *gc) {
    bool sweep = false;
    for (u32 i = 0; i < GC_INCREMENTAL_STEP_COUNT; i++) {
        if (!scan_step(gc)) {
            sweep = true;
            break;
        }
    }
    if (sweep) {
        gc_sweep(gc);
        return true;
    }
    return false;
}

static
void move_ptr(struct boxed_value *old_ptr, struct boxed_value *new_ptr, void *data) {
    struct gc_handle_set *h = ((struct gc_info *)data)->hs;
    while (h) {
        for (u32 i = 0; i < elemcount(h->handles); i++) {
            if (h->handles[i] == old_ptr) {
                h->handles[i] = new_ptr;
                return;
            }
        }
        h = h->next;
    }
    assert(false);
}

void gc_sweep(struct gc_info *gc) {
    gc->arenas[GC_NUM_GENERATIONS-1] = arena_compact(gc->arenas[GC_NUM_GENERATIONS-1], move_ptr, gc);
    for (u8 i = GC_NUM_GENERATIONS-1; i > 0; i--) {
        arena_promote(gc->arenas[i-1], move_ptr, gc);
        arena_empty(gc->arenas[i-1]);
    }
    // `promote` will have un-marked all root objects, so let's go ahead and do that again
    struct root_list_nodelist *n = gc->roots.nodes;
    while (n) {
        gc_mark(gc, value_deref(n->n->dat));
        n = n->next;
    }
}
