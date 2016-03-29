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
    sfmt_t rng; \
    TEST_GET_INTERNAL_ALLOCATOR(mctx); \
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
  int valcnt = 100;
#elif TEST_STRESS
  int valcnt = (int)5e7;
#else
  int valcnt = (int)2e6;
#endif
    for (int i = 0; i < valcnt; i++) {
      minh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
    }

    bool all_gte = true;
    last = -10001;

#ifdef TEST_FAST
  int popcnt = 30;
#elif TEST_STRESS
  int popcnt = (int)6e5;
#else
  int popcnt = (int)5e4;
#endif
  for (int i = 0; i < popcnt; i++) {
    int n = minh_pop(&h, INT_MIN);
    if (n < last) {
        all_gte = false;
        break;
    }
    last = n;
  }
  PT_ASSERT(all_gte);

#ifdef TEST_FAST
  int pushcnt = 500;
#elif TEST_STRESS
  int pushcnt = (int)9e5;
#else
  int pushcnt = (int)3e5;
#endif
  for (int i = 0; i < pushcnt; i++) {
    minh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
  }

  all_gte = true;
  last = -10001;
#ifdef TEST_FAST
  popcnt = 200;
#elif TEST_STRESS
  popcnt = (int)4e5;
#else
  popcnt = (int)3e3;
#endif
  for (int i = 0; i < popcnt; i++) {
    int n = minh_pop(&h, INT_MIN);
    if (n < last) {
        all_gte = false;
        break;
    }
    last = n;
  }
  PT_ASSERT(all_gte);

#ifdef TEST_FAST
  pushcnt = 400;
#elif TEST_STRESS
  pushcnt = (int)5e5;
#else
  pushcnt = (int)2e4;
#endif
  for (int i = 0; i < 400; i++) {
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
  int valcnt = 100;
#elif TEST_STRESS
  int valcnt = (int)5e7;
#else
  int valcnt = (int)2e6;
#endif
  for (int i = 0; i < valcnt; i++) {
    maxh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
  }


#ifdef TEST_FAST
  int popcnt = 30;
#elif TEST_STRESS
  int popcnt = (int)6e5;
#else
  int popcnt = (int)5e4;
#endif
  bool all_lte = true;
  last = 10001;
  for (int i = 0; i < popcnt; i++) {
    int n = maxh_pop(&h, INT_MIN);
    if (n > last) {
        all_lte = false;
        break;
    }
    last = n;
  }
  PT_ASSERT(all_lte);

#ifdef TEST_FAST
  int pushcnt = 500;
#elif TEST_STRESS
  int pushcnt = (int)9e5;
#else
  int pushcnt = (int)3e5;
#endif
  for (int i = 0; i < pushcnt; i++) {
    maxh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
  }

  all_lte = true;
  last = 10001;
#ifdef TEST_FAST
  popcnt = 200;
#elif TEST_STRESS
  popcnt = (int)4e5;
#else
  popcnt = (int)3e3;
#endif
  for (int i = 0; i < popcnt; i++) {
    int n = maxh_pop(&h, INT_MIN);
    if (n > last) {
        all_lte = false;
        break;
    }
    last = n;
  }
  PT_ASSERT(all_lte);

#ifdef TEST_FAST
  pushcnt = 400;
#elif TEST_STRESS
  pushcnt = (int)5e5;
#else
  pushcnt = (int)2e4;
#endif
  for (int i = 0; i < 400; i++) {
    maxh_push(&h, (int)(sfmt_genrand_uint32(&rng) % 20000 - 10000));
  }

  all_lte = true;
  last = 10001;
  while (h.size) {
    int n = maxh_pop(&h, INT_MIN);
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
