/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "uniform_distribution.h"

void uniform_distribution_init(uniform_distribution *rng, u32 seed, s32 lower_bound, s32 upper_bound) {
  assert(lower_bound < upper_bound);

  rng->base.rand_int = (yu_rand_int_fn)uniform_distribution_int;
  rng->base.rand_free = (yu_rand_free_fn)uniform_distribution_free;

  sfmt_init_gen_rand(&rng->mt, seed);
  rng->low = lower_bound;
  rng->high = upper_bound;
}

s32 uniform_distribution_int(uniform_distribution *rng) {
  s32 r = sfmt_genrand_uint32(&rng->mt) - UINT32_MAX / 2;
  return r % (rng->high - rng->low + 1) + rng->low;
}
