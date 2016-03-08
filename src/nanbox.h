/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Viktor Söderqvist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * nanbox.h
 * --------
 *
 * This file provides a is a way to store various types of data in a 64-bit
 * slot, including a type tag, using NaN-boxing.  NaN-boxing is a way to store
 * various information in unused NaN-space in the IEEE754 representation.  For
 * 64-bit platforms, unused bits in pointers are also used to encode various
 * information.  The representation in inspired by that used by Webkit's
 * JavaScriptCore.
 *
 * Datatypes that can be stored:
 *
 *   * int (int32_t)
 *   * double
 *   * pointer
 *   * boolean (true and false)
 *   * null
 *   * undefined
 *   * empty
 *   * deleted
 *   * aux 'auxillary data' (5 types of 48-bit values)
 *
 * Any value with the top 13 bits set represents a quiet NaN.  The remaining
 * bits are called the 'payload'. NaNs produced by hardware and C-library
 * functions typically produce a payload of zero.  We assume that all quiet
 * NaNs with a non-zero payload can be used to encode whatever we want.
 */

#pragma once

// This file does some magic. Let GCC and Clang know it's intentional.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"

/*
 * User-defined auxillary types. Default to void*. These types must be pointer
 * types or 32-bit types. (Pointers on 64-bit platforms always begin with 16
 * bits of zero.)
 */
#ifndef NANBOX_AUX1_TYPE
#define NANBOX_AUX1_TYPE void*
#endif
#ifndef NANBOX_AUX2_TYPE
#define NANBOX_AUX2_TYPE void*
#endif
#ifndef NANBOX_AUX3_TYPE
#define NANBOX_AUX3_TYPE void*
#endif
#ifndef NANBOX_AUX4_TYPE
#define NANBOX_AUX4_TYPE void*
#endif
#ifndef NANBOX_AUX5_TYPE
#define NANBOX_AUX5_TYPE void*
#endif


/*
 * In 32-bit mode, the double is unmasked. In 64-bit mode, the pointer is
 * unmasked.
 */
typedef union {
    uint64_t as_int64;
#ifndef YU_32BIT
    struct boxed_value *pointer;
#endif
    double as_double;
    struct {
#ifdef YU_BIG_ENDIAN
        uint32_t tag;
        uint32_t payload;
#else
        uint32_t payload;
        uint32_t tag;
#endif
    } as_bits;
} value_t;

#ifndef YU_32BIT

/*
 * 64-bit platforms
 *
 * This range of NaN space is represented by 64-bit numbers begining with
 * 13 bits of ones. That is, the first 16 bits are 0xFFF8 or higher.  In
 * practice, no higher value is used for NaNs.  We rely on the fact that no
 * valid double-precision numbers will be "higher" than this (compared as an
 * uint64).
 *
 * By adding 7 * 2^48 as a 64-bit integer addition, we shift the first 16 bits
 * in the doubles from the range 0000..FFF8 to the range 0007..FFFF.  Doubles
 * are decoded by reversing this operation, i.e. substracting the same number.
 *
 * The top 16-bits denote the type of the encoded nanbox_t:
 *
 *     Pointer {  0000:PPPP:PPPP:PPPP
 *             /  0001:xxxx:xxxx:xxxx
 *     Aux.   {           ...
 *             \  0005:xxxx:xxxx:xxxx
 *     Integer {  0006:0000:IIII:IIII
 *              / 0007:****:****:****
 *     Double  {          ...
 *              \ FFFF:****:****:****
 *
 * 32-bit signed integers are marked with the 16-bit tag 0x0006.
 *
 * The tags 0x0001..0x0005 can be used to store five additional types of
 * 48-bit auxillary data, each storing up to 48 bits of payload.
 *
 * The tag 0x0000 denotes a pointer, or another form of tagged immediate.
 * Boolean, 'null', 'undefined' and 'deleted' are represented by specific,
 * invalid pointer values:
 *
 *     False:     0x06
 *     True:      0x07
 *     Undefined: 0x0a
 *     Null:      0x02
 *     Empty:     0x00
 *     Deleted:   0x05
 *
 * All of these except Empty have bit 0 or bit 1 set.
 */

#define NANBOX_VALUE_EMPTY       0x0llu
#define NANBOX_VALUE_DELETED     0x5llu

// Booleans have bits 1 and 2 set. True also has bit 0 set.
#define NANBOX_VALUE_FALSE       0x06llu
#define NANBOX_VALUE_TRUE        0x07llu

// Null and undefined both have bit 1 set. Undefined also has bit 3 set.
#define NANBOX_VALUE_UNDEFINED   0x0Allu
#define NANBOX_VALUE_NULL        0x02llu

// This value is 7 * 2^48, used to encode doubles such that the encoded value
// will begin with a 16-bit pattern within the range 0x0007..0xFFFF.
#define NANBOX_DOUBLE_ENCODE_OFFSET 0x0007000000000000llu
// If the 16 first bits are 0x0002, this indicates an integer number.  Any
// larger value is a double, so we can use >= to check for either integer or
// double.
#define NANBOX_MIN_NUMBER           0x0006000000000000llu
#define NANBOX_HIGH16_TAG           0xffff000000000000llu

// There are 5 * 2^48 auxillary values can be stored in the 64-bit integer
// range NANBOX_MIN_AUX..NANBOX_MAX_AUX.
#define NANBOX_MIN_AUX_TAG          0x00010000
#define NANBOX_MAX_AUX_TAG          0x0005ffff
#define NANBOX_MIN_AUX              0x0001000000000000llu
#define NANBOX_MAX_AUX              0x0005ffffffffffffllu

// NANBOX_MASK_POINTER defines the allowed non-zero bits in a pointer.
#define NANBOX_MASK_POINTER         0x0000fffffffffffcllu

// The 'empty' value is guarranteed to consist of a repeated single byte,
// so that it should be easy to memset an array of nanboxes to 'empty' using
// NANBOX_EMPTY_BYTE as the value for every byte.
#define NANBOX_EMPTY_BYTE           0x0

// Define bool nanbox_is_xxx(value_t val) and value_t nanbox_xxx(void)
// with empty, deleted, true, false, undefined and null substituted for xxx.
#define NANBOX_IMMEDIATE_VALUE_FUNCTIONS(NAME, VALUE) \
    static YU_INLINE \
    value_t value_##NAME(void) { \
        value_t val; \
        val.as_int64 = VALUE; \
        return val; \
    } \
    static YU_INLINE \
    bool value_is_##NAME(value_t val) { \
        return val.as_int64 == VALUE; \
    }
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(empty, NANBOX_VALUE_EMPTY)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(deleted, NANBOX_VALUE_DELETED)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(false, NANBOX_VALUE_FALSE)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(true, NANBOX_VALUE_TRUE)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(undefined, NANBOX_VALUE_UNDEFINED)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(null, NANBOX_VALUE_NULL)

static YU_INLINE
bool value_is_undefined_or_null(value_t val) {
    // Undefined and null are the same if we remove the 'undefined' bit.
    return (val.as_int64 & ~8) == NANBOX_VALUE_NULL;
}

static YU_INLINE
bool value_is_bool(value_t val) {
    // True and false are the same if we remove the 'true' bit.
    return (val.as_int64 & ~1) == NANBOX_VALUE_FALSE;
}
static YU_INLINE
bool value_to_bool(value_t val) {
    assert(value_is_bool(val));
    return val.as_int64 & 1;
}
static YU_INLINE
value_t value_from_bool(bool b) {
    value_t val;
    val.as_int64 = b ? NANBOX_VALUE_TRUE : NANBOX_VALUE_FALSE;
    return val;
}

/* true if val is a double or an int */
static YU_INLINE
bool value_is_number(value_t val) {
    return val.as_int64 >= NANBOX_MIN_NUMBER;
}

static YU_INLINE
bool value_is_int(value_t val) {
    return (val.as_int64 & NANBOX_HIGH16_TAG) == NANBOX_MIN_NUMBER;
}
static YU_INLINE
value_t value_from_int(int32_t i) {
    value_t val;
    val.as_int64 = NANBOX_MIN_NUMBER | (uint32_t)i;
    return val;
}
static YU_INLINE
int32_t value_to_int(value_t val) {
    assert(value_is_int(val));
    return (int32_t)val.as_int64;
}

static YU_INLINE
bool value_is_double(value_t val) {
    return value_is_number(val) && !value_is_int(val);
}
static YU_INLINE
value_t value_from_double(double d) {
    value_t val;
    val.as_double = d;
    val.as_int64 += NANBOX_DOUBLE_ENCODE_OFFSET;
    assert(value_is_double(val));
    return val;
}
static YU_INLINE
double value_to_double(value_t val) {
    assert(value_is_double(val));
    val.as_int64 -= NANBOX_DOUBLE_ENCODE_OFFSET;
    return val.as_double;
}

static YU_INLINE
bool value_is_ptr(value_t val) {
    return !(val.as_int64 & ~NANBOX_MASK_POINTER) && val.as_int64;
}
static YU_INLINE
struct boxed_value *value_to_ptr(value_t val) {
    assert(value_is_ptr(val));
    return val.pointer;
}
static YU_INLINE
value_t value_from_ptr(struct boxed_value *pointer) {
    value_t val;
    val.pointer = pointer;
    assert(value_is_ptr(val));
    return val;
}

static YU_INLINE
bool value_is_aux(value_t val) {
    return val.as_int64 >= NANBOX_MIN_AUX &&
           val.as_int64 <= NANBOX_MAX_AUX;
}

/* end if !32-bit */
#else

/*
 * On 32-bit platforms we use the following NaN-boxing scheme:
 *
 * For values that do not contain a double value, the high 32 bits contain the
 * tag values listed below, which all correspond to NaN-space. When the tag is
 * 'pointer', 'integer' and 'boolean', their values (the 'payload') are store
 * in the lower 32 bits. In the case of all other tags the payload is 0.
 */
#define NANBOX_MAX_DOUBLE_TAG     0xfff80000
#define NANBOX_INT_TAG            0xfff80001
#define NANBOX_MIN_AUX_TAG        0xfff90000
#define NANBOX_MAX_AUX_TAG        0xfffdffff
#define NANBOX_POINTER_TAG        0xfffffffa
#define NANBOX_BOOLEAN_TAG        0xfffffffb
#define NANBOX_UNDEFINED_TAG      0xfffffffc
#define NANBOX_NULL_TAG           0xfffffffd
#define NANBOX_DELETED_VALUE_TAG  0xfffffffe
#define NANBOX_EMPTY_VALUE_TAG    0xffffffff

// The 'empty' value is guarranteed to consist of a repeated single byte,
// so that it should be easy to memset an array of nanboxes to 'empty' using
// NANBOX_EMPTY_BYTE as the value for every byte.
#define NANBOX_EMPTY_BYTE 0xff

/* The minimum uint64_t value for the auxillary range */
#define NANBOX_MIN_AUX            0xfff9000000000000llu
#define NANBOX_MAX_AUX            0xfffdffffffffffffllu

// Define nanbox_xxx and nanbox_is_xxx for deleted, undefined and null.
#define NANBOX_IMMEDIATE_VALUE_FUNCTIONS(NAME, TAG) \
    static YU_INLINE \
    value_t value_##NAME(void) { \
        value_t val; \
        val.as_bits.tag = TAG; \
        val.as_bits.payload = 0; \
        return val; \
    } \
    static YU_INLINE \
    bool value_is_##NAME(value_t val) { \
        return val.as_bits.tag == TAG; \
    }
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(deleted, NANBOX_DELETED_VALUE_TAG)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(undefined, NANBOX_UNDEFINED_TAG)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(null, NANBOX_NULL_TAG)

// The undefined and null tags differ only in one bit
static YU_INLINE
bool value_is_undefined_or_null(value_t val) {
    return (val.as_bits.tag & ~1) == NANBOX_UNDEFINED_TAG;
}

static YU_INLINE
value_t value_empty(void) {
    value_t val;
    val.as_int64 = 0xffffffffffffffffllu;
    return val;
}
static YU_INLINE
bool value_is_empty(value_t val) {
    return val.as_bits.tag == 0xffffffff;
}

/* Returns true if the value is auxillary space */
static YU_INLINE
bool value_is_aux(value_t val) {
    return val.as_bits.tag >= NANBOX_MIN_AUX_TAG &&
           val.as_bits.tag < NANBOX_POINTER_TAG;
}

// Define nanbox_is_yyy, nanbox_to_yyy and nanbox_from_yyy for
// boolean, int, pointer and aux1-aux5
#define NANBOX_TAGGED_VALUE_FUNCTIONS(NAME, TYPE, TAG) \
    static YU_INLINE \
    bool value_is_##NAME(value_t val) { \
        return val.as_bits.tag == TAG; \
    } \
    static YU_INLINE \
    TYPE value_to_##NAME(value_t val) { \
        assert(val.as_bits.tag == TAG); \
        return (TYPE)val.as_bits.payload; \
    } \
    static YU_INLINE \
    value_t value_from_##NAME(TYPE a) { \
        value_t val; \
        val.as_bits.tag = TAG; \
        val.as_bits.payload = (int32_t)a; \
        return val; \
    }

NANBOX_TAGGED_VALUE_FUNCTIONS(bool, bool, NANBOX_BOOLEAN_TAG)
NANBOX_TAGGED_VALUE_FUNCTIONS(int, int32_t, NANBOX_INT_TAG)
NANBOX_TAGGED_VALUE_FUNCTIONS(ptr, struct boxed_value *, NANBOX_POINTER_TAG)


static YU_INLINE
value_t value_true(void) {
    return value_from_bool(true);
}
static YU_INLINE
value_t value_false(void) {
    return value_from_bool(false);
}
static YU_INLINE
bool value_is_true(value_t val) {
    return val.as_bits.tag == NANBOX_BOOLEAN_TAG && val.as_bits.payload;
}
static YU_INLINE
bool value_is_false(value_t val) {
    return val.as_bits.tag == NANBOX_BOOLEAN_TAG && !val.as_bits.payload;
}

static YU_INLINE
bool value_is_double(value_t val) {
    return val.as_bits.tag < NANBOX_INT_TAG;
}
// is number = is double or is int
static YU_INLINE
bool value_is_number(value_t val) {
    return val.as_bits.tag <= NANBOX_INT_TAG;
}

static YU_INLINE
value_t value_from_double(double d) {
    value_t val;
    val.as_double = d;
    assert(value_is_double(val) &&
           val.as_bits.tag <= NANBOX_MAX_DOUBLE_TAG);
    return val;
}
static YU_INLINE
double value_to_double(value_t val) {
    assert(value_is_double(val));
    return val.as_double;
}

#endif /* is 32-bit */

/*
 * Representation-independent functions
 */

static YU_INLINE
double value_to_number(value_t val) {
    assert(value_is_number(val));
    return value_is_int(val) ? value_to_int(val)
                             : value_to_double(val);
}

// Re-enable int-to-pointer/pointer-to-int warnings.
#pragma GCC diagnostic pop
