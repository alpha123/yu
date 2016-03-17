#include "yu_common.h"

static
struct yu_str_dat *get_strlist_node(yu_str_ctx *ctx) {
    YU_ERR_DEFVAR
    for (size_t i = 0; i < ctx->strdatpool_len; i++) {
        if (!ctx->strdatpool[i].is_used) {
            if (ctx->strdatpool[i].str != NULL) {
                yu_buf_free(ctx->strdatpool[i].str);
                ctx->strdatpool[i].str = NULL;
            }
            return ctx->strdatpool + i;
        }
    }
    size_t old_sz = ctx->strdatpool_len;
    ctx->strdatpool_len *= 2;
    YU_CHECK(yu_realloc(ctx->bufctx.memctx, (void **)&ctx->strdatpool,
                ctx->strdatpool_len, sizeof(struct yu_str_dat), 0));
    memset(ctx->strdatpool + old_sz, 0, old_sz * sizeof(struct yu_str_dat));

    return ctx->strdatpool + old_sz;

    YU_ERR_DEFAULT_HANDLER(NULL)
}

static
YU_ERR_RET find_grapheme_clusters(yu_str s) {
    YU_ERR_DEFVAR
    u64 buflen = yu_buf_len(s), cnt = 0, gcstart, *idxs;
    struct yu_str_dat *d = YU_STR_DAT(s);

    idxs = buflen < YU_STR_BIG_THRESHOLD ? yu_xalloc(d->ctx->bufctx.memctx, buflen, sizeof(u64)) : NULL;

    utf8proc_ssize_t incr;
    utf8proc_int32_t cp1, cp2;
    for (u64 i = 0; i < buflen;) {
        incr = utf8proc_iterate(s + i, buflen - i, &cp1);
        YU_THROWIF(incr < 0, YU_ERR_UNKNOWN);
        gcstart = i;
        i += incr;
        if (i < buflen) {
            incr = utf8proc_iterate(s + i, buflen - i, &cp2);
            YU_THROWIF(incr < 0, YU_ERR_UNKNOWN);

            while (!utf8proc_grapheme_break(cp1, cp2)) {
                cp1 = cp2;
                if (i < buflen) {
                    i += incr;
                    incr = utf8proc_iterate(s + i, buflen - i, &cp2);
                    YU_THROWIF(incr < 0, YU_ERR_UNKNOWN);
                }
                else
                    break;
            }
            ++cnt;
        }
        else
            cnt += utf8proc_category(cp1) != UTF8PROC_CATEGORY_CF;
        if (idxs)
            idxs[cnt - 1] = gcstart;
    }
    d->egc_count = cnt;
    d->egc_idx = idxs;
    return 0;

    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void yu_str_ctx_init(yu_str_ctx *ctx, yu_memctx_t *mctx) {
    YU_ERR_DEFVAR

    yu_buf_ctx_init(&ctx->bufctx, mctx);
    ctx->strdatpool_len = 2000;
    YU_CHECK_ALLOC(ctx->strdatpool = yu_xalloc(mctx, ctx->strdatpool_len, sizeof(struct yu_str_dat)));
    return;

    YU_ERR_DEFAULT_HANDLER(NOTHING())
}

void yu_str_ctx_free(yu_str_ctx *ctx) {
    for (size_t i = 0; i < ctx->strdatpool_len; i++) {
        if (ctx->strdatpool[i].is_used)
            yu_buf_free(ctx->strdatpool[i].str);
    }

    yu_free(ctx->bufctx.memctx, ctx->strdatpool);
    yu_buf_ctx_free(&ctx->bufctx);
}

static
void *alloc_wrapper(void *mctx, void *ptr, size_t size) {
    yu_memctx_t *memctx = (yu_memctx_t *)mctx;
    if (ptr == NULL)
        return yu_xalloc(memctx, size, 1);
    if (yu_realloc(memctx, &ptr, size, 1, 0) != YU_OK)
        return NULL;
    return ptr;
}

static
void free_wrapper(void *mctx, void *ptr) {
    yu_free((yu_memctx_t *)mctx, ptr);
}

// We now get ownership of utf8_nfc
YU_ERR_RET yu_str_adopt(yu_str_ctx *ctx, const u8 * restrict utf8_nfc, u64 byte_len, yu_str * restrict out) {
    YU_ERR_DEFVAR
    struct yu_str_dat *sdat;
    *out = yu_buf_new(&ctx->bufctx, utf8_nfc, byte_len, true);
    if (yu_buf_get_udata(*out) == NULL) {  // Not interned yet
        sdat = get_strlist_node(ctx);
        sdat->str = *out;
        sdat->ctx = ctx;
        sdat->is_used = true;
        yu_buf_set_udata(*out, sdat);
        YU_CHECK(find_grapheme_clusters(*out));
    }
    else {
        // This string already exists, which means we can (and should) free
        // utf8_nfc (we own him now).
        yu_free(ctx->bufctx.memctx, (void *)utf8_nfc);  // shut up clang
        // In case the string has been ‘freed’ (i.e. set to unused, but still
        // holding on to allocated memory), say we're using it again.
        ((struct yu_str_dat *)yu_buf_get_udata(*out))->is_used = true;
    }
    return 0;
    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

YU_ERR_RET yu_str_new(yu_str_ctx *ctx, const u8 * restrict utf8, u64 len, yu_str * restrict out) {
    YU_ERR_DEFVAR
    u8 *nfc;
    ssize_t nfc_sz;

    nfc_sz = utf8proc_map(utf8, len, &nfc, UTF8PROC_COMPOSE | UTF8PROC_STABLE | UTF8PROC_NLF2LS,
            alloc_wrapper, free_wrapper, ctx->bufctx.memctx);

    if (nfc_sz < 0) switch (nfc_sz) {
    case UTF8PROC_ERROR_NOMEM: YU_THROW(YU_ERR_ALLOC_FAIL);
    case UTF8PROC_ERROR_INVALIDUTF8: YU_THROW(YU_ERR_BAD_STRING_ENCODING);
    case UTF8PROC_ERROR_OVERFLOW: YU_THROW(YU_ERR_UNKNOWN_FATAL);
    default: YU_THROW(YU_ERR_UNKNOWN);
    }

    YU_CHECK(yu_str_adopt(ctx, nfc, nfc_sz, out));
    return 0;

    YU_ERR_HANDLER_BEGIN
    YU_HANDLE_FATALS
    YU_CATCH_ALL
      *out = NULL;
    YU_ERR_HANDLER_END
    return yu_local_err;
}

YU_ERR_RET yu_str_new_z(yu_str_ctx *ctx, const char * restrict cstr, yu_str * restrict out) {
    return yu_str_new(ctx, (const u8 *)cstr, strlen(cstr), out);
}

void yu_str_free(yu_str s) {
    // Memory from `s` will be freed later if requested; until then keep it around
    // in case the same string is requested again.
    YU_STR_DAT(s)->is_used = false;
}

YU_ERR_RET yu_str_at(yu_str s, s64 idx, yu_str *char_out) {
    YU_ERR_DEFVAR

    // `abs` still returns a signed integer, so cast to unsigned
    // to get GCC to shut up.
    struct yu_str_dat *d = YU_STR_DAT(s);
    YU_THROWIF(idx >= 0 ? (u64)idx >= d->egc_count : (u64)llabs(idx) > d->egc_count, YU_ERR_STRING_INDEX_OUT_OF_BOUNDS);
    u64 actual_idx = idx < 0 ? d->egc_count - (u64)llabs(idx) : (u64)idx, start, len,
        buflen = YU_BUF_DAT(s)->len;

    if (YU_STR_DAT(s)->egc_idx != NULL) {
        start = d->egc_idx[actual_idx];
        len = actual_idx == d->egc_count - 1 ? buflen - start : d->egc_idx[actual_idx + 1] - start;
        YU_CHECK(yu_str_new(d->ctx, s + start, len, char_out));
    }
    return 0;

    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

YU_ERR_RET yu_str_cat(yu_str a, yu_str b, yu_str *out) {
    YU_ERR_DEFVAR

    assert(YU_STR_DAT(a)->ctx == YU_STR_DAT(b)->ctx);

    u64 al = yu_buf_len(a), bl = yu_buf_len(b);
    u8 *contents = yu_xalloc(YU_STR_DAT(a)->ctx->bufctx.memctx, al + bl, 1);
    YU_CHECK_ALLOC(contents);
    memcpy(contents, a, al);
    memcpy(contents + al, b, bl);
    YU_CHECK(yu_str_adopt(YU_STR_DAT(a)->ctx, contents, al + bl, out));
    return 0;
    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

YU_ERR_RET yu_str_cat_z(yu_str a, const char * restrict cstr, yu_str *out) {
    YU_ERR_DEFVAR

    yu_str temp;
    YU_CHECK(yu_str_new_z(YU_STR_DAT(a)->ctx, cstr, &temp));
    YU_CHECK(yu_str_cat(a, temp, out));
    yu_str_free(temp);

    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}
