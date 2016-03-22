/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_alloc.h directly! Include yu_common.h instead."
#endif

/**
 * API DESIGN
 * ----------
 *
 * Next-generation memory allocation API. This API is designed to reduce allocations
 * and take advantage of the features of modern NT and *nix systems. Allocator APIs
 * from 1979 do not provide maximum efficiency on 64-bit virtual memory systems.
 *
 * Design principles:
 *   • Embrace modern memory systems with 64-bit address space and virtual memory
 *   • Provide explicit control over many aspects of allocation such as separation
 *     of reserving/committing, page-level control, and alignment.
 *   • Information about allocation size should not be considered an implementation
 *     detail. Many allocators round allocations up a bit, and this information
 *     should be available to the user to reduce allocations.
 *   • Be composable — many allocators should simply be wrappers over lower-level
 *     allocators. A basic allocator may simply implement reserving and committing
 *     pages, while higher level allocators may provide different ways of splitting
 *     those pages up (bump allocators, power-of-2 allocators, etc).
 *   • Thread safety is not something an allocator should worry about. Memory contexts
 *     should not be shared among threads.
 *
 *
 * ALLOCATOR CONTRACT
 * ------------------
 *
 * All allocations have a context, which is the first argument to most functions.
 * Freeing a context _must_ free all allocations made in that context.
 * Allocators _must_ define a struct yu_mem_funcs type and the following functions to
 * manage contexts:
 *
 *   • yu_err alloc_ctx_init(struct yu_mem_funcs *ctx);
 *   • void alloc_ctx_free(struct yu_mem_funcs *ctx);
 *
 * Allocators _must_ provide the following functions:
 *   • yu_err reserve(struct yu_mem_funcs *ctx, void **out, size_t num, size_t elem_size);
 *     Reserve pages of memory from the OS. These pages may be at any location.
 *     This function _must never_ return an error EXCEPT in the following circumstances:
 *       • This process has exhausted its address space (highly unlikely)
 *       • `num` or `elem_size` is 0. This function may assert() that instead, but must
 *         still return an error in a release build.
 *
 *  • yu_err commit(struct yu_mem_funcs *ctx, void *addr, size_t num, size_t elem_size);
 *    Commit pages in the interval [addr,addr+num*elem_size) to physical memory.
 *    If `addr` is not on a page boundary, this function should round it down to the
 *    nearest page boundary and commit that.
 *    This function may return an error in the following circumstances:
 *      • `addr` is not in a reserved page
 *      • `addr` was reserved in a different memory context
 *      • The system is out of physical memory.
 *    It is _not_ an error to commit pages that are already committed.
 *
 *  • yu_err release(struct yu_mem_funcs *ctx, void *ptr);
 *    Release pages reserved with reserve(). The full reservation size must be released.
 *    This function may only return an error in the following circumstances:
 *      • `ptr` is not an address that was returned from reserve()
 *      • `ptr` was reserved in a different memory context
 *
 *  • yu_err decommit(struct yu_mem_funcs *ctx, void *ptr, size_t num, size_t elem_size);
 *    Decommit pages in the interval [addr,addr+num*elem_size). If `addr` is not on
 *    a page boundary, this function should round it down to the nearest page boundary
 *    and decommit that.
 *    This function may return an error in the following circumstances:
 *      • `addr` is not in a reserved page
 *      • `addr` was reserved in a different memory context
 *    It is _not_ an error to decommit a page that is not committed.
 *
 *
 * Most allocators will provide the following functions. Any that do not should be considered
 * an ‘abstract allocator’ and be used as a building block for higher-level allocators.
 *
 *   • yu_err alloc(struct yu_mem_funcs *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
 *     alloc _may_ accept a non-power-of-2 alignment. if `alignment` is 0, alloc _must_
 *     use a suitable default alignment (usually 8 or 16). In all other ways this
 *     function should behave like ISO C calloc().
 *
 *   • yu_err realloc(struct yu_mem_funcs *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
 *     This function will never be passed a NULL `*ptr` or a `num` or `elem_size` of 0.
 *     If the system cannot allocate enough memory to satisfy the request, `ptr`
 *     _must not_ be changed. If `alignment` is 0, realloc _must_ use the same default
 *     alignment as `alloc`. If the actual allocation size grows, the new space _must_
 *     be initialized to zeroes.
 *
 *   • void free(struct yu_mem_funcs *ctx, void *ptr);
 *     This function should behave like ISO C free().
 *
 *   • size_t allocated_size(struct yu_mem_funcs *ctx, void *ptr);
 *     This function _must_ return the requested allocation size, i.e. num*elem_size from
 *     alloc() or realloc().
 *
 *   • size_t usable_size(struct yu_mem_funcs *ctx, void *ptr);
 *     This function _must_ return the *actual* size that the allocator reserved. For
 *     example, if an allocator rounds up all allocations to 2^n, that number should be
 *     returned instead of the size requested from alloc() or realloc().
 *
 * An allocator should provide a function to initialize a struct yu_mem_funcs subclass with its
 * implementations of the above functions. Additionally, it _must_ a free_ctx function
 * to deallocate everything allocated within that context.
 *
 * To subclass yu_mem_funcs, a struct should be declared with `yu_mem_funcs base;` as the
 * _first_ member. (The name does not matter, but it is possible macros in the future could
 * rely on it being ‘base’).
 */

struct yu_mem_funcs;

typedef yu_err (* yu_reserve_fn)(struct yu_mem_funcs *ctx, void **out, size_t num, size_t elem_size);
typedef yu_err (* yu_release_fn)(struct yu_mem_funcs *ctx, void *ptr);

typedef yu_err (* yu_commit_fn)(struct yu_mem_funcs *ctx, void *addr, size_t num, size_t elem_size);
typedef yu_err (* yu_decommit_fn)(struct yu_mem_funcs *ctx, void *addr, size_t num, size_t elem_size);

typedef yu_err (* yu_alloc_fn)(struct yu_mem_funcs *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
typedef yu_err (* yu_realloc_fn)(struct yu_mem_funcs *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
typedef void (* yu_free_fn)(struct yu_mem_funcs *ctx, void *ptr);

typedef size_t (* yu_allocated_size_fn)(struct yu_mem_funcs *ctx, void *ptr);
typedef size_t (* yu_usable_size_fn)(struct yu_mem_funcs *ctx, void *ptr);

typedef void (* yu_free_ctx_fn)(struct yu_mem_funcs *ctx);

typedef struct yu_mem_funcs {
    yu_alloc_fn alloc;
    yu_realloc_fn realloc;
    yu_free_fn free;

    yu_free_ctx_fn free_ctx;
} yu_allocator;

// These all take void *ctx to avoid warnings about imcompatible pointer types —
// they'll always be passed a subclass of yu_allocator.

YU_INLINE
yu_err yu_alloc(void *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
  return ((yu_allocator *)ctx)->alloc(ctx, out, num, elem_size, alignment);
}
YU_INLINE
yu_err yu_realloc(void *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
    assert(ptr != NULL);
    assert(elem_size > 0);
    if (*ptr == NULL)
        return ((yu_allocator *)ctx)->alloc(ctx, ptr, num, elem_size, alignment);
    if (num == 0) {
        ((yu_allocator *)ctx)->free(ctx, *ptr);
        return YU_OK;
    }
    return ((yu_allocator *)ctx)->realloc(ctx, ptr, num, elem_size, alignment);
}
YU_INLINE
void yu_free(void *ctx, void *ptr) { ((yu_allocator *)ctx)->free(ctx, ptr); }

YU_INLINE
void yu_alloc_ctx_free(void *ctx) { ((yu_allocator *)ctx)->free_ctx(ctx); }

void *yu_xalloc(void *ctx, size_t num, size_t elem_size);
void *yu_xrealloc(void *ctx, void *ptr, size_t num, size_t elem_size);
