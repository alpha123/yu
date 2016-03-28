/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "nedmalloc/nedmalloc.h"

typedef struct {
  struct yu_mem_funcs base;
  nedpool *pool;
} ned_allocator;

yu_err ned_alloc_ctx_init(ned_allocator *ctx, size_t init_capacity);
void ned_alloc_ctx_free(ned_allocator *ctx);

yu_err ned_alloc(ned_allocator *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
yu_err ned_realloc(ned_allocator *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
void ned_free(ned_allocator *ctx, void *ptr);

size_t ned_allocated_size(ned_allocator *ctx, void *ptr);
size_t ned_usable_size(ned_allocator *ctx, void *ptr);

yu_err ned_reserve(ned_allocator *ctx, void **out, size_t num, size_t elem_size);
yu_err ned_commit(ned_allocator *ctx, void *ptr, size_t num, size_t elem_size);
yu_err ned_release(ned_allocator *ctx, void *ptr);
yu_err ned_decommit(ned_allocator *ctx, void *ptr, size_t num, size_t elem_size);
