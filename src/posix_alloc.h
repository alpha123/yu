/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

/**
 * Abstract allocator using the POSIX standard functions mmap(2), munmap(2),
 * and madvise(2).
 *
 * This allocator is meant to be a base for other allocators to build on. It
 * only provides functions to reserve, commit, and release pages of memory.
 * It cannot be used as an allocator by itself.
 *
 * Additionally, this allocator does not track pages it has reserved or committed.
 * (That would require that the allocator does some allocations itself to maintain
 * a linked list, which would not be suitable for a building-block allocator.)
 */

typedef yu_mem_funcs posix_allocator;

yu_err posix_alloc_ctx_init(posix_allocator *ctx);

yu_err posix_reserve(posix_allocator *ctx, void **out, size_t num, size_t elem_size);
yu_err posix_release(posix_allocator *ctx, void *ptr);

yu_err posix_commit(posix_allocator *ctx, void *addr, size_t num, size_t elem_size);
yu_err posix_decommit(posix_allocator *ctx, void *addr, size_t num, size_t elem_size);
