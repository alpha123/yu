/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "sys_alloc.h"

#define SETUP \
    sys_allocator ctx; \
    sys_alloc_ctx_init(&ctx);

#define TEARDOWN \
    yu_alloc_ctx_free(&ctx);

#define LIST_ALLOC_TESTS(X) \
    X(alloc, "alloc() should allocate usable space of the requested size") \
    X(alloc_zero, "alloc() should initialize the requested space to 0") \
    X(alloc_free, "free() should free the allocated space") \
    X(alloc_size, "Allocator should know the size of allocations") \
    X(usable_size, "Usable space should always be >= allocation size") \
    X(free_all, "All memory allocated within a context should be freed upon freeing the context") \
    X(alloc_aligned, "Allocated pointers should obey the specified alignment") \
    X(alloc_pages, "Allocator should be able to request virtual pages from the underlying OS and release them on cleanup") \
    X(page_free, "free() on a reserved address space should be equivalent to decommit+release") \
    X(page_sizes, "_size() functions on a reserved address space should return the requested and rounded sizes")

struct foo {
    u64 ll;
    bool b;
    char *s;
};

TEST(alloc)
    struct foo *f = yu_xalloc(&ctx, 1, sizeof(struct foo));
    size_t f_sz, ns_sz;
    PT_ASSERT(sysmem_tbl_get(&ctx.allocd, f, &f_sz));
    PT_ASSERT_EQ(f_sz, sizeof(struct foo));

    int *ns = yu_xalloc(&ctx, 50, sizeof(int));
    PT_ASSERT(sysmem_tbl_get(&ctx.allocd, ns, &ns_sz));
    PT_ASSERT_EQ(ns_sz, 50 * sizeof(int));
END(alloc)

TEST(alloc_zero)
    struct foo *f = yu_xalloc(&ctx, 1, sizeof(struct foo));
    PT_ASSERT_EQ(f->ll, UINT64_C(0));
    PT_ASSERT_EQ(f->b, false);
    PT_ASSERT_EQ(f->s, NULL);

    int *ns;
    yu_err err = yu_alloc(&ctx, (void **)&ns, 50, sizeof(int), 64);
    assert(err == YU_OK);
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
    PT_ASSERT(!sysmem_tbl_get(&ctx.allocd, f, NULL));
END(alloc_free)

TEST(alloc_size)
    struct foo *f = yu_xalloc(&ctx, 3, sizeof(struct foo));
    PT_ASSERT_EQ(yu_allocated_size(&ctx, f), 3 * sizeof(struct foo));
    f = yu_xrealloc(&ctx, f, 10, sizeof(struct foo));
    PT_ASSERT_EQ(yu_allocated_size(&ctx, f), 10 * sizeof(struct foo));
END(alloc_size)

TEST(usable_size)
    struct foo *f = yu_xalloc(&ctx, 3, sizeof(struct foo));
    PT_ASSERT_GTE(yu_usable_size(&ctx, f), 3 * sizeof(struct foo));
    f = yu_xrealloc(&ctx, f, 10, sizeof(struct foo));
    PT_ASSERT_GTE(yu_usable_size(&ctx, f), 10 * sizeof(struct foo));
END(usable_size)

TEST(free_all)
    struct foo *f = yu_xalloc(&ctx, 1, sizeof(struct foo)),
               *fs = yu_xalloc(&ctx, 3, sizeof(struct foo));
    char *s = yu_xalloc(&ctx, 20, 1);
    yu_alloc_ctx_free(&ctx);
    // We can't actually test if the table contains
    // f/fs/s like above, since it's been freed.
    // Instead re-create it and make sure it got rid
    // of all the other junk.
    // TODO figure out how to test if f/fs/s are actually
    // free. Until then, Valgrind.
    sys_alloc_ctx_init(&ctx);
    PT_ASSERT(!sysmem_tbl_get(&ctx.allocd, f, NULL));
    PT_ASSERT(!sysmem_tbl_get(&ctx.allocd, fs, NULL));
    PT_ASSERT(!sysmem_tbl_get(&ctx.allocd, s, NULL));
END(free_all)

TEST(alloc_aligned)
    int *n;
    // Remember kids, always check your error return values.
    // Unless, of course, you're in a test suite. Then you
    // can be a dirty rebel.
    // Put in the assert() just to shut $CC up, it won't
    // affect the test suite.
    yu_err err = yu_alloc(&ctx, (void **)&n, 1, sizeof(int), 2);
    assert(err == YU_OK);
    PT_ASSERT_EQ((uintptr_t)n % 2, 0u);
    yu_free(&ctx, n);

    err = yu_alloc(&ctx, (void **)&n, 128, sizeof(int), 64);
    assert(err == YU_OK);
    PT_ASSERT_EQ((uintptr_t)n % 64, 0u);
    yu_free(&ctx, n);

    err = yu_alloc(&ctx, (void **)&n, 6000, sizeof(int), 4096);
    assert(err == YU_OK);
    PT_ASSERT_EQ((uintptr_t)n % 4096, 0u);
    yu_free(&ctx, n);
END(alloc_aligned)

TEST(alloc_pages)
END(alloc_pages)

TEST(page_free)
END(page_free)

TEST(page_sizes)
    void *ptr;
    yu_err ok = yu_reserve(&ctx, &ptr, 1024, 1);
    PT_ASSERT(ok == YU_OK);
    PT_ASSERT(ptr != NULL);
    PT_ASSERT_EQ(yu_allocated_size(&ctx, ptr), 1024u);
    PT_ASSERT_EQ(yu_usable_size(&ctx, ptr), yu_virtual_pagesize(0));
END(page_sizes)

#if TEST_ALLOC == TEST_USE_SYS_ALLOC
// Skip this test suite if we aren't defined to use the system allocator.
// We rely on some internals of sys_alloc to check that things have been freed.

SUITE(alloc, LIST_ALLOC_TESTS)

#endif
