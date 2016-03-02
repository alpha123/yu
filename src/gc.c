#if 0

#include "gc.h"

YU_INLINE
int gc_root_list_ptr_cmp(struct boxed_value *a, struct boxed_value *b) {
    u64 ua = (u64)a, ub = (u64)b;
    if (ua > ub)
        return -1;
    else if (ua < ub)
        return 1;
    return 0;
}

YU_SPLAYTREE_IMPL(root_list, struct boxed_value *, gc_root_list_ptr_cmp, true)

YU_ERR_RET gc_init(struct gc_info *gc) {
    YU_ERR_DEFVAR

    root_list_init(&gc->roots);

    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        YU_CHECK_ALLOC(arena_init(gc->arenas + i));
    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        gc->arenas[i].next_gen = i < GC_NUM_GENERATIONS - 1 ? gc->arenas + i : NULL;

    gc->alloc_pressure_score = 0;

    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void gc_free(struct gc_info *gc) {
    root_list_free(&gc->roots);
    for (u8 i = 0; i < GC_NUM_GENERATIONS; i++)
        arena_free(gc->arenas + i);
}

void gc_root(struct boxed_value *v) {
    root_list_insert(&gc->roots, v, NULL);
}

void gc_unroot(struct boxed_value *v) {
    root_list_remove(&gc->roots, v, NULL);
}

#endif
