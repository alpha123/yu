/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

/**
 * Fast, short-lived bump allocator.
 * This allocator is _extremely_ fast, but should _not_ be used
 * for long-lived contexts. It only frees upon context destroy.
 */

// If you go over `size`, you're out of memory. That's all.
yu_err bump_alloc_ctx_init(yu_memctx_t *ctx, size_t size);
void bump_alloc_ctx_free(yu_memctx_t *ctx);

yu_err bump_alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
yu_err bump_realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
void bump_free(yu_memctx_t *ctx, void *ptr);

size_t bump_alloc_size(void *ptr);
