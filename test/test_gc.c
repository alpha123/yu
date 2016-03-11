/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "gc.h"

#define SETUP \
    yu_memctx_t mctx; \
    TEST_GET_ALLOCATOR(&mctx); \
    struct gc_info gc; \
    yu_err _gciniterr = gc_init(&gc, &mctx); \
    assert(_gciniterr == YU_OK); \
    struct arena_handle *a = gc.arenas[0], *b = gc.arenas[1], \
        *c = gc.arenas[2];

#define TEARDOWN \
    gc_free(&gc); \
    yu_alloc_ctx_free(&mctx);

#define LIST_GC_TESTS(X) \
    X(next_gray, "The GC should know the next gray object to scan") \
    X(root, "Rooted objects should not be freed in a GC cycle") \
    //X(sanity_check, "The GC should be able to allocate and free objects")

TEST(next_gray)
    struct boxed_value *v = arena_alloc_val(a), *w = arena_alloc_val(a), *x, *y, *z;
    PT_ASSERT_EQ(gc_next_gray(&gc), NULL);
    gc_set_gray(&gc, v);
    gc_set_gray(&gc, w);
    PT_ASSERT_EQ(gc_next_gray(&gc), v);
    PT_ASSERT_EQ(gc_next_gray(&gc), w);
    PT_ASSERT_EQ(gc_next_gray(&gc), NULL);

    x = arena_alloc_val(b);
    y = arena_alloc_val(b);
    z = arena_alloc_val(c);
    gc_set_gray(&gc, v);
    gc_set_gray(&gc, x);
    gc_set_gray(&gc, y);
    gc_set_gray(&gc, z);
    PT_ASSERT_EQ(gc_next_gray(&gc), x);
    PT_ASSERT_EQ(gc_next_gray(&gc), y);
    PT_ASSERT_EQ(gc_next_gray(&gc), v);
    PT_ASSERT_EQ(gc_next_gray(&gc), z);
END(next_gray)

TEST(root)
    struct boxed_value *v = gc_alloc_val(&gc, VALUE_FIXNUM), *w = gc_alloc_val(&gc, VALUE_FIXNUM);
    v->v.fx = 10;
    w->v.fx = 20;
    gc_root(&gc, v);
    while (!gc_scan_step(&gc)) { }
    PT_ASSERT_EQ(boxed_value_get_type(b->self->objs), VALUE_FIXNUM);
    PT_ASSERT_EQ(b->self->objs->v.fx, 10);
    // Explicit cast because typeof() naturally returns differently for pointers
    // and arrays, and the ASSERT_EQ macro uses typeof.
    PT_ASSERT_EQ(a->self->next, (struct boxed_value *)a->self->objs);
    PT_ASSERT_EQ(b->self->next, (b->self->objs + 1));
END(root)


SUITE(gc, LIST_GC_TESTS)
