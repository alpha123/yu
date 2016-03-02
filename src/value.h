/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

#define LIST_VALUE_TYPES(X) \
    X(VALUE_ERR) \
    X(VALUE_FIXNUM) \
    X(VALUE_BOOL) \
    X(VALUE_DOUBLE) \
    X(VALUE_INT) \
    X(VALUE_REAL) \
    X(VALUE_STR) \
    X(VALUE_QUOT) \
    X(VALUE_TABLE)

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
    } v;
};
