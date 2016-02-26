/*#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_skiplist.h directly! Include yu_common.h instead."
#endif*/

#define YU_SKIPLIST_MAP(sl, key_t, val_t, cmp, maxheight, seed)
#define YU_SKIPLIST_SET(sl, key_t, cmp, maxheight, seed)
#define YU_SKIPLIST_LIST(sl, val_t, cmp, maxheight, seed)

struct YU_NAME(sl, node1) {
    struct YU_NAME(sl, node1) *next;
    key_t key;
    val_t val;
};

struct YU_NAME(sl, node) {
    struct YU_NAME(sl, node1) *bottom;
    struct YU_NAME(sl, node) *next[maxheight-1];
    u8 height;
};

typedef struct {
    sfmt_t rng;
    struct YU_NAME(sl, node) *head;
    u64 size;
    u32 pending_height;
    u8 max_height;
    u8 height_shift;
} sl;

typedef s32 (* YU_NAME(sl, iter_cb))(key_t, val_t, void *);

void YU_NAME(sl, init)(sl *list);
void YU_NAME(sl, free)(sl *list);

YU_INLINE
u64 YU_NAME(sl, size)(sl *list) {
    return list->size;
}

u8 YU_NAME(sl, _gen_ins_height_)(sl *list);

bool YU_NAME(sl, insert)(sl *list, key_t key, val_t val, val_t *out);
bool YU_NAME(sl, find)(sl *list, key_t key, val_t *out);

s32 YU_NAME(sl, iter)(sl *list, YU_NAME(sl, iter_cb) cb, void *data);


#define YU_SKIPLIST_MAP_IMPL(sl, type, key_t, val_t, cmp, maxheight, seed)

void YU_NAME(sl, init)(sl *list) {
    sfmt_init_gen_rand(&list->rng, seed);
    list->height_shift = 0;
    list->pending_height = sfmt_genrand_uint32(&list->rng);
    list->head = NULL;
    list->max_height = 0;
}

void YU_NAME(sl, free)(sl *list) {
}

u8 YU_NAME(sl, _gen_ins_height_)(sl *list) {
    u8 h = __builtin_clz(list->pending_height);
    if (h + list->height_shift > 32) {
        list->pending_height = sfmt_genrand_uint32(&list->rng);
        list->height_shift = 0;
    }
    else {
        list->pending_height <<= h;
        list->height_shift += h;
    }
    return h;
}

bool YU_NAME(sl, insert)(sl *list, key_t key, val_t val, val_t *out) {
}

bool YU_NAME(sl, find)(sl *list, key_t key, val_t *out) {
    u8 height = list->max_height;
    s32 cmp_res;
    if (height == 0)
        goto not_found;
    struct YU_NAME(sl, node) *n = list->head;
    for (height--; height > 0; height--) {
        while (n->next[height-1]) {
            cmp_res = cmp(key, n->next[height-1]->key);
            if (cmp_res == 0)
                goto found;
        }
    }

    not_found:
    return false;
}

s32 YU_NAME(sl, iter)(sl *list, YU_NAME(sl, iter_cb) cb, void *data) {
    if (list->size == 0)
        return 0;
    s32 res;
    struct YU_NAME(sl, node1) *n = list->head->bottom->next[0]->bottom;
    while (n) {
        if ((res = cb(n->key, n->val, data)))
            return res;
        n = n->next;
    }
    return 0;
}
