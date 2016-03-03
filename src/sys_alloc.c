/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "yu_common.h"

// but more correct (to let the compiler know we're using the
// address as an unsigned integer).
// Also shift off the lower byte or two which will always be 0
// since the addresses will be 8/16-byte aligned on x86(_64).
// Both of these functions are basically just FNV-1a hashes.

static
u64 addrhash_1(void *x) {
    u64 h = (u64)(uintptr_t)x >> (sizeof(void *)*2); \
    h ^=  h        & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (h >>  8) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (h >> 16) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (h >> 24) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (h >> 32) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (h >> 40) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (h >> 48)       ; h *= UINT64_C(0x100000001b3);
    return h;
}
static
u64 addrhash_2(void *x) {
    u64 h = UINT64_C(0x1ffffffffffffe2f),
        n = (u64)(uintptr_t)x >> (sizeof(void *)*2);
    h ^= (n >> 48)       ; h *= UINT64_C(0x100000001b3);
    h ^= (n >> 40) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (n >> 32) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (n >> 24) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (n >> 16) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^= (n >>  8) & 0xff; h *= UINT64_C(0x100000001b3);
    h ^=  n        & 0xff; h *= UINT64_C(0x100000001b3);
    return h;
}
#define addr_eq(x,y) ((x)==(y))

YU_HASHTABLE_IMPL(sysmem_tbl, void *, size_t, addrhash_1, addrhash_2, addr_eq)

static
u32 free_ctx_all(void *address, size_t YU_UNUSED(sz), void * YU_UNUSED(data)) {
    free(address);
    return 0;
}

YU_INLINE
void sys_alloc_ctx_free(sys_memctx_t *ctx) {
    sysmem_tbl_iter(&ctx->allocd, free_ctx_all, NULL);
    sysmem_tbl_free(&ctx->allocd);
}

yu_err sys_alloc(sys_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
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
    sysmem_tbl_put(&ctx->allocd, *out, alloc_sz, NULL);
    return YU_OK;
}

yu_err sys_realloc(sys_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
    // It's slightly unclear if pointers from aligned_alloc can safely be reallocated with
    // realloc. The POSIX standard doesn't mention it, but Linux manpages say it's okay.
    // Unsure about the ISO C11 standard.
    //
    // For now, sys_realloc to a alignment of 0 on a pointer that was originally aligned
    // with a non-0 argument to sys_align *MAY FAIL CATASTROPHICALLY AND WITHOUT WARNING*.
    // Fortunately, Yu never does that. ;-)
    // But this code is technically possibly broken.

    if (*ptr == NULL)
	return sys_alloc(ctx, ptr, num, elem_size, alignment);
    if (num == 0) {
	sys_free(ctx, *ptr);
	return YU_OK;
    }

    size_t old_sz, alloc_sz = num * elem_size;
    bool we_own_this = sysmem_tbl_remove(&ctx->allocd, *ptr, &old_sz);
    assert(we_own_this);

    void *safety_first;
    if (alignment == 0) {
	if ((safety_first = realloc(*ptr, alloc_sz)) == NULL)
	    return YU_ERR_ALLOC_FAIL;
	sysmem_tbl_put(&ctx->allocd, safety_first, alloc_sz, NULL);
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
    sysmem_tbl_put(&ctx->allocd, safety_first, alloc_sz, NULL);

    ok:
    // 0 out the newly allocated portion
    memset((u8 *)safety_first + old_sz, 0, alloc_sz - old_sz);
    *ptr = safety_first;
    return YU_OK;
}

void sys_free(sys_memctx_t *ctx, void *ptr) {
    sysmem_tbl_remove(&ctx->allocd, ptr, NULL);
    free(ptr);
}

YU_DEFAULT_ARRAY_ALLOC_IMPL(sys_array_alloc, sys_alloc)
YU_DEFAULT_ARRAY_FREE_IMPL(sys_array_free, sys_free)
YU_DEFAULT_ARRAY_REALLOC_IMPL(sys_array_realloc, sys_realloc, sys_array_alloc, sys_array_free)
