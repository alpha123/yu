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
    X(value_type, "Value type should be not depend on whether or not the value is boxed") \
    X(gray_bit, "Boxed values should maintain a gray bit") \
    X(packed, "Metadata about a value (type, owner, gray, etc) should be stored compacted")

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
    PT_ASSERT_EQ(boxed_value_owner(v), a);
END(ptr)

TEST(value_type)
    struct arena_handle *a = arena_new(&mctx);
    value_t w = value_from_int(655), x = value_true(),
            y = value_from_ptr(arena_alloc_val(a)),
            z = value_from_ptr(arena_alloc_val(a));
    boxed_value_set_type(value_to_ptr(y), VALUE_REAL);
    boxed_value_set_type(value_to_ptr(z), VALUE_FIXNUM);

    PT_ASSERT_EQ(value_what(w), VALUE_FIXNUM);
    PT_ASSERT_EQ(value_what(z), VALUE_FIXNUM);
    PT_ASSERT_EQ(value_what(x), VALUE_BOOL);
    PT_ASSERT_EQ(value_what(y), VALUE_REAL);
END(value_type)

TEST(gray_bit)
    struct arena_handle *a = arena_new(&mctx);
    struct boxed_value *v = arena_alloc_val(a);
    boxed_value_set_gray(v, false);
    PT_ASSERT(!boxed_value_is_gray(v));
    boxed_value_set_gray(v, true);
    PT_ASSERT(boxed_value_is_gray(v));
END(gray_bit)

TEST(packed)
    struct arena_handle *a = arena_new(&mctx), *b = arena_new(&mctx);
    struct boxed_value *v = arena_alloc_val(a);
    boxed_value_set_gray(v, true);
    boxed_value_set_type(v, VALUE_TABLE);
    PT_ASSERT_EQ(boxed_value_owner(v), a);
    boxed_value_set_owner(v, b);
    PT_ASSERT(boxed_value_is_gray(v));
    PT_ASSERT_EQ(boxed_value_owner(v), b);
    boxed_value_set_gray(v, false);
    PT_ASSERT_EQ(boxed_value_owner(v), b);
    PT_ASSERT(!boxed_value_is_gray(v));
    boxed_value_set_type(v, VALUE_REAL);
    PT_ASSERT_EQ(boxed_value_owner(v), b);
    PT_ASSERT(!boxed_value_is_gray(v));
    PT_ASSERT_EQ(boxed_value_get_type(v), VALUE_REAL);
END(packed)


SUITE(value, LIST_VALUE_TESTS)
