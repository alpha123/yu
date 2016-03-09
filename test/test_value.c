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
    X(packed, "Metadata about a value (type, owner, gray, etc) should be stored compacted") \
    X(hash, "Value hashes should be well-distributed")

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

static
int hash_cmp(const void *a, const void *b) {
    const u64 *ua = (const u64 *)a;
    const u64 *ub = (const u64 *)b;
    return (*ua > *ub) - (*ua < *ub);
}

static
s32 cmp_uint32(uint32_t a, uint32_t b) {
    return (a > b) - (a < b);
}

YU_SPLAYTREE(rndset, uint32_t, cmp_uint32, false)
YU_SPLAYTREE_IMPL(rndset, uint32_t, cmp_uint32, false)

TEST(hash)
    struct arena_handle *a = arena_new(&mctx);
    yu_str_ctx sctx;
    yu_str_ctx_init(&sctx, &mctx);
    sfmt_t rng;
    sfmt_init_gen_rand(&rng, 55123);

#ifdef TEST_FAST
    u64 valcnt = 800;
#else
    u64 valcnt = (u64)3e5;
#endif

    u64 *hashes1 = calloc(valcnt, sizeof(u64)),
        *hashes2 = calloc(valcnt, sizeof(u64));

    value_t v;
    struct boxed_value *b = arena_alloc_val(a);

    hashes1[0] = value_hash1(value_true());
    hashes1[1] = value_hash1(value_false());
    hashes2[0] = value_hash2(value_true());
    hashes2[1] = value_hash2(value_false());

    rndset rs;
    rndset_init(&rs, &mctx);

    for (u64 i = 2; i < valcnt; i++) {
        u32 n = sfmt_genrand_uint32(&rng);
        // Make sure we only generate unique numbers to
        // avoid false positive hash collisions.
        while (rndset_find(&rs, n, NULL))
            n = sfmt_genrand_uint32(&rng);
        rndset_insert(&rs, n, NULL);
        switch (n % 7) {
        case 0:
            v = value_from_int(n);
            break;
        case 1:
            v = value_from_double(sfmt_to_real1(n) * n);
            break;
        case 2: {
            mpz_t z;
            mpz_init_set_ui(z, n);
            boxed_value_set_type(b, VALUE_INT);
            b->v.i = &z;
            v = value_from_ptr(b);
            break;
        }
        case 3: {
            mpfr_t r;
            mpfr_init_set_d(r, sfmt_to_real1(n) * n, MPFR_RNDN);
            boxed_value_set_type(b, VALUE_REAL);
            b->v.r = r;
            v = value_from_ptr(b);
            break;
        }
        case 4: {
            int cnt = 12 + n % 8 * 4;
            u8 sdat[cnt];
            for (int j = 0; j < cnt/4; j++) {
                u32 m = sfmt_genrand_uint32(&rng);
                sdat[j*4]   =  (m        & 0xff) % 94 + 32;
                sdat[j*4+1] = ((m >>  8) & 0xff) % 94 + 32;
                sdat[j*4+2] = ((m >> 16) & 0xff) % 94 + 32;
                sdat[j*4+3] =  (m >> 24)         % 94 + 32;
            }
            yu_str s;
            yu_err err = yu_str_new(&sctx, sdat, cnt, &s);
            assert(err == YU_OK);
            boxed_value_set_type(b, VALUE_STR);
            b->v.s = s;
            v = value_from_ptr(b);
            break;
        }
        case 5:
            b = arena_alloc_val(a);
            boxed_value_set_type(b, VALUE_TABLE);
            v = value_from_ptr(b);
            break;
        case 6:
            b = arena_alloc_val(a);
            boxed_value_set_type(b, VALUE_QUOT);
            v = value_from_ptr(b);
            break;
        }
        hashes1[i] = value_hash1(v);
        hashes2[i] = value_hash2(v);
        if (value_what(v) == VALUE_INT)
            mpz_clear(*value_to_ptr(v)->v.i);
        else if (value_what(v) == VALUE_REAL)
            mpfr_clear(*value_to_ptr(v)->v.r);
    }

    u64 collisions1 = 0, collisions2 = 0;

    qsort(hashes1, valcnt, sizeof(u64), hash_cmp);
    qsort(hashes2, valcnt, sizeof(u64), hash_cmp);

    for (u64 i = 0; i < valcnt; i++) {
        if (i > 0 && hashes1[i-1] == hashes1[i])
            ++collisions1;
        if (i > 0 && hashes2[i-1] == hashes2[i])
            ++collisions2;
    }

    PT_ASSERT_LT(collisions1, valcnt/200);
    PT_ASSERT_LT(collisions2, valcnt/200);

    free(hashes1);
    free(hashes2);
    yu_str_ctx_free(&sctx);
END(hash)


SUITE(value, LIST_VALUE_TESTS)
