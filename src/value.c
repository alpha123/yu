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

struct arena_handle *boxed_value_owner(struct boxed_value *val) {
    return ((struct arena *)((uintptr_t)val & ~((1 << yu_ceil_log2(sizeof(struct arena))) - 1)))->meta;
}

#define BUILD_TYPE_TABLE(enumval, randint) \
    [enumval] = randint,
static const u64 type_hashes[] = {
    LIST_VALUE_TYPES(BUILD_TYPE_TABLE)
};
#undef BUILD_TYPE_TABLE

static
s32 tuple_hash1(value_t v, void *data) {
    u64 *h = data;
    *h ^= value_hash1(v);
    return 0;
}

static
s32 tuple_hash2(value_t v, void *data) {
    u64 *h = data;
    *h ^= value_hash2(v);
    return 0;
}

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
	// I attempted converting them to a string with 30 digits of
	// precision and hashing that instead, but that resulted in
	// quite a lot of collisions.
        // TODO this is less than ideal.
        double x = value_to_double(v);
	return *(u64 *)&x;
    }
    case VALUE_STR:
	return YU_BUF_DAT(value_to_ptr(v)->v.s)->hash[0];
    case VALUE_INT: {
	mpz_t *z = value_to_ptr(v)->v.i;
	char *s = alloca(mpz_sizeinbase(*z, 62) + 2);
	mpz_get_str(s, 62, *z);
	return yu_fnv1a((const u8 *)s, strlen(s));
    }
    case VALUE_REAL: {
	mpfr_t *r = value_to_ptr(v)->v.r;
	mpfr_exp_t exp;
	char *s = mpfr_get_str(NULL, &exp, 62, 0, *r, MPFR_RNDN);
	int slen = strlen(s);
	// Reuse the \0 for the exponent, so that 3.14 and 31.4 don't
	// hash the same.
	s[slen] = (char)exp;
	u64 h = yu_fnv1a((const u8 *)s, slen + 1);
	// Probably unnecessary, but restore the string to normal before freeing it.
	s[slen] = '\0';
	mpfr_free_str(s);
	return h;
    }
    case VALUE_TUPLE: {
	if (value_tuple_len(value_to_ptr(v)) == 0)
	    return UINT64_C(0xB16B00B5);
	u64 h = 0;
	value_tuple_foreach(value_to_ptr(v), tuple_hash1, &h);
	return h;
    }
    case VALUE_TABLE:  // hash by address
    case VALUE_QUOT:
        return (uintptr_t)value_to_ptr(v);
    default:
	assert(false);
#ifdef __GNUC__
	__builtin_unreachable();
#endif
        return 0;
    }
}

static
u64 raw_value_hash2(value_t v) {
    switch (value_what(v)) {
    case VALUE_FIXNUM: {
	int x = ~value_to_int(v);
	return yu_murmur2((const u8 *)&x, 4);
    }
    case VALUE_BOOL:
        return value_to_bool(v) ? UINT64_C(0xC0DE6EA55) : UINT64_C(0x5F3759DF);
    case VALUE_DOUBLE: {
        double x = value_to_double(v);
        return yu_murmur2((const u8 *)&x, sizeof(double));
    }
    case VALUE_STR:
	return YU_BUF_DAT(value_to_ptr(v)->v.s)->hash[1];
    case VALUE_INT: {
	mpz_t *z = value_to_ptr(v)->v.i;
	char *s = alloca(mpz_sizeinbase(*z, 62) + 2);
	mpz_get_str(s, 62, *z);
	return yu_murmur2((const u8 *)s, strlen(s));
    }
    case VALUE_REAL: {
	mpfr_t *r = value_to_ptr(v)->v.r;
	mpfr_exp_t exp;
	char *s = mpfr_get_str(NULL, &exp, 62, 0, *r, MPFR_RNDN);
	int slen = strlen(s);
	s[slen] = (char)exp;
	u64 h = yu_murmur2((const u8 *)s, slen + 1);
	s[slen] = '\0';
	mpfr_free_str(s);
	return h;
    }
    case VALUE_TUPLE: {
	u64 h = 0;
	if (value_tuple_len(value_to_ptr(v)) == 0)
	    return UINT64_C(0xCAFEBABE);
	value_tuple_foreach(value_to_ptr(v), tuple_hash2, &h);
	return h;
    }
    case VALUE_TABLE:
    case VALUE_QUOT:
        return ~(uintptr_t)value_to_ptr(v) * UINT64_C(0x7ffffffffffffe29);
    default:
	assert(false);
#ifdef __GNUC__
	__builtin_unreachable();
#endif
        return 0;
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
    if (value_is_ptr(a) && value_can_unbox_untagged(value_to_ptr(a)))
	a = value_unbox(value_to_ptr(a));
    if (value_is_ptr(b) && value_can_unbox_untagged(value_to_ptr(b)))
	b = value_unbox(value_to_ptr(b));
    return value_hash1(a) == value_hash1(b) &&
           value_hash2(a) == value_hash2(b);
}

s32 value_tuple_foreach(struct boxed_value *val, value_tuple_iter_fn iter, void *data) {
    assert(boxed_value_get_type(val) == VALUE_TUPLE);

    value_t v = val->v.tup[0];
    s32 res = 0;
    while (!value_is_empty(v)) {
	if ((res = iter(v, data)))
	    break;
	v = val->v.tup[1];
	if ((res = iter(v, data)))
	    break;
	if (value_is_empty((v = val->v.tup[2])))
	    break;
	else {
	    assert(value_is_ptr(v));
	    val = value_to_ptr(v);
	    assert(boxed_value_get_type(val) == VALUE_TUPLE);
	    v = val->v.tup[0];
	}
    }
    return res;
}

u64 value_tuple_len(struct boxed_value *val) {
    u64 len = 0;
    while (!value_is_empty(val->v.tup[2])) {
	len += !value_is_empty(val->v.tup[0]);
	len += !value_is_empty(val->v.tup[1]);
	val = value_to_ptr(val->v.tup[2]);
    }
    return len;
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
