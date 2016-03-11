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
    X(next_gray, "The GC should know the next gray object to scan")

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


SUITE(gc, LIST_GC_TESTS)
