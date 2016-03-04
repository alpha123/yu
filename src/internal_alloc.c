/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "internal_alloc.h"

yu_err internal_alloc_ctx_init(yu_memctx_t *ctx) {
    yu_default_alloc_ctx_init(ctx);
    ctx->alloc = internal_alloc;
    ctx->realloc = internal_realloc;
    ctx->free = internal_free;
    ctx->free_ctx = internal_alloc_ctx_free;
    return YU_OK;
}

void internal_alloc_ctx_free(yu_memctx_t * YU_UNUSED(ctx)) {
    // Nothing to do here
}

yu_err internal_alloc(yu_memctx_t * YU_UNUSED(ctx), void **out, size_t num,
                        size_t elem_size, size_t YU_UNUSED(alignment)) {
    if ((*out = calloc(num, elem_size)) == NULL)
        return YU_ERR_ALLOC_FAIL;
    return YU_OK;
}

yu_err internal_realloc(yu_memctx_t * YU_UNUSED(ctx), void **ptr, size_t num,
                        size_t elem_size, size_t YU_UNUSED(alignment)) {
    void *safe = *ptr;
    if ((*ptr = realloc(safe, num * elem_size)) == NULL) {
        *ptr = safe;
        return YU_ERR_ALLOC_FAIL;
    }
    return YU_OK;
}

void internal_free(yu_memctx_t * YU_UNUSED(ctx), void *ptr) {
    free(ptr);
}
