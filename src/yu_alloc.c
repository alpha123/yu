#include "yu_common.h"

void yu_default_ctx_init(yu_memctx_t *ctx) {
    memset(ctx, 0, sizeof(yu_memctx_t));
    ctx->array_alloc = yu_default_array_alloc_impl;
    ctx->array_realloc = yu_default_array_realloc_impl;
    ctx->array_free = yu_default_array_free_impl;
    ctx->array_len = yu_default_array_len_impl;
}

yu_err yu_default_array_alloc_impl(yu_memctx_t *ctx, void **out, size_t elem_count, size_t elem_size, size_t alignment) {
    YU_ERR_DEFVAR
    void *base, *ptr;
    YU_CHECK(yu_alloc(ctx, &base, elem_count*elem_size + sizeof(size_t) + sizeof(void *) + alignment-1, 1, 16));
    ptr = (void *)(((uintptr_t)base + (alignment-1)) & ~(uintptr_t)(alignment-1));
    *((void **)ptr) = base;
    *((size_t *)((void **)ptr+1)) = elem_count;
    *out = (void *)((size_t *)((void **)ptr+1)+1);
    return YU_OK;
    yu_err_handler:
    *out = NULL;
    return yu_local_err;
}

size_t yu_default_array_len_impl(yu_memctx_t * YU_UNUSED(ctx), void *ptr) {
    return *((size_t *)ptr-1);
}

void yu_default_array_free_impl(yu_memctx_t *ctx, void *ptr) {
    yu_free(ctx, (void *)((void **)((size_t *)ptr-1)-1));
}

yu_err yu_default_array_realloc_impl(yu_memctx_t *ctx, void **ptr, size_t elem_count, size_t elem_size, size_t alignment) {
    YU_ERR_DEFVAR
    if (*ptr == NULL) return yu_array_alloc(ctx, ptr, elem_count, elem_size, alignment);
    if (elem_count == 0) {
	yu_array_free(ctx, *ptr);
	return YU_OK;
    }
    void *base = (void *)((void **)((size_t *)ptr-1)-1), *p;
    YU_CHECK(yu_realloc(ctx, &base, elem_count*elem_size + sizeof(size_t) + sizeof(void *) + alignment-1, 1, 16));
    p = (void *)(((uintptr_t)base + (alignment-1)) & ~(uintptr_t)(alignment-1));
    *((void **)p) = base;
    *((size_t *)((void **)p+1)) = elem_count;
    *ptr = (void *)((size_t *)((void **)p+1)+1);
    return YU_OK;
    yu_err_handler:
    return yu_local_err;
}

void *yu_xalloc(yu_memctx_t *ctx, size_t num, size_t elem_size) {
    void *ptr;
    yu_err err;
    if ((err = yu_alloc(ctx, &ptr, num, elem_size, 0)) != YU_OK) {
	yu_global_fatal_handler(err);
	return NULL;
    }
    return ptr;
}
