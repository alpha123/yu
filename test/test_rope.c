/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "rope.h"

#define SETUP \
  TEST_GET_ALLOCATOR(mctx); \
  sfmt_t rng; \
  sfmt_init_gen_rand(&rng, 126155); \
  rope *r = rope_new((yu_allocator *)&mctx, &rng);

#define TEARDOWN \
  rope_free(r); \
  yu_alloc_ctx_free(&mctx);

#define LIST_ROPE_TESTS(X) \
  X(create_str, "Initializing a rope with a string should create a rope with the same contents") \
  X(unicode_str, "Code point counts should accurately be reported for UTF-8 strings") \


TEST(create_str)
  const char s[] = "Do you have any idea how stupid we are? Don't underestimate us!";
  rope *r2 = rope_new_with_utf8((yu_allocator *)&mctx, &rng, (const u8 *)s);
  PT_ASSERT_EQ(rope_byte_count(r2), strlen(s));
  PT_ASSERT_EQ(rope_char_count(r2), strlen(s));
  char out_s[sizeof s];
  PT_ASSERT_EQ(rope_write_cstr(r2, (u8 *)out_s), strlen(s) + 1);
  PT_ASSERT_STR_EQ(out_s, s);
  rope_free(r2);
END(create_str)

TEST(unicode_str)
  // File must be encoded as UTF-8 for this test to work.
  const char s[] = "年年有余";
  rope *r2 = rope_new_with_utf8((yu_allocator *)&mctx, &rng, (const u8 *)s);
  PT_ASSERT_EQ(rope_byte_count(r2), strlen(s));
  PT_ASSERT_EQ(rope_char_count(r2), 4u);
END(unicode_str)

SUITE(rope, LIST_ROPE_TESTS)
