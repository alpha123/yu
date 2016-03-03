/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "sys_alloc.h"

#define SETUP \
    yu_memctx_t ctx; \
    sys_alloc_ctx_init(&ctx);

#define TEARDOWN \
    yu_alloc_ctx_free(&ctx);

#define LIST_ALLOC_TESTS(X) \
    X(alloc, "alloc() should allocate usable space of the requested size") \
    X(alloc_zero, "alloc() should initialize the requested space to 0") \
    X(alloc_free, "free() should free the allocated space") \
    X(free_all, "All memory allocated within a context should be freed upon freeing the context") \
    X(alloc_aligned, "Allocated pointers should obey the specified alignment")

struct foo {
    u64 ll;
    bool b;
    char *s;
};

TEST(alloc)
    struct foo *f = yu_xalloc(&ctx, 1, sizeof(struct foo));
    size_t f_sz, ns_sz;
    PT_ASSERT(sysmem_tbl_get((sysmem_tbl *)ctx.adata, f, &f_sz));
    PT_ASSERT_EQ(f_sz, sizeof(struct foo));

    int *ns = yu_xalloc(&ctx, 50, sizeof(int));
    PT_ASSERT(sysmem_tbl_get((sysmem_tbl *)ctx.adata, ns, &ns_sz));
    PT_ASSERT_EQ(ns_sz, 50 * sizeof(int));
END(alloc)

TEST(alloc_zero)
    struct foo *f = yu_xalloc(&ctx, 1, sizeof(struct foo));
    PT_ASSERT_EQ(f->ll, UINT64_C(0));
    PT_ASSERT_EQ(f->b, false);
    PT_ASSERT_EQ(f->s, NULL);

    int *ns;
    assert(yu_alloc(&ctx, &ns, 50, sizeof(int), 64) == YU_OK);
    bool all_z = true;
    for (int i = 0; i < 50; i++) {
        if (ns[i] != 0) {
            all_z = false;
            break;
        }
    }
    PT_ASSERT(all_z);
END(alloc_zero)

TEST(alloc_free)
    struct foo *f = yu_xalloc(&ctx, 1, sizeof(struct foo));
    yu_free(&ctx, f);
    PT_ASSERT(!sysmem_tbl_get((sysmem_tbl *)ctx.adata, f, NULL));
END(alloc_free)

TEST(free_all)
    struct foo *f = yu_xalloc(&ctx, 1, sizeof(struct foo)),
               *fs = yu_xalloc(&ctx, 3, sizeof(struct foo));
    char *s = yu_xalloc(&ctx, 20, 1);
    yu_alloc_ctx_free(&ctx);
    // We can't actually test if the table contains
    // f/fs/s like above, since it's been freed.
    // Instead re-create it and make sure it got rid
    // of all the other junk.
    // TODO figure out how to test if f/fs/n are actually
    // free. Until then, Valgrind.
    sys_alloc_ctx_init(&ctx);
    PT_ASSERT(!sysmem_tbl_get((sysmem_tbl *)ctx.adata, f, NULL));
    PT_ASSERT(!sysmem_tbl_get((sysmem_tbl *)ctx.adata, fs, NULL));
    PT_ASSERT(!sysmem_tbl_get((sysmem_tbl *)ctx.adata, s, NULL));
END(free_all)

TEST(alloc_aligned)
    int *n;
    // Remember kids, always check your error return values.
    // Unless, of course, you're in a test suite. Then you
    // can be a dirty rebel.
    // Put in the assert() just to shut $CC up, it won't
    // affect the test suite.
    assert(yu_alloc(&ctx, &n, 1, sizeof(int), 2) == YU_OK);
    PT_ASSERT_EQ((uintptr_t)n % 2, 0);
    yu_free(&ctx, n);

    assert(yu_alloc(&ctx, &n, 128, sizeof(int), 64) == YU_OK);
    PT_ASSERT_EQ((uintptr_t)n % 64, 0);
    yu_free(&ctx, n);

    assert(yu_alloc(&ctx, &n, 6000, sizeof(int), 4096) == YU_OK);
    PT_ASSERT_EQ((uintptr_t)n % 4096, 0);
    yu_free(&ctx, n);
END(alloc_aligned)

SUITE(alloc, LIST_ALLOC_TESTS)
