/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#define SYSTEM_ALLOCATOR(func, ...) sys_ ## func (__VA_ARGS__)

#define addrhash_1 ahash1
#define addrhash_2 ahash2
#define addr_eq(x,y) ((x)==(y))
YU_HASHTABLE(sysmem_tbl, void *, size_t, addrhash_1, addrhash_2, addr_eq)
#undef addr_hash1
#undef addr_hash2
#undef addr_eq

typedef struct {
    sysmem_tbl allocd;
} sys_memctx_t;

YU_INLINE
yu_err sys_alloc_ctx_init(sys_memctx_t *ctx) {
    sysmem_tbl_init(&ctx->allocd, 2000);
    return YU_OK;
}

void sys_alloc_ctx_free(sys_memctx_t *ctx);

yu_err sys_alloc(sys_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment);
yu_err sys_realloc(sys_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment);
void sys_free(sys_memctx_t *ctx, void *ptr);

yu_err sys_array_alloc(sys_memctx_t *ctx, void **out, size_t elem_count, size_t elem_size, size_t alignment);
//YU_DEFAULT_ARRAY_LEN_IMPL(sys_array_len)
void sys_array_free(sys_memctx_t *ctx, void *ptr);
yu_err sys_array_realloc(sys_memctx_t *ctx, void **ptr, size_t elem_count, size_t elem_size, size_t alignment);
