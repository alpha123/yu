#include "yu_common.h"

#include <stdio.h>
#include <inttypes.h>

int main(int argc, char **argv) {
    YU_ERR_DEFVAR

    yu_str s, t, u;
    yu_str_ctx ctx;
    yu_str_ctx_init(&ctx);
    YU_CHECK(yu_str_new_c(&ctx, argv[1], &s));
    YU_CHECK(yu_str_new_c(&ctx, argv[1], &t));

    printf(s == t ? "worked\n" : "nope\n");
    printf("len: %" PRIu64 "\n", yu_str_len(s));

    for (u64 i = 0; i < yu_str_len(s); i++) {
        YU_CHECK(yu_str_at(s, i, &t));
        printf("%" PRIu64 ": ", i);
        printf("%" PRIu64 ",%" PRIu64 " ", yu_str_len(t), yu_buf_len(t));
        for (u64 j = 0; j < yu_buf_len(t); j++)
            printf("0x%x ", t[j]);
        printf("%.*s\n", yu_buf_len(t), t);
    }

    yu_str_new_c(&ctx, "bar", &t);
    yu_str_new_c(&ctx, "baz", &t);

    for (u64 i = 0; i < yu_buf_len(s); i++)
        printf("0x%x ", s[i]);
    printf("\n");

    YU_CHECK(yu_str_new_c(&ctx, "नि", &t));

    u = s;
    YU_CHECK(yu_str_cat(u, t, &s));
    printf("'%.*s' ++ '%.*s' == '%.*s'\n", yu_buf_len(u), u, yu_buf_len(t), t, yu_buf_len(s), s);
    for (u64 i = 0; i < yu_str_len(s); i++) {
        YU_CHECK(yu_str_at(s, i, &t));
        printf("%" PRIu64 ": ", i);
        printf("%" PRIu64 ",%" PRIu64 " ", yu_str_len(t), yu_buf_len(t));
        for (u64 j = 0; j < yu_buf_len(t); j++)
            printf("0x%x ", t[j]);
        printf("%.*s\n", yu_buf_len(t), t);
    }

    yu_str_ctx_free(&ctx);
    return 0;

    yu_err_handler:
    yu_str_ctx_free(&ctx);
    printf("ERROR: %s\n", yu_err_msg(yu_local_err));
    return yu_local_err;
}
