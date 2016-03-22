/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_hashtable.h directly! Include yu_common.h instead."
#endif

/**
 * Relatively simple cuckoo hash implementation. The only significant difference
 * from a textbook cuckoo hash table is that we store 2 key/value pairs per
 * bucket here. This improves maximum load factor dramatically (up to 80% in
 * some cases) at the cost a few more `eq` comparisons.
 * TODO should probably profile that.
 *
 * If you're looking to contribute, this code gets quite repetitive checking
 * left/right buckets and key1/key2. It's also most likely not as performant
 * as it could be. Perhaps an optimization would be to store left and right
 * buckets in the same array, with even indices being left and odd being right.
 * Since they're frequently accessed together, this could improve locality of
 * reference somewhat.
 * Also, buckets would most likely be better represented as a struct of arrays
 * instead of an array of structs.
 *
 * Another possible optimization could be moving is_set information
 * out of the buckets and into an auxillary array indexed by bucket number.
 * This would fit more buckets into cache (potentially many more), though
 * locality with is_set bits would be lost. I'm sure something could be worked out.
 *
 * keywords: cuckoo hash, hash table
 */

#define YU_HASHTABLE(tbl, key_t, val_t, hash1, hash2, eq) \
typedef u32 (* YU_NAME(tbl, iter_cb))(key_t, val_t, void *); \
\
struct YU_NAME(tbl, bucket) { \
    val_t val1; \
    val_t val2; \
    key_t key1; \
    key_t key2; \
    u8 is_set; \
}; \
\
typedef struct { \
    yu_allocator *memctx; \
    struct YU_NAME(tbl, bucket) *left; \
    struct YU_NAME(tbl, bucket) *right; \
    u64 size; \
    u8 capacity;  /* size of left and right is 2^capacity */ \
} tbl; \
\
void YU_NAME(tbl, init)(tbl *t, u64 init_capacity, yu_allocator *mctx); \
u32 YU_NAME(tbl, iter)(tbl *t, YU_NAME(tbl, iter_cb) cb, void *data); \
u8 YU_NAME(tbl, _findbucket_)(tbl *t, key_t k, struct YU_NAME(tbl, bucket) **b_out); \
bool YU_NAME(tbl, get)(tbl *t, key_t k, val_t *v_out); \
val_t YU_NAME(tbl, try_get)(tbl *t, key_t k, val_t default_val); \
void YU_NAME(tbl, _rehash_)(tbl *t, struct YU_NAME(tbl, bucket) *left, struct YU_NAME(tbl, bucket) *right, u64 cap); \
bool YU_NAME(tbl, _insert_)(tbl *t, bool left_insert, key_t k, val_t v, val_t *v_out, u8 iter_count); \
bool YU_NAME(tbl, put)(tbl *t, key_t k, val_t v, val_t *v_out); \
bool YU_NAME(tbl, remove)(tbl *t, key_t k, val_t *v_out);

#define YU_HASHTABLE_IMPL(tbl, key_t, val_t, hash1, hash2, eq) \
void YU_NAME(tbl, init)(tbl *t, u64 init_capacity, yu_allocator *mctx) { \
    /* Since we store 2 keys per bucket and have 2 bucket arrays, \
       if we were to use init_capacity as-passed the actual number of \
       storable key-value pairs would exceed the requested capacity \
       by quite a bit. Do a ceiling division to fix that. */ \
    u8 k = yu_ceil_log2(1 + (init_capacity - 1) / 4); \
    init_capacity = 1 << k; \
    t->left = yu_xalloc(mctx, init_capacity, sizeof(struct YU_NAME(tbl, bucket))); \
    t->right = yu_xalloc(mctx, init_capacity, sizeof(struct YU_NAME(tbl, bucket))); \
    t->memctx = mctx; \
    t->size = 0; \
    t->capacity = k; \
} \
void YU_NAME(tbl, free)(tbl *t) { \
    yu_free(t->memctx, t->left); \
    yu_free(t->memctx, t->right); \
}\
\
u32 YU_NAME(tbl, iter)(tbl *t, YU_NAME(tbl, iter_cb) cb, void *data) { \
    u32 stop_code; \
    u64 cap = 1 << t->capacity, sz = t->size; \
    struct YU_NAME(tbl, bucket) *b; \
    for (u64 i = 0, j = 0; i < cap && j < sz; i++) { \
        b = t->left + i; \
        if (b->is_set & 1) { \
            ++j; \
            if ((stop_code = cb(b->key1, b->val1, data))) \
                return stop_code; \
        } \
        if (b->is_set & 2) { \
            ++j; \
            if ((stop_code = cb(b->key2, b->val2, data))) \
                return stop_code; \
        } \
        b = t->right + i; \
        if (b->is_set & 1) { \
            ++j; \
            if ((stop_code = cb(b->key1, b->val1, data))) \
                return stop_code; \
        } \
        if (b->is_set & 2) { \
            ++j; \
            if ((stop_code = cb(b->key2, b->val2, data))) \
                return stop_code; \
        } \
    } \
    return 0; \
} \
\
u8 YU_NAME(tbl, _findbucket_)(tbl *t, key_t k, struct YU_NAME(tbl, bucket) **b_out) { \
    u64 cap, idx1, idx2; \
    struct YU_NAME(tbl, bucket) *b1, *b2; \
\
    cap = 1 << t->capacity; \
    idx1 = hash1(k) & (cap - 1); \
    idx2 = hash2(k) & (cap - 1); \
    b1 = t->left + idx1; \
    b2 = t->right + idx2; \
    if ((b1->is_set & 1) && eq(k, b1->key1)) { \
        *b_out = b1; \
        return 1; \
    } \
    else if ((b1->is_set & 2) && eq(k, b1->key2)) { \
        *b_out = b1; \
        return 2; \
    } \
    else if ((b2->is_set & 1) && eq(k, b2->key1)) { \
        *b_out = b2; \
        return 1; \
    } \
    else if ((b2->is_set & 2) && eq(k, b2->key2)) { \
        *b_out = b2; \
        return 2; \
    } \
    else { \
        *b_out = b1;  /* Insert in the first bucket */ \
        return 0;     /* 0 means an is_set check will always fail, \
                         so we don't need to set b_out to NULL \
                         and can instead use it for inserting. */ \
    } \
} \
\
bool YU_NAME(tbl, get)(tbl *t, key_t k, val_t *v_out) { \
    struct YU_NAME(tbl, bucket) *b; \
    u8 b_idx = YU_NAME(tbl, _findbucket_)(t, k, &b); \
    if (b_idx) { \
        if (v_out) *v_out = b_idx == 1 ? b->val1 : b->val2; \
        return true; \
    } \
    return false; \
} \
\
val_t YU_NAME(tbl, try_get)(tbl *t, key_t k, val_t default_val) { \
    val_t v; \
    if (YU_NAME(tbl, get)(t, k, &v)) \
        return v; \
    return default_val; \
} \
\
bool YU_NAME(tbl, _insert_)(tbl *t, bool left_insert, key_t k, val_t v, val_t *v_out, u8 iter_count); \
\
void YU_NAME(tbl, _rehash_)(tbl *t, struct YU_NAME(tbl, bucket) *left, struct YU_NAME(tbl, bucket) *right, u64 cap) { \
    struct YU_NAME(tbl, bucket) *b; \
    for (u64 i = 0; i < cap; i++) { \
        b = left + i; \
        /* No need to unset the buckets; they are old and get freed after the rehash anyway. */ \
        if (b->is_set & 1) \
            YU_NAME(tbl, _insert_)(t, true, b->key1, b->val1, NULL, 0); \
        if (b->is_set & 2) \
            YU_NAME(tbl, _insert_)(t, true, b->key2, b->val2, NULL, 0); \
        b = right + i; \
        /* Also, insert both old buckets into the left new bucket and let insert resolve collisions. */ \
        if (b->is_set & 1) \
            YU_NAME(tbl, _insert_)(t, true, b->key1, b->val1, NULL, 0); \
        if (b->is_set & 2) \
            YU_NAME(tbl, _insert_)(t, true, b->key2, b->val2, NULL, 0); \
    } \
} \
\
bool YU_NAME(tbl, _insert_)(tbl *t, bool left_insert, key_t k, val_t v, val_t *v_out, u8 iter_count) { \
    u64 cap = 1 << t->capacity, new_cap; \
    u64 idx = (left_insert ? hash1(k) : hash2(k)) & (cap - 1); \
    struct YU_NAME(tbl, bucket) *left = t->left, *right = t->right, \
      *b = left_insert ? left + idx : right + idx; \
    key_t move_k; \
    val_t move_v; \
\
    if (b->is_set == 0) { \
        b->key1 = k; \
        b->val1 = v; \
        b->is_set = 1; \
        return false; \
    } \
    else if (b->is_set == 1) { \
        if (eq(k, b->key1)) { \
            if (v_out) *v_out = b->val1; \
            b->val1 = v; \
            return true; \
        } \
        else { \
            b->key2 = k; \
            b->val2 = v; \
            b->is_set = 3; \
            return false; \
        } \
    } \
    else if (b->is_set == 2) { \
        if (eq(k, b->key2)) { \
            if (v_out) *v_out = b->val2; \
            b->val2 = v; \
            return true; \
        } \
        else { \
            b->key1 = k; \
            b->val1 = v; \
            b->is_set = 3; \
            return false; \
        } \
    } \
    else {  /* both slots are occupied in this bucket */ \
        if (eq(k, b->key1)) { \
            if (v_out) *v_out = b->val1; \
            b->val1 = v; \
            return true; \
        } \
        else if (eq(k, b->key2)) { \
            if (v_out) *v_out = b->val2; \
            b->val2 = v; \
            return true; \
        } \
        else { \
            /* If we've been shuffling for a while assume the table is pretty full */ \
            if (iter_count == 32) { \
                new_cap = 1 << ++t->capacity; \
                t->left = yu_xalloc(t->memctx, new_cap, sizeof(struct YU_NAME(tbl, bucket))); \
                t->right = yu_xalloc(t->memctx, new_cap, sizeof(struct YU_NAME(tbl, bucket))); \
                YU_NAME(tbl, _rehash_)(t, left, right, cap); \
                yu_free(t->memctx, left); \
                yu_free(t->memctx, right); \
                return YU_NAME(tbl, _insert_)(t, true, k, v, v_out, 0); \
            } \
            else { \
                move_k = b->key1; \
                move_v = b->val1; \
                b->key1 = k; \
                b->val1 = v; \
                YU_NAME(tbl, _insert_)(t, !left_insert, move_k, move_v, NULL, iter_count + 1); \
                return false; \
            } \
        } \
    } \
} \
\
bool YU_NAME(tbl, put)(tbl *t, key_t k, val_t v, val_t *v_out) { \
    bool exists = YU_NAME(tbl, _insert_)(t, true, k, v, v_out, 0); \
    t->size += !exists; \
    return exists; \
} \
\
bool YU_NAME(tbl, remove)(tbl *t, key_t k, val_t *v_out) { \
    struct YU_NAME(tbl, bucket) *b; \
    u8 b_idx = YU_NAME(tbl, _findbucket_)(t, k, &b); \
    if (b_idx) { \
        if (v_out) *v_out = b_idx == 1 ? b->val1 : b->val2; \
        b->is_set &= ~b_idx; \
        --t->size; \
        return true; \
    } \
    return false; \
}
