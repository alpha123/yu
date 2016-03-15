/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "arena.h"

#define SETUP \
    yu_memctx_t mctx; \
    TEST_GET_ALLOCATOR(&mctx); \
    struct arena_handle *a = arena_new(&mctx);

#define TEARDOWN \
    arena_free(a); \
    yu_alloc_ctx_free(&mctx);

#define LIST_ARENA_TESTS(X) \
    X(alloc, "Arenas should be aligned to their size") \
    X(alloc_val, "Arenas should provide objects, expanding themselves if necessary") \
    X(gray_queue, "Arenas should maintain a queue of gray objects") \
    X(empty, "Emptying an arena should reset its object pool") \
    X(promote, "Promoting an arena should copy alive objects to its next generation") \
    X(compact, "Compacting an arena should fill holes left by unmarked objects")

TEST(alloc)
    size_t align = 1 << yu_ceil_log2(sizeof(struct arena));
    PT_ASSERT_EQ((uintptr_t)a->self & (align-1), 0u);
END(alloc)

TEST(alloc_val)
    struct boxed_value *v;
    struct arena *old_a = a->self;
    PT_ASSERT_EQ(arena_allocated_count(a), 0u);

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
#ifdef TEST_FAST
    PT_ASSERT_EQ(a->self, old_a);
#else
    PT_ASSERT_NEQ(a->self, old_a);
#endif

    u32 acount = 0;
    struct arena_handle *ah = a;
    while (ah) {
        ++acount;
        ah = ah->next;
    }
#ifdef TEST_FAST
    PT_ASSERT_EQ(acount, 1u);
#else
    PT_ASSERT_EQ(acount, valcnt/GC_ARENA_NUM_OBJECTS);
#endif
END(alloc_val)

TEST(gray_queue)
    struct boxed_value *v = arena_alloc_val(a), *w = arena_alloc_val(a),
                       *x = arena_alloc_val(a), *y = arena_alloc_val(a),
                       *z = arena_alloc_val(a);

    PT_ASSERT_EQ(arena_gray_count(a), 0u);
    PT_ASSERT_EQ(arena_pop_gray(a), NULL);
    arena_push_gray(a, v);
    arena_push_gray(a, w);
    arena_push_gray(a, x);
    PT_ASSERT_EQ(arena_gray_count(a), 3u);
    PT_ASSERT_EQ(arena_pop_gray(a), v);
    PT_ASSERT_EQ(arena_pop_gray(a), w);
    PT_ASSERT_EQ(arena_gray_count(a), 1u);
    arena_push_gray(a, y);
    arena_push_gray(a, z);
    PT_ASSERT_EQ(arena_pop_gray(a), x);
    PT_ASSERT_EQ(arena_pop_gray(a), y);
    PT_ASSERT_EQ(arena_pop_gray(a), z);
    PT_ASSERT_EQ(arena_pop_gray(a), NULL);
END(gray_queue)

TEST(empty)
    struct boxed_value *x = arena_alloc_val(a), *y = arena_alloc_val(a), *z;
    arena_push_gray(a, x);
    arena_empty(a);
    PT_ASSERT_EQ(arena_allocated_count(a), 0u);
    PT_ASSERT_EQ(arena_gray_count(a), 0u);
    z = arena_alloc_val(a);
    PT_ASSERT_EQ(z, x);
    z = arena_alloc_val(a);
    PT_ASSERT_EQ(z, y);
END(empty)

extern const char *value_type_name(value_type x);

TEST(promote)
    struct arena_handle *b = arena_new(&mctx);
    a->next_gen = b;
#ifdef TEST_FAST
    int valcnt = 4000;
#else
    int valcnt = (int)1e5;
#endif
    for (int i = 0; i < valcnt; i++) {
        struct boxed_value *v = arena_alloc_val(a);
        boxed_value_set_type(v, VALUE_FIXNUM);
        v->v.fx = i;
        if (i & 1) arena_mark(a, v);
    }
    arena_promote(a, NULL, NULL);
    PT_ASSERT_EQ(arena_allocated_count(b), (u32)valcnt/2);
    bool types_ok = true, values_ok = true;
    struct arena *ar = b->self;
    u32 objcnt = elemcount(ar->objs);
    // Compensate for the way objects will fill up; the most recently
    // allocated arena will be partly empty.
    u32 skipcnt = valcnt / 2 - objcnt * (valcnt / 2 / objcnt);
    for (int i = 0; i < (int)(valcnt/2+skipcnt); i++) {
        if ((u32)i >= skipcnt)
            continue;
        if (i > 0 && i % objcnt == 0) {
            b = b->next;
            ar = b->self;
        }
        if (boxed_value_get_type(ar->objs + i % objcnt) != VALUE_FIXNUM) {
            types_ok = false;
            break;
        }
        // We don't know the exact order they got copied in so we
        // can't check exact values, but we can make sure they're
        // all odd.
        if ((ar->objs[i % objcnt].v.fx & 1) == 0) {
            values_ok = false;
            break;
        }
    }
    PT_ASSERT(types_ok);
    PT_ASSERT(values_ok);
END(promote)

TEST(compact)
#ifdef TEST_FAST
    int valcnt = 4000;
#else
    int valcnt = (int)1e5;
#endif

    for (int i = 0; i < valcnt; i++) {
        struct boxed_value *v = arena_alloc_val(a);
        boxed_value_set_type(v, VALUE_FIXNUM);
        v->v.fx = i;
        if (i & 1) arena_mark(a, v);
    }
    arena_compact(a, NULL, NULL);
    PT_ASSERT_EQ(arena_allocated_count(a), (u32)valcnt/2);

    bool contiguous = true;
    struct arena_handle *ah = a;
    while (ah) {
        for (u32 i = 0; i < elemcount(ah->self->markmap); i++) {
            u64 marked = ah->self->markmap[i];
            if (marked != (u64)-1) {
                u64 s = (u64)-1 << __builtin_ctzll(marked);
                if (marked != s &&
                        (marked != 0 &&
                         (i == elemcount(ah->self->markmap) - 1 ||
                          ah->self->markmap[i+1] == 0))) {
                    contiguous = false;
                    goto dengo;
                }
            }
        }
        ah = ah->next;
    }
    dengo:
    PT_ASSERT(contiguous);
END(compact)


SUITE(arena, LIST_ARENA_TESTS)
