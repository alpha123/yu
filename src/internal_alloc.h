/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

/**
 * Note: this *DOES NOT* obey the standard allocator contract.
 * Other allocators need various data structures to track their
 * information, but the data structures need allocators too!
 * This allocator is just a calloc/realloc/free wrapper, for
 * internal use by real allocators.
 *
 * Most importantly, internal_alloc_ctx_free does *not* free
 * all objects allocated in the context with alloc_ctx_free.
 * This is fine since all data structures free their stuff
 * individually.
 */

yu_err internal_alloc_ctx_init(yu_memctx_t *ctx);
void internal_alloc_ctx_free(yu_memctx_t * YU_UNUSED(ctx));

yu_err internal_alloc(yu_memctx_t * YU_UNUSED(ctx), void **out, size_t num, size_t elem_size, size_t alignment);
yu_err internal_realloc(yu_memctx_t * YU_UNUSED(ctx), void **ptr, size_t num, size_t elem_size, size_t alignment);
void internal_free(yu_memctx_t * YU_UNUSED(ctx), void *ptr);
