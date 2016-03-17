/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "sys_alloc.h"

#define SETUP \
    yu_memctx_t mctx; \
    yu_str_ctx ctx; \
    sys_alloc_ctx_init(&mctx); \
    yu_str_ctx_init(&ctx, &mctx);

#define TEARDOWN \
    yu_str_ctx_free(&ctx); \
    yu_alloc_ctx_free(&mctx);

#define LIST_STR_TESTS(X) \
    X(intern, "Strings should be interned") \
    X(length, "Length should be in grapheme clusters")

TEST(intern)
    yu_str s, t;
    yu_err err;
    err = yu_str_new_z(&ctx, "foo", &s);
    assert(err == YU_OK);
    err = yu_str_new_z(&ctx, "foo", &t);
    assert(err == YU_OK);
    PT_ASSERT_EQ(s, t);
END(intern)

TEST(length)
    yu_str s;
    // नि is two codepoints but 1 grapheme, and like 8 bytes in utf-8
    yu_err err = yu_str_new_z(&ctx, "hiनिффनि", &s) ;
    assert(err == YU_OK);
    PT_ASSERT_EQ(yu_str_len(s), 6u);
END(length)


SUITE(str, LIST_STR_TESTS)
