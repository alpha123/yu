/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#define SETUP \
    yu_memctx_t ctx; \
    yu_alloc_ctx_init(&ctx);

#define TEARDOWN \
    yu_alloc_ctx_free(&ctx);

#define LIST_ALLOC_TESTS(X) \
    X(alloc, "alloc() should allocate usable space of the requested size") \
    X(alloc_zero, "alloc() should initialize the requested space to 0") \
    X(alloc_free, "free() should free the allocated space")

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

    int *ns = yu_xalloc(&ctx, 50, sizeof(int));
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


SUITE(alloc, LIST_ALLOC_TESTS)
