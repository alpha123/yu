/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "debug_alloc.h"

struct alloc_dat {
    struct alloc_dat *prev, *next;
    void *base;
    size_t size;
};

#define DAT(x) (((struct alloc_dat *)(x))-1)

yu_err debug_alloc_ctx_init(yu_memctx_t *ctx) {
    yu_default_alloc_ctx_init(ctx);

    // Head alloc_dat ptr
    ctx->adata = NULL;

    ctx->alloc = debug_alloc;
    ctx->free = debug_free;
    ctx->realloc = debug_realloc;

    ctx->free_ctx = debug_alloc_ctx_free;

    return YU_OK;
}

void debug_alloc_ctx_free(yu_memctx_t *ctx) {
    struct alloc_dat *ptr = ctx->adata, *next;
    while (ptr) {
        next = ptr->next;
        free(ptr->base);
        ptr = next;
    }
}

yu_err debug_alloc(yu_memctx_t *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
    size_t alloc_sz = num * elem_size;
    void *base;

    if (alignment == 0) {
        if ((base = malloc(alloc_sz + sizeof(struct alloc_dat))) == NULL)
            return YU_ERR_ALLOC_FAIL;
        memset(base, 0, alloc_sz + sizeof(struct alloc_dat));
        *out = (void *)((uintptr_t)base + sizeof(struct alloc_dat));
    }
    else {
        assert((alignment & (alignment - 1)) == 0);
        --alignment;
        if ((base = malloc(alloc_sz + sizeof(struct alloc_dat) + alignment)) == NULL)
            return YU_ERR_ALLOC_FAIL;
        memset(base, 0, alloc_sz + sizeof(struct alloc_dat) + alignment);
        *out = (void *)(((uintptr_t)base + alignment + sizeof(struct alloc_dat)) & ~(uintptr_t)alignment);
    }

    DAT(*out)->base = base;
    DAT(*out)->size = alloc_sz;
    DAT(*out)->next = ctx->adata;
    if (ctx->adata)
        ((struct alloc_dat *)ctx->adata)->prev = DAT(*out);
    ctx->adata = DAT(*out);

    return YU_OK;
}

yu_err debug_realloc(yu_memctx_t *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
    if (*ptr == NULL)
        return debug_alloc(ctx, ptr, num, elem_size, alignment);
    if (num == 0) {
        debug_free(ctx, *ptr);
        return YU_OK;
    }

    size_t old_sz = DAT(*ptr)->size, alloc_sz = num * elem_size;

    assert((alignment & (alignment - 1)) == 0);

    void *new_ptr;
    if (debug_alloc(ctx, &new_ptr, num, elem_size, alignment) != YU_OK)
        return YU_ERR_ALLOC_FAIL;

    memcpy(new_ptr, *ptr, old_sz);
    debug_free(ctx, *ptr);

    // 0 out the newly allocated portion
    memset((u8 *)new_ptr + old_sz, 0, alloc_sz - old_sz);

    *ptr = new_ptr;
    return YU_OK;
}

void debug_free(yu_memctx_t *ctx, void *ptr) {
    if (DAT(ptr)->prev == NULL)
        ctx->adata = DAT(ptr)->next;
    else {
        DAT(ptr)->prev->next = DAT(ptr)->next;
        if (DAT(ptr)->next)
            DAT(ptr)->next->prev = DAT(ptr)->prev;
    }
    free(DAT(ptr)->base);
}
