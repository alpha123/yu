#include arena.h

static
YU_MALLOC_LIKE YU_RETURN_ALIGNED(GC_ARENA_ALIGNMENT)
struct arena *get_new_arena(void) {
    struct arena *a;
    if (posix_memalign(&a, GC_ARENA_ALIGNMENT, sizeof(struct arena)) != 0)
        return NULL;
    return a;
}

YU_ERR_RET arena_alloc(struct arena *restrict *restrict out) {
    YU_ERR_DEFVAR

    struct arena *a;
    YU_CHECK_ALLOC(a = get_new_arena());
    YU_CHECK_ALLOC(a->meta = calloc(1, sizeof(struct arena_metadata)));
    a->meta->self = a;
    a->grays = NULL;
    a->grayc = 0;
    a->next_free = a->objs;

    memset(a->freemap, 0, GC_BITMAP_SIZE * 16); /* set both freemap and markmap = 0 */

    *out = a;
    return YU_OK;

    YU_ERR_HANDLER_BEGIN
    YU_CATCH_ALL {
        *out = NULL;
        return yu_local_err;
    }
    YU_ERR_HANDLER_END
}

struct boxed_value *arena_bump_alloc_value(struct arena *a) {
    a->freemap[a->next_free/64] |= 1 << a->next_free;
    return a->objs[a->next_free++];
}

struct boxed_value *arena_fit_alloc_value(struct arena *a) {
    u16 free_idx;
    for (u8 i = 0; i < GC_BITMAP_SIZE/8; i++) {
        if (~a->freemap[i]) {
            free_idx = (u16)__builtin_clzll(~a->freemap[i]);
            break;
        }
    }
    a->freemap[free_idx/64] |= 1 << free_idx;
    return a->objs[free_idx];
}
