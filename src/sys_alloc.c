/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "yu_common.h"
#include "internal_alloc.h"
#include "sys_alloc.h"

#define addrhash_1(x) ((uintptr_t)(x))
#define addrhash_2(x) (~(uintptr_t)(x))
#define addr_eq(x,y) ((x)==(y))

YU_HASHTABLE_IMPL(sysmem_tbl, void *, size_t, addrhash_1, addrhash_2, addr_eq)

#undef addr_eq
#undef addrhash_1
#undef addrhash_2

yu_err sys_alloc_ctx_init(yu_memctx_t *ctx) {
    yu_default_alloc_ctx_init(ctx);
    yu_memctx_t *tbl_mctx;

    if ((ctx->adata = calloc(1, sizeof(sysmem_tbl))) == NULL)
	return YU_ERR_ALLOC_FAIL;
    if ((tbl_mctx = calloc(1, sizeof(yu_memctx_t))) == NULL ||
	    internal_alloc_ctx_init(tbl_mctx) != YU_OK)
	return YU_ERR_ALLOC_FAIL;

    sysmem_tbl_init(ctx->adata, 2000, tbl_mctx);

    ctx->alloc = sys_alloc;
    ctx->free = sys_free;
    ctx->realloc = sys_realloc;

    ctx->free_ctx = sys_alloc_ctx_free;

    return YU_OK;
}

static
u32 free_ctx_all(void *address, size_t YU_UNUSED(sz), void * YU_UNUSED(data)) {
    free(address);
    return 0;
}

void sys_alloc_ctx_free(yu_memctx_t *ctx) {
    sysmem_tbl_iter((sysmem_tbl *)ctx->adata, free_ctx_all, NULL);
    sysmem_tbl_free((sysmem_tbl *)ctx->adata);
    yu_alloc_ctx_free(((sysmem_tbl *)ctx->adata)->memctx);
    free(ctx->adata);
}

yu_err sys_alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
    size_t alloc_sz = num * elem_size;

    // Default system alignment requested — we can return calloc directly.
    if (alignment == 0) {
	if ((*out = calloc(num, elem_size)) == NULL)
	    return YU_ERR_ALLOC_FAIL;
	goto ok;
    }

    // Otherwise use aligned_alloc and memset, which have a few invariants:

    // Alignment must be a power of 2.
    assert((alignment & (alignment - 1)) == 0);

    // Total allocated bytes must be a multiple of the alignment.
    if ((alloc_sz & (alignment - 1)) != 0)
	alloc_sz = (alloc_sz + alignment - 1) & ~(alignment - 1);

    if ((*out = aligned_alloc(alignment, alloc_sz)) == NULL)
	return YU_ERR_ALLOC_FAIL;
    memset(*out, 0, alloc_sz);

    // Now we have to store the original size so that we can reallocate properly.
    // Use a hash table keyed by address. This is also used to clean up a context.
    ok:
    sysmem_tbl_put((sysmem_tbl *)ctx->adata, *out, alloc_sz, NULL);
    return YU_OK;
}

yu_err sys_realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
    // It's slightly unclear if pointers from aligned_alloc can safely be reallocated with
    // realloc. The POSIX standard doesn't mention it, but Linux manpages say it's okay.
    // Unsure about the ISO C11 standard.
    //
    // For now, sys_realloc to a alignment of 0 on a pointer that was originally aligned
    // with a non-0 argument to sys_alloc *MAY FAIL CATASTROPHICALLY AND WITHOUT WARNING*.
    // Fortunately, Yu never does that. ;-)
    // But this code is technically possibly broken.

    if (*ptr == NULL)
	return sys_alloc(ctx, ptr, num, elem_size, alignment);
    if (num == 0) {
	sys_free(ctx, *ptr);
	return YU_OK;
    }

    size_t old_sz, alloc_sz = num * elem_size;
    bool we_own_this = sysmem_tbl_remove((sysmem_tbl *)ctx->adata, *ptr, &old_sz);
    assert(we_own_this);

    void *safety_first;
    if (alignment == 0) {
	if ((safety_first = realloc(*ptr, alloc_sz)) == NULL)
	    return YU_ERR_ALLOC_FAIL;
	goto ok;
    }

    // aligned_alloc's invariants, as above
    assert((alignment & (alignment - 1)) == 0);

    if ((alloc_sz & (alignment - 1)) != 0)
	alloc_sz = (alloc_sz + alignment - 1) & ~(alignment - 1);

    if ((safety_first = aligned_alloc(alignment, alloc_sz)) == NULL)
	return YU_ERR_ALLOC_FAIL;

    memcpy(safety_first, *ptr, old_sz);
    free(*ptr);

    ok:
    // 0 out the newly allocated portion
    memset((u8 *)safety_first + old_sz, 0, alloc_sz - old_sz);
    *ptr = safety_first;
    sysmem_tbl_put((sysmem_tbl *)ctx->adata, *ptr, alloc_sz, NULL);
    return YU_OK;
}

void sys_free(yu_memctx_t *ctx, void *ptr) {
    sysmem_tbl_remove((sysmem_tbl *)ctx->adata, ptr, NULL);
    free(ptr);
}
