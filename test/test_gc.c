/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "gc.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#define SETUP \
    TEST_GET_ALLOCATOR(mctx); \
    struct gc_info gc; \
    yu_err _gciniterr = gc_init(&gc, (yu_allocator *)&mctx);  \
    assert(_gciniterr == YU_OK); \
    struct arena_handle *a = gc.arenas[0], *b = gc.arenas[1], \
        *c = gc.arenas[2];

#define TEARDOWN \
    gc_free(&gc); \
    yu_alloc_ctx_free(&mctx);

#define LIST_GC_TESTS(X) \
    X(handle, "Handles to objects should remain valid when the object is moved") \
    X(next_gray, "The GC should know the next gray object to scan") \
    X(root, "Rooted objects should not be freed in a GC cycle") \
    X(object_graph, "The GC should correctly traverse the object graph, including cycles") \
    X(write_barrier, "Objects written to after being scanned should be re-scanned") \
    X(sanity_check, "GC should work")

TEST(handle)
    value_handle x = gc_alloc_val(&gc, VALUE_FIXNUM),
        y = gc_alloc_val(&gc, VALUE_FIXNUM);
    struct boxed_value *old_loc = value_deref(y), *new_loc;
    old_loc->v.fx = 42;
    arena_mark(a, old_loc);
    gc_sweep(&gc);
    new_loc = value_deref(y);
    PT_ASSERT_EQ(new_loc->v.fx, 42);
    PT_ASSERT_NEQ(old_loc, new_loc);
END(handle)

TEST(next_gray)
    struct boxed_value *v = arena_alloc_val(a), *w = arena_alloc_val(a), *x, *y, *z;
    boxed_value_set_type(v, VALUE_TUPLE);
    boxed_value_set_type(w, VALUE_TUPLE);
    PT_ASSERT_EQ(gc_next_gray(&gc), NULL);
    gc_set_gray(&gc, v);
    gc_set_gray(&gc, w);
    PT_ASSERT_EQ(gc_next_gray(&gc), v);
    PT_ASSERT_EQ(gc_next_gray(&gc), w);
    PT_ASSERT_EQ(gc_next_gray(&gc), NULL);

    x = arena_alloc_val(b);
    y = arena_alloc_val(b);
    z = arena_alloc_val(c);
    boxed_value_set_type(x, VALUE_TUPLE);
    boxed_value_set_type(y, VALUE_TUPLE);
    boxed_value_set_type(z, VALUE_TUPLE);
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
    value_handle v = gc_alloc_val(&gc, VALUE_FIXNUM), w = gc_alloc_val(&gc, VALUE_FIXNUM);
    value_deref(v)->v.fx = 10;
    value_deref(w)->v.fx = 20;
    gc_root(&gc, v);
    while (!gc_scan_step(&gc)) { }
    PT_ASSERT_EQ(boxed_value_get_type(value_deref(w)), VALUE_ERR); // 0
    PT_ASSERT_EQ(boxed_value_get_type(b->self->objs), VALUE_FIXNUM);
    PT_ASSERT_EQ(b->self->objs->v.fx, 10);
    // Explicit cast because typeof() naturally returns differently for pointers
    // and arrays, and the ASSERT_EQ macro uses typeof.
    PT_ASSERT_EQ(a->self->next, (struct boxed_value *)a->self->objs);
    PT_ASSERT_EQ(b->self->next, (b->self->objs + 1));
END(root)

TEST(object_graph)
    value_handle tup = gc_alloc_val(&gc, VALUE_TUPLE),
        tup2 = gc_alloc_val(&gc, VALUE_TUPLE),
        tup3 = gc_alloc_val(&gc, VALUE_TUPLE),
        v = gc_alloc_val(&gc, VALUE_FIXNUM), w = gc_alloc_val(&gc, VALUE_FIXNUM),
        x = gc_alloc_val(&gc, VALUE_FIXNUM), y = gc_alloc_val(&gc, VALUE_FIXNUM);

    value_deref(v)->v.fx = 1;
    value_deref(w)->v.fx = 2;
    value_deref(x)->v.fx = 3;
    value_deref(y)->v.fx = 4;

    value_deref(tup2)->v.tup[0] = value_from_ptr(v);
    value_deref(tup2)->v.tup[1] = value_from_ptr(w);
    value_deref(tup)->v.tup[0] = value_from_ptr(tup2);
    value_deref(tup)->v.tup[1] = value_from_ptr(x);

    value_deref(tup3)->v.tup[0] = value_from_ptr(y);
    value_deref(tup3)->v.tup[1] = value_from_ptr(tup3);

    gc_root(&gc, tup);
    while (!gc_scan_step(&gc)) { }

    PT_ASSERT_EQ(arena_allocated_count(a), 0u);
    // tup, tup2, v, w, x
    PT_ASSERT_EQ(arena_allocated_count(b), 5u);

    PT_ASSERT_EQ(boxed_value_get_type(b->self->objs), VALUE_TUPLE);
    PT_ASSERT_EQ(boxed_value_get_type(b->self->objs + 1), VALUE_TUPLE);
    PT_ASSERT_EQ(boxed_value_get_type(b->self->objs + 2), VALUE_FIXNUM);
    PT_ASSERT_EQ(boxed_value_get_type(b->self->objs + 3), VALUE_FIXNUM);
    PT_ASSERT_EQ(boxed_value_get_type(b->self->objs + 4), VALUE_FIXNUM);

    PT_ASSERT_EQ(boxed_value_get_type(value_deref(tup3)), VALUE_ERR);
    PT_ASSERT_EQ(boxed_value_get_type(value_deref(y)), VALUE_ERR);
END(object_graph)

TEST(write_barrier)
    value_handle tup = gc_alloc_val(&gc, VALUE_TUPLE), x, y;

    gc_root(&gc, tup);
    gc_scan_step(&gc);

    x = gc_alloc_val(&gc, VALUE_FIXNUM);
    value_deref(x)->v.fx = 10;
    gc_barrier(&gc, tup);
    value_deref(tup)->v.tup[0] = value_from_ptr(x);
    gc_scan_step(&gc);

    y = gc_alloc_val(&gc, VALUE_FIXNUM);
    value_deref(y)->v.fx = 20;
    gc_barrier(&gc, tup);
    value_deref(tup)->v.tup[1] = value_from_ptr(y);

    while (!gc_scan_step(&gc)) { }
    PT_ASSERT_EQ(arena_allocated_count(b)+arena_allocated_count(c), 3u);
    PT_ASSERT_EQ(value_deref(x)->v.fx, 10);
    PT_ASSERT_EQ(value_deref(y)->v.fx, 20);
END(write_barrier)

TEST(sanity_check)
#ifdef TEST_FAST
    int valcnt = 2000;
#elif TEST_STRESS
    int valcnt = 50000;
#else
    int valcnt = 10000;
#endif
    value_handle root = gc_alloc_val(&gc, VALUE_TABLE);
    gc_root(&gc, root);
    for (int i = 0; i < valcnt; i++) {
	value_handle v = gc_alloc_val(&gc, VALUE_FIXNUM);
	value_deref(v)->v.fx = i;
        gc_barrier(&gc, root);
	value_table_put(value_deref(root)->v.tbl, value_from_int(i), value_from_ptr(v), NULL);
	gc_scan_step(&gc);
    }
    while (!gc_scan_step(&gc)) { }
    PT_ASSERT_EQ(arena_allocated_count(b)+arena_allocated_count(c), (u32)valcnt+1);
    gc_unroot(&gc, root);

    value_handle v, w, x, y, z;
    v = gc_alloc_val(&gc, VALUE_TUPLE);
    gc_root(&gc, v);
    gc_scan_step(&gc);
    w = gc_alloc_val(&gc, VALUE_TUPLE);
    gc_barrier(&gc, v);
    value_deref(v)->v.tup[0] = value_from_ptr(w);
    value_deref(w)->v.tup[0] = value_from_ptr(v);
    value_deref(w)->v.tup[1] = value_from_ptr(w);
    gc_scan_step(&gc);

    x = gc_alloc_val(&gc, VALUE_TUPLE);
    value_deref(x)->v.tup[0] = value_from_ptr(v);
    y = gc_alloc_val(&gc, VALUE_TABLE);
    value_table_put(value_deref(y)->v.tbl, value_from_int(10), value_from_ptr(x), NULL);
    z = gc_alloc_val(&gc, VALUE_TABLE);
    value_table_put(value_deref(z)->v.tbl, value_from_int(30), value_from_ptr(y), NULL);
    gc_barrier(&gc, w);
    value_deref(w)->v.tup[1] = value_from_ptr(x);

    gc.collecting_generation = GC_NUM_GENERATIONS-1;
    gc_full_collect(&gc);
    // Double collect to make sure the oldest generation is fully cleaned up
    // so that we have an accurate count of living objects.
    gc_full_collect(&gc);
    // v, w, x alive, y, z, dead
    PT_ASSERT_EQ(arena_allocated_count(b)+arena_allocated_count(c), 3u);
END(sanity_check)


SUITE(gc, LIST_GC_TESTS)

#pragma GCC diagnostic pop
