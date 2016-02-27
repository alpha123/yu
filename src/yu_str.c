#include "yu_common.h"

static struct yu_str_dat *get_strlist_node(yu_str_ctx *ctx) {
    YU_ERR_DEFVAR
    for (size_t i = 0; i < ctx->strdatpool_len; i++) {
        if (!ctx->strdatpool[i].is_used)
            return ctx->strdatpool + i;
    }
    size_t old_sz = ctx->strdatpool_len;
    ctx->strdatpool_len *= 2;
    struct yu_str_dat *mem_check = realloc(ctx->strdatpool, ctx->strdatpool_len * sizeof(struct yu_str_dat));
    memset(mem_check + old_sz, 0, old_sz * sizeof(struct yu_str_dat));
    YU_CHECK_ALLOC(mem_check);
    ctx->strdatpool = mem_check;

    return mem_check + old_sz;

    YU_ERR_DEFAULT_HANDLER(NULL)
}

static yu_err find_grapheme_clusters(yu_str s) {
    YU_ERR_DEFVAR
    u64 buflen = yu_buf_len(s), cnt = 0, gcstart, *idxs;
    struct yu_str_dat *d = YU_STR_DAT(s);

    idxs = buflen < YU_STR_BIG_THRESHOLD ? calloc(buflen, sizeof(u64)) : NULL;

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

void yu_str_ctx_init(yu_str_ctx *ctx) {
    YU_ERR_DEFVAR

    yu_buf_ctx_init(&ctx->bufctx);
    ctx->strdatpool_len = 2000;
    YU_CHECK_ALLOC(ctx->strdatpool = calloc(ctx->strdatpool_len, sizeof(struct yu_str_dat)));
    return;

    YU_ERR_DEFAULT_HANDLER(NOTHING())
}

void yu_str_ctx_free(yu_str_ctx *ctx) {
    for (size_t i = 0; i < ctx->strdatpool_len; i++) {
        if (ctx->strdatpool[i].is_used)
            yu_buf_free(ctx->strdatpool[i].str);
    }

    yu_buf_ctx_free(&ctx->bufctx);
    free(ctx->strdatpool);
}

// We now get ownership of utf8_nfc
YU_ERR_RET yu_str_adopt(yu_str_ctx *ctx, const u8 *utf8_nfc, u64 byte_len, yu_str *out) {
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
        free((void *)utf8_nfc);  // shut up clang
    }
    return 0;
    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

YU_ERR_RET yu_str_new(yu_str_ctx *ctx, const u8 *utf8, u64 len, yu_str *out) {
    YU_ERR_DEFVAR
    u8 *nfc;
    ssize_t nfc_sz;

    nfc_sz = utf8proc_map(utf8, len, &nfc, UTF8PROC_COMPOSE | UTF8PROC_STABLE | UTF8PROC_NLF2LS);

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

YU_ERR_RET yu_str_new_c(yu_str_ctx *ctx, const char *cstr, yu_str *out) {
    return yu_str_new(ctx, (const u8 *)cstr, strlen(cstr), out);
}

YU_ERR_RET yu_str_at(yu_str s, s64 idx, yu_str *char_out) {
    YU_ERR_DEFVAR

    struct yu_str_dat *d = YU_STR_DAT(s);
    YU_THROWIF(idx >= 0 ? idx >= d->egc_count : labs(idx) > d->egc_count, YU_ERR_STRING_INDEX_OUT_OF_BOUNDS);
    u64 actual_idx = idx < 0 ? d->egc_count + idx : idx, start, len,
        buflen = YU_BUF_DAT(s)->len, substrstart = 0, substrlen = 0;

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
    u64 al = yu_buf_len(a), bl = yu_buf_len(b);
    u8 *contents = calloc(al + bl, 1);
    YU_CHECK_ALLOC(contents);
    memcpy(contents, a, al);
    memcpy(contents + al, b, bl);
    YU_CHECK(yu_str_adopt(YU_STR_DAT(a)->ctx, contents, al + bl, out));
    return 0;
    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}
