/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "sys_alloc.h"
#include "arena.h"

#define SETUP \
    yu_memctx_t mctx; \
    sys_alloc_ctx_init(&mctx); \
    struct arena_handle *a = arena_new(&mctx);

#define TEARDOWN \
    arena_free(a); \
    sys_alloc_ctx_free(&mctx);

#define LIST_ARENA_TESTS(X) \
    X(alloc, "Arenas should be aligned to their size") \
    X(alloc_val, "Arenas should provide objects, expanding themselves if necessary") \
    X(gray_queue, "Arenas should maintain a queue of gray objects")

TEST(alloc)
    size_t align = 1 << yu_ceil_log2(sizeof(struct arena));
    PT_ASSERT_EQ((uintptr_t)a->self & (align-1), 0);
END(alloc)

TEST(alloc_val)
    struct boxed_value *v;
    struct arena *old_a = a->self;
    PT_ASSERT_EQ(arena_allocated_count(a), 0);

#ifdef TEST_FAST
    u32 valcnt = 1000;
#else
    u32 valcnt = 1 << 20;
#endif

    for (u32 i = 0; i < valcnt; i++) {
        v = arena_alloc_val(a);
        if (v == NULL)
            break;
    }
    PT_ASSERT_NEQ(v, NULL);
    PT_ASSERT_EQ(arena_allocated_count(a), valcnt);
    PT_ASSERT_NEQ(a->self, old_a);

    u32 acount = 0;
    struct arena_handle *ah = a;
    while (ah) {
        ++acount;
        ah = ah->next;
    }
    PT_ASSERT_EQ(acount, valcnt/GC_ARENA_NUM_OBJECTS);
END(alloc_val)

TEST(gray_queue)
    struct boxed_value *v = arena_alloc_val(a), *w = arena_alloc_val(a),
                       *x = arena_alloc_val(a), *y = arena_alloc_val(a),
                       *z = arena_alloc_val(a);

    PT_ASSERT_EQ(arena_gray_count(a), 0);
    PT_ASSERT_EQ(arena_pop_gray(a), NULL);
    arena_push_gray(a, v);
    arena_push_gray(a, w);
    arena_push_gray(a, x);
    PT_ASSERT_EQ(arena_gray_count(a), 3);
    PT_ASSERT_EQ(arena_pop_gray(a), v);
    PT_ASSERT_EQ(arena_pop_gray(a), w);
    PT_ASSERT_EQ(arena_gray_count(a), 1);
    arena_push_gray(a, y);
    arena_push_gray(a, z);
    PT_ASSERT_EQ(arena_pop_gray(a), x);
    PT_ASSERT_EQ(arena_pop_gray(a), y);
    PT_ASSERT_EQ(arena_pop_gray(a), z);
    PT_ASSERT_EQ(arena_pop_gray(a), NULL);
END(gray_queue)


SUITE(arena, LIST_ARENA_TESTS)
