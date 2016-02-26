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
    X(VALUE_MATRIX) \
    X(VALUE_QUOT) \
    X(VALUE_LIST) \
    X(VALUE_TABLE)

DEF_ENUM(value_type, LIST_VALUE_TYPES)

struct boxed_value;
#include "nanbox.h" /* value_t */


bool value_eq(value_t a, value_t b);

typedef struct list_node {
    struct list_node *next;
    value_t v1;
    value_t v2;
    value_t v3;
    value_t v4;
    struct {
        bool i1 : 1;
        bool i2 : 1;
        bool i3 : 1;
        bool i4 : 1;
    } is_set;
} value_list;

YU_HASHTABLE(value_table, value_t, value_t, value_hash1, value_hash2, value_eq)

struct boxed_value {
    union {
        mpz_t i;
        mpfr_t r;
        yu_matrix v;
        yu_str s;
        value_list *lst;
        value_table *tbl;
    } v;
    yu_str tag;
    value_type what;
};
