/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include <sys/mman.h>
#include <errno.h>
#include "bump_alloc.h"

struct bump_data {
    void *block;
    size_t used;
    size_t size;
};

yu_err bump_alloc_ctx_init(yu_memctx_t *ctx, size_t size) {
    assert(size > sizeof(struct bump_data));

    yu_default_alloc_ctx_init(ctx);

    void *block = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED)
        return YU_ERR_ALLOC_FAIL;

    struct bump_data *bmp = block;
    bmp->block = block;
    bmp->used = sizeof(struct bump_data);
    bmp->size = size;

    ctx->adata = bmp;

    ctx->alloc = bump_alloc;
    ctx->free = bump_free;
    ctx->realloc = bump_realloc;

    ctx->free_ctx = bump_alloc_ctx_free;

    return YU_OK;
}

void bump_alloc_ctx_free(yu_memctx_t *ctx) {
    struct bump_data *bmp = ctx->adata;
    munmap(bmp->block, bmp->size);
}

yu_err bump_alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
    assert(alignment == 0 || (alignment & (alignment - 1)) == 0);

    struct bump_data *bmp = ctx->adata;
    size_t alloc_sz = num * elem_size + sizeof(size_t);
    if (alloc_sz > bmp->size - bmp->used)
        return YU_ERR_ALLOC_FAIL;

    void *ptr;
    if (alignment == 0 || ((uintptr_t)bmp->block & (alignment - 1)) == 0) {
        ptr = (u8 *)bmp->block + bmp->used + sizeof(size_t);
        goto ok;
    }

    alloc_sz += alignment - 1;
    if (alloc_sz > bmp->size - bmp->used)
        return YU_ERR_ALLOC_FAIL;

    void *base = (u8 *)bmp->block + bmp->used + alignment - 1;
    ptr = (void *)((uintptr_t)base & ~(uintptr_t)(alignment - 1));

    ok:
    ((size_t *)ptr)[-1] = num * elem_size;
    *out = ptr;
    bmp->used += alloc_sz;
    return YU_OK;
}

size_t bump_alloc_size(void *ptr) {
    return ((size_t *)ptr)[-1];
}

// Super fast = stupid simple. Can't realloc or free.
yu_err bump_realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
    void *out;
    yu_err err = bump_alloc(ctx, &out, num, elem_size, alignment);
    if (err) return err;
    memcpy(out, *ptr, bump_alloc_size(*ptr));
    *ptr = out;
    return YU_OK;
}

void bump_free(yu_memctx_t * YU_UNUSED(ctx), void * YU_UNUSED(ptr)) {
}
