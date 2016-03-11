/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

// The 64-bit integers are a unique, random code
// for this type. Primary use is hashing.
#define LIST_VALUE_TYPES(X) \
    X(VALUE_ERR,    UINT64_C(0x0000000000000000)) \
    X(VALUE_FIXNUM, UINT64_C(0xcd0d00325d47f5d7)) \
    X(VALUE_BOOL,   UINT64_C(0x5a0afbf6aea87b24)) \
    X(VALUE_DOUBLE, UINT64_C(0xe24675a519385cf0)) \
    X(VALUE_INT,    UINT64_C(0x78d21b919717779b)) \
    X(VALUE_REAL,   UINT64_C(0x64865eed2081a8e4)) \
    X(VALUE_STR,    UINT64_C(0x7fc8135de41ebbd0)) \
    X(VALUE_QUOT,   UINT64_C(0x9f70ef325b9a5b12)) \
    X(VALUE_TABLE,  UINT64_C(0x56927984b20a8d63))

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
	int fx;
	double d;
	bool b;
    } v;
    yu_str tag;

    struct {
        value_type what : 7;
        bool gray : 1;
    } bits;
};

value_type value_what(value_t val);

bool value_can_unbox_untagged(struct boxed_value *v);
value_t value_unbox(struct boxed_value *v);

YU_INLINE
value_type boxed_value_get_type(struct boxed_value *val) {
    return val->bits.what;
}

YU_INLINE
void boxed_value_set_type(struct boxed_value *val, value_type type) {
    assert((u8)type < 128);
    val->bits.what = type;
}

YU_INLINE
bool boxed_value_is_gray(struct boxed_value *val) {
    return val->bits.gray;
}

YU_INLINE
void boxed_value_set_gray(struct boxed_value *val, bool gray) {
    val->bits.gray = gray;
}

struct arena_handle *boxed_value_owner(struct boxed_value *val);
