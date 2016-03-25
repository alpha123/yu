/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "rope.h"

YU_MALLOC_LIKE
yu_rope *rope_new(yu_allocator *mctx, yu_rand *rng) {
  yu_rope *r = yu_xalloc(mctx, 1, sizeof(yu_rope));
  r->mem_ctx = mctx;
  r->rng = rng;
  return r;
}

void rope_free(yu_rope *r) {
  yu_free(r->mem_ctx, r);
}
