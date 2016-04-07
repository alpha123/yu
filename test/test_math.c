/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"
#include "gc.h"
#include "math_ops.h"

#define SETUP                                               \
  TEST_GET_ALLOCATOR(mctx);                                 \
  struct gc_info gc;                                        \
  yu_err _gciniterr = gc_init(&gc, (yu_allocator *)&mctx);  \
  assert(_gciniterr == YU_OK);                              \

#define TEARDOWN                                \
  gc_free(&gc);                                 \
  yu_alloc_ctx_free(&mctx);

#define LIST_MATH_TESTS(X) \
  X(add_fix, "Adding two fixnums that do not overflow should result in a fixnum") \
  X(add_fix_overflow, "Adding two fixnums that overflow should produce an int")

TEST(add_fix)
  value_t x = value_from_int(42), y = value_from_int(5),
    z = value_add(&gc, x, y);
  PT_ASSERT_EQ(value_to_int(z), 47);
END(add_fix)

TEST(add_fix_overflow)
  value_t x = value_from_int(UINT32_MAX), y = value_from_int(7),
    z = value_add(&gc, x, y);
  PT_ASSERT(value_is_ptr(z));
END(add_fix_overflow)


SUITE(math, LIST_MATH_TESTS)
