/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

#define addrhash_1 ahash1
#define addrhash_2 ahash2
#define addr_eq(x,y) ((x)==(y))
YU_HASHTABLE(sysmem_tbl, void *, size_t, addrhash_1, addrhash_2, addr_eq)
#undef addrhash_1
#undef addrhash_2
#undef addr_eq

yu_err sys_alloc_ctx_init(yu_memctx_t *ctx);
void sys_alloc_ctx_free(yu_memctx_t *ctx);

yu_err sys_alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
yu_err sys_realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
void sys_free(yu_memctx_t *ctx, void *ptr);
