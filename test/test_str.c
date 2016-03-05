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
    assert(yu_str_new_c(&ctx, "foo", &s) == YU_OK);
    assert(yu_str_new_c(&ctx, "foo", &t) == YU_OK);
    PT_ASSERT_EQ(s, t);
END(intern)

TEST(length)
    yu_str s;
    // नि is two codepoints but 1 grapheme, and like 8 bytes in utf-8
    assert(yu_str_new_c(&ctx, "hiनिффनि", &s) == YU_OK);
    PT_ASSERT_EQ(yu_str_len(s), 6);
END(length)


SUITE(str, LIST_STR_TESTS)
