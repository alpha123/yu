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
 * An allocator should provide a function to initialize a yu_memctx_t struct with its
 * implementations of the above functions, or yu_default_*_impl functions (where applicable).
 * Additionally, it should _must_ a free_ctx function to deallocate everything allocated
 * within that context.
 */

struct yu_memctx;

typedef yu_err (* yu_alloc_fn)(struct yu_memctx *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
typedef yu_err (* yu_realloc_fn)(struct yu_memctx *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
typedef void (* yu_free_fn)(struct yu_memctx *ctx, void *ptr);

typedef yu_err (* yu_array_alloc_fn)(struct yu_memctx *ctx, void **out, size_t elem_count, size_t elem_size, size_t alignment);
typedef yu_err (* yu_array_realloc_fn)(struct yu_memctx *ctx, void **ptr, size_t elem_count, size_t elem_size, size_t alignment);
typedef void (* yu_array_free_fn)(struct yu_memctx *ctx, void *ptr);
typedef size_t (* yu_array_len_fn)(struct yu_memctx *ctx, void *ptr);

typedef void (* yu_free_ctx_fn)(struct yu_memctx *ctx);

typedef struct yu_memctx {
    void *adata;  // pointer to the allocator's custom data structure

    yu_alloc_fn alloc;
    yu_realloc_fn realloc;
    yu_free_fn free;

    yu_array_alloc_fn array_alloc;
    yu_array_realloc_fn array_realloc;
    yu_array_free_fn array_free;
    yu_array_len_fn array_len;

    yu_free_ctx_fn free_ctx;
} yu_memctx_t;

YU_INLINE
yu_err yu_alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
    return ctx->alloc(ctx, out, num, elem_size, alignment);
}
YU_INLINE
yu_err yu_realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
    return ctx->realloc(ctx, ptr, num, elem_size, alignment);
}
YU_INLINE
void yu_free(yu_memctx_t *ctx, void *ptr) { ctx->free(ctx, ptr); }

YU_INLINE
yu_err yu_array_alloc(yu_memctx_t *ctx, void **out, size_t elem_count, size_t elem_size, size_t alignment) {
    return ctx->array_alloc(ctx, out, elem_count, elem_size, alignment);
}
YU_INLINE
yu_err yu_array_realloc(yu_memctx_t *ctx, void **ptr, size_t elem_count, size_t elem_size, size_t alignment) {
    return ctx->array_realloc(ctx, ptr, elem_count, elem_size, alignment);
}
YU_INLINE
void yu_array_free(yu_memctx_t *ctx, void *ptr) { ctx->array_free(ctx, ptr); }
YU_INLINE
size_t yu_array_len(yu_memctx_t *ctx, void *ptr) { return ctx->array_len(ctx, ptr); }

YU_INLINE
void yu_alloc_ctx_free(yu_memctx_t *ctx) { ctx->free_ctx(ctx); }

void yu_default_ctx_init(yu_memctx_t *ctx);

yu_err yu_default_array_alloc_impl(yu_memctx_t *ctx, void **out, size_t elem_count, size_t elem_size, size_t alignment);
yu_err yu_default_array_realloc_impl(yu_memctx_t *ctx, void **ptr, size_t elem_count, size_t elem_size, size_t alignment);
void yu_default_array_free_impl(yu_memctx_t *ctx, void *ptr);
size_t yu_default_array_len_impl(yu_memctx_t * YU_UNUSED(ctx), void *ptr);

void *yu_xalloc(yu_memctx_t *ctx, size_t num, size_t elem_size);
