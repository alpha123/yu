/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "rope.h"

rope *rope_new(yu_allocator *mctx) {
  rope *r = yu_xalloc(1, sizeof(rope));
  r->mem_ctx = mctx;
  sfmt_init_genrand(&r->rng, yu_sys_random());
  return r;
}

void rope_free(rope *r) {
  yu_free(r->mem_ctx, r);
}
