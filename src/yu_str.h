#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_str.h directly! Include yu_common.h instead."
#endif

#include "utf8proc/utf8proc.h"

#define YU_STR_BIG_THRESHOLD 1024

typedef yu_buf yu_str;

struct yu_str_dat;

typedef struct {
    yu_buf_ctx bufctx;
    struct yu_str_dat * restrict strdatpool;
    size_t strdatpool_len;
} yu_str_ctx;

struct yu_str_dat {
    bool is_used;

    // Extended Grapheme Cluster indexes in this string.
    // Only store this for small strings, so random access is fast
    // (O(1), just a couple of pointer dereferences) but uses more
    // memory (up to 2x more, which is actually the common case on
    // English text).
    // Big strings are stored without grapheme cluster data, so random
    // access is slower (O(n)) but uses much less memory.
    // We consider strings over YU_STR_BIG_THRESHOLD to be big; 1kibibyte
    // by default.
    //
    // TODO there's gotta be an efficient way to do this
    // It _really_ seems like the sort of thing that's possible to have
    // O(lg n) access time with O(n lg n) or better memory overhead.
    u64 *egc_idx;
    u64 egc_count;

    yu_str str;
    yu_str_ctx *ctx;
};

#define YU_STR_DAT(s) ((struct yu_str_dat *)(yu_buf_get_udata((s))))

void yu_str_ctx_init(yu_str_ctx *ctx, yu_memctx_t *mctx);
void yu_str_ctx_free(yu_str_ctx *ctx);

// Creates a new string and takes ownership of the pointer (utf8_nfc) passed to it.
YU_ERR_RET yu_str_adopt(yu_str_ctx *ctx, const u8 * restrict utf8_nfc, u64 byte_len, yu_str * restrict out);
// Like yu_str_adopt, but composes utf8 to NFC and copies it.
YU_ERR_RET yu_str_new(yu_str_ctx *ctx, const u8 * restrict utf8, u64 len, yu_str * restrict out);
YU_ERR_RET yu_str_new_c(yu_str_ctx *ctx, const char * restrict cstr, yu_str * restrict out);

YU_INLINE
u64 yu_str_len(yu_str s) {
    return YU_STR_DAT(s)->egc_count;
}

YU_ERR_RET yu_str_at(yu_str s, s64 idx, yu_str *char_out);

YU_ERR_RET yu_str_cat(yu_str a, yu_str b, yu_str *out);
