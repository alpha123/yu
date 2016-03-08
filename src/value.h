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
        mpz_t i;
        mpfr_t r;
        yu_str s;
        value_table *tbl;
    } v;
    yu_str tag;
    struct {
        bool gray : 1;
        value_type what : 7;
    } x;
};

void value_mark(struct boxed_value *v);
