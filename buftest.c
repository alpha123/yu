#include <stdio.h>

#include "yu_common.h"

int main(int argc, char **argv) {
    yu_buf_ctx ctx;
    yu_buf_ctx_init(&ctx);
    yu_buf a, b, c;

    a = yu_buf_new(&ctx, "foo", 3, true);
    b = yu_buf_new(&ctx, "foo", 3, true);
    if (a == b)
        printf("worked\n");
    else
        printf("nope");

    c = yu_buf_cat(a, b, true);
    yu_buf_free(a);
    yu_buf_free(b);
    a = c;
    b = yu_buf_new(&ctx, "foofoo", 6, true);
    if (a == b)
        printf("worked\n");
    else
        printf("nope");
    yu_buf_free(a);
    yu_buf_free(b);

    yu_buf_ctx_free(&ctx);

    return 0;
}
