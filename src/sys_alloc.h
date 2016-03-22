/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "internal_alloc.h"

#define addrhash_1 ahash1
#define addrhash_2 ahash2
#define addr_eq(x,y) ((x)==(y))
YU_HASHTABLE(sysmem_tbl, void *, size_t, addrhash_1, addrhash_2, addr_eq)
#undef addrhash_1
#undef addrhash_2
#undef addr_eq

typedef struct {
  struct yu_mem_funcs base;
  sysmem_tbl allocd;
  internal_allocator tbl_mctx;
} sys_allocator;

yu_err sys_alloc_ctx_init(sys_allocator *ctx);
void sys_alloc_ctx_free(sys_allocator *ctx);

yu_err sys_alloc(sys_allocator *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
yu_err sys_realloc(sys_allocator *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
void sys_free(sys_allocator *ctx, void *ptr);
