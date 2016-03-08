/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "sys_alloc.h"
#include "arena.h"
#include "value.h"

#define SETUP \
    yu_memctx_t mctx; \
    sys_alloc_ctx_init(&mctx);

#define TEARDOWN \
    yu_alloc_ctx_free(&mctx);

#define LIST_VALUE_TESTS(X) \
    X(double, "Doubles can be stored inline") \
    X(fixnum, "Small ints can be stored inline") \
    X(bool, "Booleans can be stored inline") \
    X(ptr, "Boxed values should be heap-allocated and a pointer stored") \
    X(value_type, "Values know their type")

TEST(double)
    value_t x = value_from_double(42.101010);
    PT_ASSERT_EQ(value_to_double(x), 42.101010);
END(double)

TEST(fixnum)
    value_t x = value_from_int(322);
    PT_ASSERT_EQ(value_to_int(x), 322);
END(fixnum)

TEST(bool)
    value_t x = value_true();
    PT_ASSERT(value_to_bool(x));
    x = value_false();
    PT_ASSERT(!value_to_bool(x));
END(bool)

TEST(ptr)
    struct arena_handle *a = arena_new(&mctx);
    struct boxed_value *v = arena_alloc_val(a);
    value_t x = value_from_ptr(v);
    PT_ASSERT_EQ(value_to_ptr(x), v);
END(ptr)

TEST(value_type)
END(value_type)


SUITE(value, LIST_VALUE_TESTS)
