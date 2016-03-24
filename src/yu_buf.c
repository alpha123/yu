#include "yu_common.h"

YU_HASHTABLE_IMPL(yu_buf_table, u64 *, yu_buf, yu__u64hashfnv, yu__u64hashm2, yu__u64eq)

u64 yu_fnv1a(const u8 *bytes, u64 len) {
    u64 hash = 0xcbf29ce484222325ull;
    // Hope the compiler vectorizes this
    for (u64 i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ull;
    }
    return hash;
}

u64 yu_murmur2(const u8 *bytes, u64 len) {
    const u64 m = 0xc6a4a7935bd1e995ull;
    u64 hash = 0x1fffffffffffffffull ^ len * m;
    for (u64 i = 0; i < len; i++) {
        u64 b = bytes[i];
        b *= m;
        b ^= b >> 47;
        b *= m;
        hash ^= b;
        hash *= m;
    }

    // Fallthrough
    switch (len & 7) {
    case 7: hash ^= (u64)bytes[6] << 48;
    case 6: hash ^= (u64)bytes[5] << 40;
    case 5: hash ^= (u64)bytes[4] << 32;
    case 4: hash ^= (u64)bytes[3] << 24;
    case 3: hash ^= (u64)bytes[2] << 16;
    case 2: hash ^= (u64)bytes[1] <<  8;
    case 1: hash ^= (u64)bytes[0]; hash *= m;
    }

    hash ^= hash >> 47;
    hash *= m;
    hash ^= hash >> 47;

    return hash;
}

u64 yu__u64hashfnv(u64 *key) {
    u64 h = 0xcbf29ce484222325ull,
        k = key[0];
    h ^= k & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 8) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 16) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 24) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 32) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 40) & 0xff;
    h *= 0x100000001b3ull;
    h ^= k >> 48;
    h *= 0x100000001b3ull;
    k = key[1];
    h ^= k & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 8) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 16) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 24) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 32) & 0xff;
    h *= 0x100000001b3ull;
    h ^= (k >> 40) & 0xff;
    h *= 0x100000001b3ull;
    h ^= k >> 48;
    h *= 0x100000001b3ull;
    return h;
}


static void yu_hash_buf(yu_buf buf) {
    struct yu_buf_dat *d = YU_BUF_DAT(buf);
    d->hash[0] = yu_fnv1a(buf, d->len);
    d->hash[1] = yu_murmur2(buf, d->len);
}

static void yu_buf_ctx_clean_free_list(yu_buf_ctx *ctx) {
    /* Assume that bigger buffers are reused less frequently,
       and thus clean the top 1/5 biggest. To avoid scanning the
       whole free list, we only scan the first 1/e of it, then
       free the biggest in that section and then any in the rest
       that are bigger than the largest from the sample until
       we get to the target count. */
    if (ctx->num_free > YU_BUF_CTX_FREE_THRESHOLD) {
        u64 sample_sz = (u64)floor(1.0/M_E * ctx->num_free),
            free_cnt = (u64)ceil(ctx->num_free / 3.0);
        u8 purged = 0;
        struct yu_buf_dat *big = NULL, *big_prev = NULL, *curr = ctx->free, *prev = NULL, *next;
        for (u64 i = 0; i < sample_sz; ++i) {
            if (big == NULL)
                big = curr;
            else if (curr->capacity > big->capacity ||
                     (curr->capacity == big->capacity && curr->len > big->len)) {
                big = curr;
                big_prev = prev;
            }
            prev = curr;
            curr = curr->next;
        }
        while (curr) {
            next = curr->next;
            if (big == NULL || curr->capacity > big->capacity ||
                (curr->capacity == big->capacity && curr->len > big->len)) {
                    if (prev)
                        prev->next = curr->next;
                    else
                        ctx->free = curr->next;
                    if (curr->udata != NULL)
                        ctx->udata_free(curr->udata);
                    yu_free(ctx->memctx, curr);
                    --ctx->num_free;
                    if (++purged == free_cnt)
                        break;
            }
            else
                prev = curr;
            curr = next;
        }
        if (big) {
            if (big_prev)
                big_prev->next = big->next;
            else
                ctx->free = big->next;
            if (big->udata != NULL)
                ctx->udata_free(big->udata);
            yu_free(ctx->memctx, big);
            --ctx->num_free;
        }
    }
}

static void null_free(void * YU_UNUSED(_)) { }

void yu_buf_ctx_init(yu_buf_ctx *ctx, yu_allocator *memctx) {
    yu_buf_table_init(&ctx->frozen_bufs, 10, memctx);
    ctx->memctx = memctx;
    ctx->free = NULL;
    ctx->num_free = 0;
    ctx->udata_free = null_free;
}

void yu_buf_ctx_free(yu_buf_ctx *ctx) {
    struct yu_buf_dat *curr = ctx->free, *next;
    while (curr) {
        next = curr->next;
        yu_free(ctx->memctx, curr);
        curr = next;
    }
    yu_buf_table_free(&ctx->frozen_bufs);
}


yu_buf yu_buf_alloc(yu_buf_ctx *ctx, u64 size) {
    u32 k = yu_ceil_log2(size);
    void *base;
    if (yu_alloc(ctx->memctx, &base, 1, sizeof(struct yu_buf_dat) + (1 << k), 16) != YU_OK)
        return NULL;
    struct yu_buf_dat *d = (struct yu_buf_dat *)base;
    d->ctx = ctx;
    d->capacity = k;
    d->len = 0;
    d->udata = NULL;
    d->refs = 1;
    d->next = NULL;
    d->is_frozen = false;
    d->hash[0] = d->hash[1] = 0;
    return (yu_buf)(d + 1);
}

yu_buf yu_buf_new(yu_buf_ctx *ctx, const u8 *contents, u64 size, bool frozen) {
    YU_ERR_DEFVAR
    u64 check_hash[2];
    yu_buf buf;

    if (frozen) {
        check_hash[0] = yu_fnv1a(contents, size);
        check_hash[1] = yu_murmur2(contents, size);
        if (yu_buf_table_get(&ctx->frozen_bufs, check_hash, &buf)) {
            if (YU_BUF_DAT(buf)->refs++ == 0)
                --ctx->num_free;
            return buf;
        }
    }

    YU_CHECK_ALLOC(buf = yu_buf_alloc(ctx, size));
    if (contents)
        memcpy(buf, contents, size);
    YU_BUF_DAT(buf)->len = contents ? size : 0;
    if (frozen)
        buf = yu_buf_freeze(buf);
    return buf;

    YU_ERR_DEFAULT_HANDLER(NULL)
}

void yu_buf_free(yu_buf buf) {
    struct yu_buf_dat *d = YU_BUF_DAT(buf);
    if (--d->refs == 0) {
        if (d->is_frozen) {
            d->next = d->ctx->free;
            d->ctx->free = d;
            ++d->ctx->num_free;
            yu_buf_ctx_clean_free_list(d->ctx);
        }
        else {
            if (d->udata != NULL)
                d->ctx->udata_free(d->udata);
            yu_free(d->ctx->memctx, d);
        }
    }
}

yu_buf yu_buf_freeze(yu_buf buf) {
    yu_buf old;
    struct yu_buf_dat *d = YU_BUF_DAT(buf);
    if (d->is_frozen)
        return buf;
    d->is_frozen = true;
    yu_hash_buf(buf);
    if (yu_buf_table_get(&d->ctx->frozen_bufs, d->hash, &old)) {
        /* Free buf instead of old, because we're returning the new, interned
           buffer here. This means we can free and replace it without the caller
           ‘noticing’, so to speak, while freeing `old` would invalidate any
           existing references to it. */
        yu_buf_free(buf);
        return old;
    }
    yu_buf_table_put(&d->ctx->frozen_bufs, d->hash, buf, &old);
    return buf;
}

yu_buf yu_buf_cat(yu_buf a, yu_buf b, bool frozen) {
    struct yu_buf_dat *d = YU_BUF_DAT(a), *e = YU_BUF_DAT(b);
    yu_buf c;
    u8 *cat;
    // If we're concatenating short strings and the buffer is immutable,
    // do the concat first so that yu_buf_new can check if the
    // resulting string already exists.
    if (frozen && d->len + e->len < YU_BUF_STACK_ALLOC_THRESHOLD) {
        cat = alloca(d->len + e->len);
        memcpy(cat, a, d->len);
        memcpy(cat + d->len, b, e->len);
        c = yu_buf_new(d->ctx, cat, d->len + e->len, true);
    }
    else {
        c = yu_buf_new(d->ctx, NULL, d->len + e->len, false);
        memcpy(c, a, d->len);
        memcpy(c + d->len, b, e->len);
        YU_BUF_DAT(c)->len = d->len + e->len;
        if (frozen)
            c = yu_buf_freeze(c);
    }
    return c;
}
