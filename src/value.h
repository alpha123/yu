/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

// Descriptions are unnecessary and unused but required to avoid
// ‘ISO C99 requires at least one argument to variadic macros’ warnings
// (See yu_common.h /DEF_ENUM)
#define LIST_VALUE_TYPES(X) \
    X(VALUE_ERR, "error") \
    X(VALUE_FIXNUM, "small number, stored unboxed") \
    X(VALUE_BOOL, "boolean") \
    X(VALUE_DOUBLE, "double") \
    X(VALUE_INT, "GMP mpz_t bigint") \
    X(VALUE_REAL, "MPFR mpfr_t arbitrary-precision real") \
    X(VALUE_STR, "yu_str") \
    X(VALUE_QUOT, "quotation, stored as bytecode") \
    X(VALUE_TABLE, "hashtable")

DEF_ENUM(value_type, LIST_VALUE_TYPES)

struct boxed_value;
#include "nanbox.h" /* value_t */


u64 value_hash1(value_t v);
u64 value_hash2(value_t v);
bool value_eq(value_t a, value_t b);

YU_HASHTABLE(value_table, value_t, value_t, value_hash1, value_hash2, value_eq)

struct boxed_value {
    union {
        // mpz and mpfr are actually quite big (mpfr is 32! bytes)
        // Use pointers to them to keep object size down, as well as
        // open the possibility of pooling them in the future.
        mpz_t *i;
        mpfr_t *r;
        yu_str s;
        value_table *tbl;
    } v;
    yu_str tag;

    /* Compact the following:
        bool gray;
        value_type what;
        struct arena_handle *owner;
       into a 64-bit int, since pointers even on amd64 are only 48-bits
    */
    u64 bits;
};

#define VALUE_PTR_MASK  UINT64_C(0xfffffffffffffff0)
#define VALUE_TYPE_MASK UINT64_C(0x0000000000000007)
#define VALUE_GRAY_MASK UINT64_C(0x0000000000000008)

value_type value_what(value_t val);

YU_INLINE
value_type boxed_value_get_type(struct boxed_value *val) {
    return val->bits & VALUE_TYPE_MASK;
}

YU_INLINE
void boxed_value_set_type(struct boxed_value *val, value_type type) {
    assert((u8)type < 128);
    val->bits = (val->bits & ~VALUE_TYPE_MASK) | (u8)type;
}

YU_INLINE
bool boxed_value_is_gray(struct boxed_value *val) {
    return val->bits & VALUE_GRAY_MASK;
}

YU_INLINE
bool boxed_value_set_gray(struct boxed_value *val, bool gray) {
    val->bits = (val->bits & ~VALUE_GRAY_MASK) | (-gray & VALUE_GRAY_MASK);
}

YU_INLINE
struct arena_handle *boxed_value_owner(struct boxed_value *val) {
    return (struct arena_handle *)(val->bits & VALUE_PTR_MASK);
}

YU_INLINE
void boxed_value_set_owner(struct boxed_value *val, struct arena_handle *owner) {
    val->bits = (val->bits & ~VALUE_PTR_MASK) | (uintptr_t)owner;
}

void boxed_value_mark(struct boxed_value *v);
