/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "context.h"
#include "math_ops.h"

#define SETUP \
    yu_ctx ctx; \
    yu_err _cerr = yu_ctx_init(&ctx); \
    assert(_cerr == YU_OK);

#define TEARDOWN \
    yu_ctx_free(&ctx);

#define LIST_NUMERIC_TESTS(X) \
    X(int_overflow, "Integer overflow should become an arbitrary-precision int instead")

TEST(int_overflow)
END(int_overflow)


SUITE(numeric, LIST_NUMERIC_TESTS)
