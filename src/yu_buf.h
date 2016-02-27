#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_buf.h directly! Include yu_common.h instead."
#endif

#define YU_BUF_CTX_FREE_THRESHOLD 1
// Modern stacks are very very large, so 1MB should be fine.
#define YU_BUF_STACK_ALLOC_THRESHOLD (1024*1024)

#define YU_BUF_DAT(buf) ((struct yu_buf_dat *)(buf) - 1)

/* If `bytes` is text, is should be normalized and utf-8 encoded.
   Which normalization form doesn't matter as long as it's consistent. */
u64 yu_fnv1a(const u8 *bytes, u64 len);
u64 yu_murmur2(const u8 *bytes, u64 len);


u64 yu__u64hashfnv(u64 *key);
YU_INLINE
u64 yu__u64hashm2(u64 *key) {
    return yu_murmur2((u8 *)key, sizeof(u64) * 2);
}
YU_INLINE
bool yu__u64eq(u64 *a, u64 *b) {
    return a[0] == b[0] && a[1] == b[1];
}


typedef u8 * yu_buf;

YU_HASHTABLE(yu_buf_table, u64 *, yu_buf, yu__u64hashfnv, yu__u64hashm2, yu__u64eq)

struct yu_buf_dat;

typedef void (* yu_buf_udata_cleanup_func)(void *);

typedef struct {
    yu_buf_table frozen_bufs;
    struct yu_buf_dat *free;
    u64 num_free;
    yu_buf_udata_cleanup_func udata_free;
} yu_buf_ctx;

struct yu_buf_dat {
    u64 len;
    u32 capacity;  // 2^capacity is the number of bytes this buffer can hold

    yu_buf_ctx *ctx;

    void *base;  // the 'real' pointer that malloc() gave us; not necessarily aligned

    void *udata;

    bool is_frozen;
    u64 hash[2];

    u32 refs;

    struct yu_buf_dat *next;
};

void yu_buf_ctx_init(yu_buf_ctx *ctx);
void yu_buf_ctx_free(yu_buf_ctx *ctx);

/* Returns a new buffer of capacity 2^ceil(log2(size)) aligned to a 16-byte boundary.
   Returns NULL if the allocation failed. Generally use yu_buf_new instead. */
YU_MALLOC_LIKE YU_RETURN_ALIGNED(16)
yu_buf yu_buf_alloc(yu_buf_ctx *ctx, u64 size);

yu_buf yu_buf_new(yu_buf_ctx *ctx, const u8 *contents, u64 size, bool frozen);

void yu_buf_free(yu_buf buf);

yu_buf yu_buf_freeze(yu_buf buf);

YU_INLINE
u64 yu_buf_len(yu_buf buf) {
    return YU_BUF_DAT(buf)->len;
}
YU_INLINE
void *yu_buf_get_udata(yu_buf buf) {
    return YU_BUF_DAT(buf)->udata;
}
YU_INLINE
void yu_buf_set_udata(yu_buf buf, void *udata) {
    YU_BUF_DAT(buf)->udata = udata;
}

yu_buf yu_buf_cat(yu_buf a, yu_buf b, bool frozen);
