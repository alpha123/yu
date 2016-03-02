/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_alloc.h directly! Include yu_common.h instead."
#endif

/**
 * Generic allocator API.
 *
 * Allocators _need not_ be thread-safe. Allocation functions will _always_
 * be called from a single thread.
 *
 * All allocations have a context, which is the first argument to most functions.
 * Freeing a context _must_ free all allocations made in that context.
 * Allocators _must_ define a yu_memctx_t type and the following functions to
 * manage contexts:
 *
 *   • yu_err alloc_ctx_init(yu_memctx_t *ctx);
 *   • void alloc_ctx_free(yu_memctx_t *ctx);
 *
 * Allocators _must_ provide the following functions, roughly approximating
 * their ISO C equivalents in behavior:
 *
 *   • yu_err alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
 *     alloc _may_ accept a non-power-of-2 alignment. if `alignment` is 0,
 *     alloc _must_ use a suitable default alignment (usually 8 or 16). In
 *     all other ways this function should behave like ISO C calloc().
 *   • void free(yu_memctx_t *ctx, void *ptr);
 *     This function should behave like ISO C free().
 *
 * Allocators _may_ provide the following functions. If they do not, suitable
 * default implementations will be used:
 *
 *   • yu_err realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
 *     If `*ptr` is NULL, realloc _must_ behave identically to `alloc`. If the
 *     system cannot allocate enough memory to satisfy the request, `ptr`
 *     _must not_ be changed.
 *   • yu_err array_alloc(yu_memctx_t *ctx, void **out, size_t elem_count, size_t elem_size, size_t alignment);
 *   • yu_err array_realloc(yu_memctx_t *ctx, void **ptr, size_t elem_count, size_t elem_size, size_t alignment);
 *   • void array_free(yu_memctx_t *ctx, void *ptr);
 *   • size_t array_len(yu_memctx_t *ctx, void *ptr);
 *
 *
 * An "allocator" should be a macro that accepts a function name and varargs. 
 * ALLOCATOR(alloc, ...) _must_ expand to alloc(...)
 * ALLOCATOR(free, ...) _must_ expand to free(...)
 * and so on.
 */

#include "sys_alloc.h"
#define yu_memctx_t sys_memctx_t
#define YU_DEFAULT_ALLOCATOR SYSTEM_ALLOCATOR

#define yu_alloc_ctx_init(...) YU_DEFAULT_ALLOCATOR(alloc_ctx_init, __VA_ARGS__)
#define yu_alloc_ctx_free(...) YU_DEFAULT_ALLOCATOR(alloc_ctx_free, __VA_ARGS__)
#define yu_alloc(...) YU_DEFAULT_ALLOCATOR(alloc, __VA_ARGS__)
#define yu_realloc(...) YU_DEFAULT_ALLOCATOR(realloc, __VA_ARGS__)
#define yu_free(...) YU_DEFAULT_ALLOCATOR(free, __VA_ARGS__)
#define yu_array_alloc(...) YU_DEFAULT_ALLOCATOR(array_alloc, __VA_ARGS__)
#define yu_array_realloc(...) YU_DEFAULT_ALLOCATOR(array_realloc, __VA_ARGS__)
#define yu_array_free(...) YU_DEFAULT_ALLOCATOR(array_free, __VA_ARGS__)

#define YU_DEFAULT_ARRAY_ALLOC_IMPL(fn_name, alloc) \
    yu_err fn_name(yu_memctx_t *ctx, void **out, size_t elem_count, size_t elem_size, size_t alignment) { \
	YU_ERR_DEFVAR \
	void *base, *ptr; \
	YU_CHECK(alloc(ctx, &base, elem_count*elem_size + sizeof(size_t) + sizeof(void *) + alignment-1, 1, 16)); \
	ptr = (void *)(((uintptr_t)base + (alignment-1)) & ~(uintptr_t)(alignment-1)); \
	*((void **)ptr) = base; \
	*((size_t *)((void **)ptr+1)) = elem_count; \
	*out = (void *)((size_t *)((void **)ptr+1)+1); \
	return YU_OK; \
	yu_err_handler: \
	*out = NULL; \
	return yu_local_err; \
    }

#define YU_DEFAULT_ARRAY_LEN_IMPL(fn_name) \
    YU_INLINE \
    size_t fn_name(void *ptr) { \
	return *((size_t *)ptr-1); \
    }

#define YU_DEFAULT_ARRAY_FREE_IMPL(fn_name, free) \
    void fn_name(yu_memctx_t *ctx, void *ptr) { \
	free(ctx, (void *)((void **)((size_t *)ptr-1)-1)); \
    }

#define YU_DEFAULT_ARRAY_REALLOC_IMPL(fn_name, realloc, array_alloc, array_free) \
    yu_err fn_name(yu_memctx_t *ctx, void **ptr, size_t elem_count, size_t elem_size, size_t alignment) { \
	YU_ERR_DEFVAR \
	if (*ptr == NULL) return array_alloc(ctx, ptr, elem_count, elem_size, alignment); \
	if (elem_count == 0) { \
	    array_free(ctx, *ptr); \
	    return YU_OK; \
	} \
	void *base = (void *)((void **)((size_t *)ptr-1)-1), *p; \
	YU_CHECK(realloc(ctx, &base, elem_count*elem_size + sizeof(size_t) + sizeof(void *) + alignment-1, 1, 16)); \
	p = (void *)(((uintptr_t)base + (alignment-1)) & ~(uintptr_t)(alignment-1)); \
	*((void **)p) = base; \
	*((size_t *)((void **)p+1)) = elem_count; \
	*ptr = (void *)((size_t *)((void **)p+1)+1); \
	return YU_OK; \
	yu_err_handler: \
	return yu_local_err; \
    }

YU_INLINE
void *yu_xalloc(yu_memctx_t *ctx, size_t num, size_t elem_size) {
    void *ptr;
    yu_err err;
    if ((err = yu_alloc(ctx, &ptr, num, elem_size, 0)) != YU_OK) {
	yu_global_fatal_handler(err);
	return NULL;
    }
    return ptr;
}
