/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "math_ops.h"


bool value_is_numeric(value_t val) {
  value_type w = value_what(val);
  return w == VALUE_FIXNUM || w == VALUE_INT || w == VALUE_DOUBLE || w == VALUE_REAL;
}

value_t value_add(struct gc_info *gc, value_t a, value_t b) {
  value_t out;
  if (value_is_int(a) && value_is_int(b)) {
    int c;
    if (yu_checked_add_s32(value_to_int(a), value_to_int(b), &c)) {
      out = value_from_ptr(gc_alloc_val(gc, VALUE_INT));
      mpz_t *x = value_get_ptr(out)->v.i = yu_xalloc(gc->mem_ctx, 1, sizeof(mpz_t));
      mpz_init_set_si(*x, value_to_int(a));
      // Apparently there's no mpz_add_si
      if (value_to_int(b) < 0)
        mpz_sub_ui(*x, *x, (u32)abs(value_to_int(b)));
      else
        mpz_add_ui(*x, *x, (u32)value_to_int(b));
    }
    else
      out = value_from_int(c);
  }
  return out;
}

value_t value_sub(struct gc_info *gc, value_t a, value_t b) {
  return value_empty();
}

value_t value_mul(struct gc_info *gc, value_t a, value_t b) {
  return value_empty();
}

value_t value_div(struct gc_info *gc, value_t a, value_t b) {
  return value_empty();
}
