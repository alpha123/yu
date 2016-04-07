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
  X(checked_add, "Checked addition should signal that it has overflowed") \
  X(checked_sub, "Checked subtraction should signal that it has overflowed") \
  X(checked_mul, "Checked multiplication should signal that it has overflowed") \
  X(checked_fadd, "Checked floating-point addition should signal that it has overflowed") \
  X(checked_fsub, "Checked floating-point subtraction should signal that it has overflowed") \
  X(checked_fmul, "Checked floating-point multiplication should signal that it has overflowed or underflowed") \
  X(checked_fdiv, "Checked floating-point division should signal that it has underflowed") \
  X(add_fix, "Adding two fixnums that do not overflow should result in a fixnum") \
  X(add_fix_overflow, "Adding two fixnums that overflow should produce an int")

TEST(checked_add)
  s32 x = 20, y = 10, z;
  PT_ASSERT(!yu_checked_add_s32(x, y, &z));
  PT_ASSERT_EQ(z, 30);
  x = INT32_MAX;
  PT_ASSERT(yu_checked_add_s32(x, y, &z));
END(checked_add)

TEST(checked_sub)
  s32 x = -20, y = 10, z;
  PT_ASSERT(!yu_checked_sub_s32(x, y, &z));
  PT_ASSERT_EQ(z, -30);
  y = INT32_MAX;
  PT_ASSERT(yu_checked_sub_s32(x, y, &z));
END(checked_add)

TEST(checked_mul)
  s32 x = 20, y = 10, z;
  PT_ASSERT(!yu_checked_mul_s32(x, y, &z));
  PT_ASSERT_EQ(z, 200);
  x = INT32_MAX;
  PT_ASSERT(yu_checked_mul_s32(x, y, &z));
END(checked_mul)

TEST(checked_fadd)
  double x = 2.5, y = 2, z;
  PT_ASSERT(!yu_checked_add_d(x, y, &z));
  PT_ASSERT_LTE(fabs(z - 4.5), 0.00001);
  x = y = DBL_MAX;
  PT_ASSERT(yu_checked_add_d(x, y, &z));
END(checked_fadd)

TEST(checked_fsub)
  double x = 2.5, y = 2, z;
  PT_ASSERT(!yu_checked_sub_d(x, y, &z));
  PT_ASSERT_LTE(fabs(z - 0.5), 0.00001);
  x = DBL_MIN;
  y = 1;
  PT_ASSERT(yu_checked_sub_d(x, y, &z));
END(checked_fsub)

TEST(checked_fmul)
  double x = 2.5, y = 2, z;
  PT_ASSERT(!yu_checked_mul_d(x, y, &z));
  PT_ASSERT_LTE(fabs(z - 5.0), 0.00001);
  x = y = DBL_MAX;
  PT_ASSERT(yu_checked_mul_d(x, y, &z));

  // Check underflow
  x = DBL_MIN;
  y = 1e-98;
  PT_ASSERT(yu_checked_mul_d(x, y, &z));
END(checked_fmul)

TEST(checked_fdiv)
  double x = 5.5, y = 3, z;
  PT_ASSERT(!yu_checked_div_d(x, y, &z));
  PT_ASSERT_LTE(fabs(z - 1.8333333333), 0.00001);
  x = DBL_MIN;
  PT_ASSERT(yu_checked_div_d(x, y, &z));
END(checked_fdiv)

TEST(add_fix)
  value_t x = value_from_int(42), y = value_from_int(5),
    z = value_add(&gc, x, y);
  PT_ASSERT_EQ(value_to_int(z), 47);
END(add_fix)

TEST(add_fix_overflow)
  value_t x = value_from_int(INT32_MAX), y = value_from_int(7),
    z = value_add(&gc, x, y);
  PT_ASSERT(value_is_ptr(z));
  mpz_t *i = value_get_ptr(z)->v.i;
  PT_ASSERT_LTE(fabs(mpz_get_d(*i) - (INT32_MAX + 7.0)), 0.00001);
END(add_fix_overflow)


SUITE(math, LIST_MATH_TESTS)
