/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_checked_math.h directly! Include yu_common.h instead."
#endif

#if (defined(__GNUC__) && __GNUC__ >= 5) || (defined(__clang__) && __has_builtin(__builtin_sadd_overflow))
#define yu_checked_add_s32 __builtin_sadd_overflow
#define yu_checked_sub_s32 __builtin_ssub_overflow
#define yu_checked_mul_s32 __builtin_smul_overflow
#elif defined(_MSC_VER) && _MSC_VER >= 1600  // VS2010+
#include <safeint.h>
// Technically this is C++ code; fortunately MSVC always compiles as C++
// See https://msdn.microsoft.com/en-us/library/dd570023.aspx
extern "C" {
  inline bool yu_checked_add_s32(s32 a, s32 b, s32 *out) {
    s32 c;
    bool ok = SafeAdd(a, b, c);
    *out = c;
    return ok;
  }
  inline bool yu_checked_sub_s32(s32 a, s32 b, s32 *out) {
    s32 c;
    bool ok = SafeSubtract(a, b, c);
    *out = c;
    return ok;
  }
  inline bool yu_checked_mul_s32(s32 a, s32 b, s32 *out) {
    s32 c;
    bool ok = SafeMultiply(a, b, c);
    *out = c;
    return ok;
  }
}
#else
#warning "Compiler doesn't support checked arithmetic, using slow math; try GCC5+ or Clang 3.4+"
inline
bool yu_checked_add_s32(s32 a, s32 b, s32 *out) {
  if (b > 0 && a > INT32_MAX - b) return true;
  if (b < 0 && a < INT32_MIN - b) return true;
  *out = a + b;
  return false;
}
inline
bool yu_checked_sub_s32(s32 a, s32 b, s32 *out) {
  if (b > 0 && a > INT32_MAX - b) return true;
  if (b < 0 && a < INT32_MIN - b) return true;
  *out = a - b;
  return false;
}
inline
bool yu_checked_mul_s32(s32 a, s32 b, s32 *out) {
  if (b > 0 && a > INT32_MAX / b) return true;
  if (b < 0 && a < INT32_MIN / b) return true;
  *out = a * b;
  return false;
}
#endif

// MSVC support is limited to VS2013+ for fenv.h
#include <fenv.h>

static inline
bool yu_checked_add_d(double a, double b, double *out) {
  // Docs aren't clear on exactly when this function can fail, so assume we
  // cannot safely test exceptions and just fall back to software/mpfr
  // floating-point.
  if (feclearexcept(FE_OVERFLOW) != 0) return true;
  *out = a + b;
  return fetestexcept(FE_OVERFLOW) != 0;
}

static inline
bool yu_checked_sub_d(double a, double b, double *out) {
  // Note that subtraction usually raises FE_INEXACT and does not cause an
  // overflow trap. Actually, I'm not sure FE_OVERFLOW is ever raised; for
  // example DBL_MIN - 1 is -1 and FE_INEXACT.
  if (feclearexcept(FE_OVERFLOW|FE_INEXACT) != 0) return true;
  *out = a - b;
  return fetestexcept(FE_OVERFLOW|FE_INEXACT) != 0;
}

static inline
bool yu_checked_mul_d(double a, double b, double *out) {
  if (feclearexcept(FE_OVERFLOW|FE_UNDERFLOW) != 0) return true;
  *out = a * b;
  return fetestexcept(FE_OVERFLOW|FE_UNDERFLOW) != 0;
}

// This function merely checks for underflow: it is ‘checked’ division, not
// ‘safe’ division—it is the caller's responsibility to ensure that `b ≠ 0`.
static inline
bool yu_checked_div_d(double a, double b, double *out) {
  if (feclearexcept(FE_UNDERFLOW) != 0) return true;
  *out = a / b;
  return fetestexcept(FE_UNDERFLOW) != 0;
}
