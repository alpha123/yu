#include "yu_common.h"

void *yu_xalloc(void *ctx, size_t num, size_t elem_size) {
    void *ptr;
    yu_err err;
    if ((err = yu_alloc(ctx, &ptr, num, elem_size, 0)) != YU_OK) {
        yu_global_fatal_handler(err);
        return NULL;
    }
    return ptr;
}

void *yu_xrealloc(void *ctx, void *ptr, size_t num, size_t elem_size) {
    yu_err err;
    if ((err = yu_realloc(ctx, &ptr, num, elem_size, 0)) != YU_OK) {
        yu_global_fatal_handler(err);
        return NULL;
    }
    return ptr;
}
