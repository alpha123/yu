/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

typedef struct {
  struct yu_rand_funcs base;
  sfmt_t mt;
  s32 low, high;
} uniform_distribution;

void uniform_distribution_init(uniform_distribution *rng, u32 seed, s32 lower_bound, s32 upper_bound);

void uniform_distribution_free(uniform_distribution * YU_UNUSED(rng)) { }
s32 uniform_distribution_int(uniform_distribution *rng);

YU_INLINE
s32 uniform_distribution_min(uniform_distribution *rng) {
  return rng->low;
}

YU_INLINE
s32 uniform_distribution_max(uniform_distribution *rng) {
  return rng->high;
}
