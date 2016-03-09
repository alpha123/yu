/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#define INT_CMP(a,b) ((a)-(b))

YU_QUICKHEAP(minh, int, INT_CMP, YU_QUICKHEAP_MINHEAP)
YU_QUICKHEAP_IMPL(minh, int, INT_CMP, YU_QUICKHEAP_MINHEAP)

YU_QUICKHEAP(maxh, int, INT_CMP, YU_QUICKHEAP_MAXHEAP)
YU_QUICKHEAP_IMPL(maxh, int, INT_CMP, YU_QUICKHEAP_MAXHEAP)

#define SETUP \
    yu_memctx_t mctx; \
    sfmt_t rng; \
    TEST_GET_INTERNAL_ALLOCATOR(&mctx); \
    sfmt_init_gen_rand(&rng, 135135);

#define TEARDOWN \
    yu_alloc_ctx_free(&mctx);

#define LIST_QUICKHEAP_TESTS(X) \
    X(minheap, "Minheaps should order elements smallest to largest") \
    X(maxheap, "Maxheaps should order elements largest to smallest") \
    X(top_empty, "Accessing the top of an empty heap should return the default value")

TEST(minheap)
    minh h;
    minh_init(&h, 20, &mctx);

    int min = INT_MAX;
    for (int i = 0; i < 50; i++) {
	int n = (int)(sfmt_genrand_uint32(&rng) % 500);
	if (n < min)
	    min = n;
	minh_push(&h, n);
    }
    PT_ASSERT_EQ(h.size, 50u);

    // `min` will always be positive (we're only generating positive ints),
    // so -1 as a default value is fine.
    PT_ASSERT_EQ(minh_top(&h, -1), min);

    int last = -1;
    for (int i = 0; i < 50; i++) {
	int n = minh_pop(&h, INT_MIN);
	PT_ASSERT_GTE(n, last);
	last = n;
    }
    PT_ASSERT_EQ(h.size, 0u);

#ifdef TEST_FAST
    for (int i = 0; i < 100; i++)
#else
    for (int i = 0; i < (int)2e6; i++)
#endif
    {
	minh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
    }

    bool all_gte = true;
    last = -10001;
#ifdef TEST_FAST
    for (int i = 0; i < 30; i++)
#else
    for (int i = 0; i < 5e4; i++)
#endif
    {
	int n = minh_pop(&h, INT_MIN);
	if (n < last) {
	    all_gte = false;
	    break;
	}
	last = n;
    }
    PT_ASSERT(all_gte);

#ifdef TEST_FAST
    for (int i = 0; i < 500; i++)
#else
    for (int i = 0; i < (int)3e5; i++)
#endif
    {
	minh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
    }

    all_gte = true;
    last = -10001;
#ifdef TEST_FAST
    for (int i = 0; i < 200; i++)
#else
    for (int i = 0; i < 3e3; i++)
#endif
    {
	int n = minh_pop(&h, INT_MIN);
	if (n < last) {
	    all_gte = false;
	    break;
	}
	last = n;
    }
    PT_ASSERT(all_gte);

#ifdef TEST_FAST
    for (int i = 0; i < 400; i++)
#else
    for (int i = 0; i < (int)2e4; i++)
#endif
    {
	minh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
    }

    all_gte = true;
    last = -10001;
    while (h.size) {
	int n = minh_pop(&h, INT_MIN);
	if (n < last) {
	    all_gte = false;
	    break;
	}
	last = n;
    }
    PT_ASSERT(all_gte);

    minh_free(&h);
END(minheap)

TEST(maxheap)
    maxh h;
    maxh_init(&h, 20, &mctx);

    int max = INT_MIN;
    for (int i = 0; i < 50; i++) {
	int n = (int)(sfmt_genrand_uint32(&rng) % 500);
	if (n > max)
	    max = n;
	maxh_push(&h, n);
    }
    PT_ASSERT_EQ(h.size, 50u);

    PT_ASSERT_EQ(maxh_top(&h, -1), max);

    int last = INT_MAX;
    for (int i = 0; i < 50; i++) {
	int n = maxh_pop(&h, INT_MAX);
	PT_ASSERT_LTE(n, last);
	last = n;
    }
    PT_ASSERT_EQ(h.size, 0u);

#ifdef TEST_FAST
    for (int i = 0; i < 100; i++)
#else
    for (int i = 0; i < (int)2e6; i++)
#endif
    {
	maxh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
    }

    bool all_lte = true;
    last = 10001;
#ifdef TEST_FAST
    for (int i = 0; i < 30; i++)
#else
    for (int i = 0; i < 5e4; i++)
#endif
    {
	int n = maxh_pop(&h, INT_MAX);
	if (n > last) {
	    all_lte = false;
	    break;
	}
	last = n;
    }
    PT_ASSERT(all_lte);

#ifdef TEST_FAST
    for (int i = 0; i < 500; i++)
#else
    for (int i = 0; i < (int)3e5; i++)
#endif
    {
	maxh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
    }

    all_lte = true;
    last = 10001;
#ifdef TEST_FAST
    for (int i = 0; i < 200; i++)
#else
    for (int i = 0; i < 3e3; i++)
#endif
    {
	int n = maxh_pop(&h, INT_MAX);
	if (n > last) {
	    all_lte = false;
	    break;
	}
	last = n;
    }
    PT_ASSERT(all_lte);

#ifdef TEST_FAST
    for (int i = 0; i < 400; i++)
#else
    for (int i = 0; i < (int)2e4; i++)
#endif
    {
	maxh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
    }

    all_lte = true;
    last = 10001;
    while (h.size) {
	int n = maxh_pop(&h, INT_MAX);
	if (n > last) {
	    all_lte = false;
	    break;
	}
	last = n;
    }
    PT_ASSERT(all_lte);

    maxh_free(&h);
END(maxheap)

TEST(top_empty)
    minh h;
    minh_init(&h, 7, &mctx);
    PT_ASSERT_EQ(minh_pop(&h, 0), 0);
    PT_ASSERT_EQ(minh_top(&h, -10), -10);
    minh_free(&h);
END(top_empty)


SUITE(quickheap, LIST_QUICKHEAP_TESTS)
