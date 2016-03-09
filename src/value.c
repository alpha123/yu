/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "arena.h"
#include "value.h"

YU_HASHTABLE_IMPL(value_table, value_t, value_t, value_hash1, value_hash2, value_eq)

value_type value_what(value_t val) {
    if (value_is_ptr(val))
        return boxed_value_get_type(value_to_ptr(val));
    if (value_is_bool(val))
        return VALUE_BOOL;
    if (value_is_int(val))
        return VALUE_FIXNUM;
    if (value_is_double(val))
        return VALUE_DOUBLE;
    return VALUE_ERR;
}

#define BUILD_TYPE_TABLE(enumval, randint) \
    [enumval] = randint,
static const u64 type_hashes[] = {
    LIST_VALUE_TYPES(BUILD_TYPE_TABLE)
};
#undef BUILD_TYPE_TABLE

static
u64 raw_value_hash1(value_t v) {
    switch (value_what(v)) {
    case VALUE_FIXNUM:
        return value_to_int(v);
    case VALUE_BOOL:
        return value_to_bool(v) ? UINT64_C(0xDECAFBAD) : UINT64_C(0xDEADBEEF);
    case VALUE_DOUBLE: {
        // Hashing doubles is somewhat problematic in general.
        // Double ‘equality’ is abs(a-b) < delta, so doubles that are
        // ‘equal’ might not hash the same with a naive algorithm.
        // Convert them to a string with 10 digits of precision, and
        // hash that.
        // TODO this is less than ideal.
        double x = value_to_double(v);
        char as_str[50];
        int n = snprintf(as_str, 50, "%.10", x);
        return yu_fnv1a((const u8 *)as_str, n);
    }
    case VALUE_TABLE:  // hash by address
    case VALUE_QUOT:
        return (uintptr_t)value_to_ptr(v);
    default:
        return *(u64 *)&v;
    }
}

static
u64 raw_value_hash2(value_t v) {
    switch (value_what(v)) {
    case VALUE_FIXNUM:
        return ~value_to_int(v);
    case VALUE_BOOL:
        return value_to_bool(v) ? UINT64_C(0xC0DE6EA55) : UINT64_C(0x5F3759DF);
    case VALUE_DOUBLE: {
        double x = value_to_double(v);
        char as_str[50];
        int n = snprintf(as_str, 50, "%.10", x);
        return yu_murmur2((const u8 *)as_str, n);
    }
    case VALUE_TABLE:
    case VALUE_QUOT:
        return ~(uintptr_t)value_to_ptr(v);
    default:
        return ~*(u64 *)&v;
    }
}

#define WRAP_HASH(hname, strhidx) \
    u64 value_ ## hname (value_t v) { \
        u64 h; \
        struct boxed_value *ptr = value_is_ptr(v) ? value_to_ptr(v) : NULL; \
        if (ptr && value_can_unbox_untagged(ptr)) \
	    v = value_unbox(ptr); \
	h = raw_value_ ## hname (v); \
        if (ptr && ptr->tag && yu_str_len(ptr->tag) > 0) \
            h ^= YU_BUF_DAT(ptr->tag)->hash[strhidx]; \
        return h ^ type_hashes[value_what(v)]; \
    }

WRAP_HASH(hash1, 0)
WRAP_HASH(hash2, 1)

#undef WRAP_HASH

bool value_eq(value_t a, value_t b) {
    return value_hash1(a) == value_hash1(b) &&
           value_hash2(a) == value_hash2(b);
}

bool value_can_unbox_untagged(struct boxed_value *v) {
    value_type what = boxed_value_get_type(v);
    if (what == VALUE_FIXNUM || what == VALUE_DOUBLE || what == VALUE_BOOL)
	return true;
    if (what == VALUE_INT && mpz_fits_sint_p(*v->v.i))
	return true;
    if (what == VALUE_REAL && mpfr_cmp_d(*v->v.r, DBL_MAX) < 1)
	return true;
    return false;
}

value_t value_unbox(struct boxed_value *v) {
    assert(value_can_unbox_untagged(v));
    switch (boxed_value_get_type(v)) {
    case VALUE_FIXNUM:
        return value_from_int(v->v.fx);
    case VALUE_DOUBLE:
        return value_from_double(v->v.d);
    case VALUE_BOOL:
        return value_from_bool(v->v.b);
    case VALUE_INT:
	return value_from_int((s32)mpz_get_si(*v->v.i));
    case VALUE_REAL:
	return value_from_double(mpfr_get_d(*v->v.r, MPFR_RNDN));
    default:
#ifdef __GNUC__
        __builtin_unreachable();
#endif
        return value_undefined();
    }
}

static
u32 mark_table(value_t key, value_t val, void * YU_UNUSED(data)) {
    if (value_is_ptr(key)) {
        struct boxed_value *k = value_to_ptr(key);
        arena_mark(boxed_value_owner(k), k);
    }
    if (value_is_ptr(val)) {
        struct boxed_value *v = value_to_ptr(val);
        arena_mark(boxed_value_owner(v), v);
    }
    return 0;
}

void boxed_value_mark(struct boxed_value *v) {
    switch (boxed_value_get_type(v)) {
    case VALUE_TABLE:
        value_table_iter(v->v.tbl, mark_table, NULL);
        break;
    }
}
