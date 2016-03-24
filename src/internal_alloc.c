/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "internal_alloc.h"

yu_err internal_alloc_ctx_init(internal_allocator *ctx) {
  memset(ctx, 0, sizeof(internal_allocator));
  ctx->alloc = (yu_alloc_fn)internal_alloc;
  ctx->realloc = (yu_realloc_fn)internal_realloc;
  ctx->free = (yu_free_fn)internal_free;
  ctx->free_ctx = (yu_ctx_free_fn)internal_alloc_ctx_free;
  return YU_OK;
}

void internal_alloc_ctx_free(internal_allocator * YU_UNUSED(ctx)) {
    // Nothing to do here
}

yu_err internal_alloc(internal_allocator * YU_UNUSED(ctx), void **out, size_t num,
                        size_t elem_size, size_t YU_UNUSED(alignment)) {
    if ((*out = calloc(num, elem_size)) == NULL)
        return YU_ERR_ALLOC_FAIL;
    return YU_OK;
}

yu_err internal_realloc(internal_allocator * YU_UNUSED(ctx), void **ptr, size_t num,
                        size_t elem_size, size_t YU_UNUSED(alignment)) {
    void *safe = *ptr;
    if ((*ptr = realloc(safe, num * elem_size)) == NULL) {
        *ptr = safe;
        return YU_ERR_ALLOC_FAIL;
    }
    return YU_OK;
}

void internal_free(internal_allocator * YU_UNUSED(ctx), void *ptr) {
    free(ptr);
}
