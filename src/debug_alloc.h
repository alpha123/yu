/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

yu_err debug_alloc_ctx_init(yu_memctx_t *ctx);
void debug_alloc_ctx_free(yu_memctx_t *ctx);

yu_err debug_alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
yu_err debug_realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
void debug_free(yu_memctx_t *ctx, void *ptr);
