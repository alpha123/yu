/* ==========================================================================
 * phf.c - Tiny perfect hash function library.
 * --------------------------------------------------------------------------
 * Copyright (c) 2014-2015  William Ahern
 * C99 port Copyright (c) 2016 Peter Cannici
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ==========================================================================
 */

#include <errno.h>    /* errno */
#include "phf.h"


/*
 * M A C R O  R O U T I N E S
 *
 * Mostly copies of <sys/param.h>
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define PHF_BITS(T) (sizeof (T) * CHAR_BIT)
#define PHF_HOWMANY(x, y) (((x) + ((y) - 1)) / (y))
#define PHF_MIN(a, b) (((a) < (b))? (a) : (b))
#define PHF_MAX(a, b) (((a) > (b))? (a) : (b))
#define PHF_ROTL(x, y) (((x) << (y)) | ((x) >> (PHF_BITS(x) - (y))))
#define PHF_COUNTOF(a) (sizeof (a) / sizeof *(a))


/*
 * M O D U L A R  A R I T H M E T I C  R O U T I N E S
 *
 * Two modular reduction schemes are supported: bitwise AND and naive
 * modular division. For bitwise AND we must round up the values r and m to
 * a power of 2.
 *
 * TODO: Implement and test Barrett reduction as alternative to naive
 * modular division.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* round up to nearest power of 2 */
static inline size_t phf_powerup(size_t i) {
    return 1 << yu_ceil_log2(i);
}

static inline uint64_t phf_a_s_mod_n(uint64_t a, uint64_t s, uint64_t n) {
    uint64_t v;

    assert(n <= UINT32_MAX);

    v = 1;
    a %= n;

    while (s > 0) {
        if (s % 2 == 1)
            v = (v * a) % n;
        a = (a * a) % n;
        s /= 2;
    }

    return v;
}

/*
 * Rabin-Miller primality test adapted from Niels Ferguson and Bruce
 * Schneier, "Practical Cryptography" (Wiley, 2003), 201-204.
 */
static bool phf_witness(uint64_t n, uint64_t a, uint64_t s, uint64_t t) {
    uint64_t v, i;

    assert(a > 0 && a < n);
    assert(n <= UINT32_MAX);

    if (1 == (v = phf_a_s_mod_n(a, s, n)))
        return 1;

    for (i = 0; v != n - 1; i++) {
        if (i == t - 1)
            return 0;
        v = (v * v) % n;
    }

    return 1;
}

static bool phf_rabinmiller(uint64_t n) {
    /*
     * Witness 2 is deterministic for all n < 2047. Witnesses 2, 7, 61
     * are deterministic for all n < 4,759,123,141.
     */
    static const int witness[] = { 2, 7, 61 };
    uint64_t s, t, i;

    assert(n <= UINT32_MAX);

    if (n < 3 || n % 2 == 0)
        return 0;

    /* derive 2^t * s = n - 1 where s is odd */
    s = n - 1;
    t = 0;
    while (s % 2 == 0) {
        s /= 2;
        t++;
    }

    /* NB: witness a must be 1 <= a < n */
    if (n < 2027)
        return phf_witness(n, 2, s, t);

    for (i = 0; i < PHF_COUNTOF(witness); i++) {
        if (!phf_witness(n, witness[i], s, t))
            return 0;
    }

    return 1;
}

static bool phf_isprime(size_t n) {
    static const uint8_t map[] = { 0, 1, 2, 3, 0, 5, 0, 7 };
    size_t i;

    if (n < PHF_COUNTOF(map))
        return map[n];

    for (i = 2; i < PHF_COUNTOF(map); i++) {
        if (map[i] && (n % map[i] == 0))
            return 0;
    }

    return phf_rabinmiller(n);
}

static inline size_t phf_primeup(size_t n) {
    /* NB: 4294967291 is largest 32-bit prime */
    if (n > 4294967291)
        return 0;

    while (n < SIZE_MAX && !phf_isprime(n))
        n++;

    return n;
}


/*
 * B I T M A P  R O U T I N E S
 *
 * We use a bitmap to track output hash occupancy when searching for
 * displacement values.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef unsigned long phf_bits_t;

static inline bool phf_isset(phf_bits_t *set, size_t i) {
    return set[i / PHF_BITS(*set)] & ((size_t)1 << (i % PHF_BITS(*set)));
}

static inline void phf_setbit(phf_bits_t *set, size_t i) {
    set[i / PHF_BITS(*set)] |= ((size_t)1 << (i % PHF_BITS(*set)));
}

static inline void phf_clrbit(phf_bits_t *set, size_t i) {
    set[i / PHF_BITS(*set)] &= ~((size_t)1 << (i % PHF_BITS(*set)));
}

static inline void phf_clrall(phf_bits_t *set, size_t n) {
    memset(set, 0, PHF_HOWMANY(n, PHF_BITS(*set)) * sizeof(*set));
}


/*
 * K E Y  D E D U P L I C A T I O N
 *
 * Auxiliary routine to ensure uniqueness of each key in array.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static bool phf_str_neq(const phf_string_t * restrict a, const phf_string_t * restrict b) {
    return a->n != b->n || memcmp(a->p, b->p, a->n) != 0;
}

static int phf_str_cmp(const phf_string_t * restrict a, const phf_string_t * restrict b) {
    int cmp;
    if (a->n > b->n)
        return -1;
    if (a->n < b->n)
        return 1;
    if ((cmp = memcmp(a->p, b->p, a->n)) != 0)
        return cmp;
    return 0;
}

PHF_PUBLIC size_t phf_uniq(phf_string_t *k, const size_t n) {
    size_t i, j;

    qsort(k, n, sizeof *k, (int (*)(const void *, const void *))phf_str_cmp);

    for (i = 1, j = 0; i < n; i++) {
        if (phf_str_neq(k + i, k + j)) {
            k[++j].p = k[i].p;
            k[j].n = k[i].n;
        }
    }

    return n > 0 ? j + 1 : 0;
}

/*
 * H A S H  P R I M I T I V E S
 *
 * Universal hash based on MurmurHash3_x86_32. Variants for 32- and 64-bit
 * integer keys, and string keys.
 *
 * We use a random seed to address the non-cryptographic-strength collision
 * resistance of MurmurHash3. A stronger hash like SipHash is just too slow
 * and unnecessary for my particular needs. For some environments a
 * cryptographically stronger hash may be prudent.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static inline uint32_t phf_round32_i(uint32_t k1, uint32_t h1) {
    k1 *= UINT32_C(0xcc9e2d51);
    k1 = PHF_ROTL(k1, 15);
    k1 *= UINT32_C(0x1b873593);

    h1 ^= k1;
    h1 = PHF_ROTL(h1, 13);
    h1 = h1 * 5 + UINT32_C(0xe6546b64);

    return h1;
}

static inline uint32_t phf_round32(const unsigned char *p, size_t n, uint32_t h1) {
    uint32_t k1;

    while (n >= 4) {
        k1 = (p[0] << 24)
           | (p[1] << 16)
           | (p[2] << 8)
           | (p[3] << 0);

        h1 = phf_round32_i(k1, h1);

        p += 4;
        n -= 4;
    }

    k1 = 0;

    switch (n & 3) {
    case 3:
        k1 |= p[2] << 8;
        // FALLTHROUGH
    case 2:
        k1 |= p[1] << 16;
        // FALLTHROUGH
    case 1:
        k1 |= p[0] << 24;
        h1 = phf_round32_i(k1, h1);
    }

    return h1;
}

static inline uint32_t phf_mix32(uint32_t h1) {
    h1 ^= h1 >> 16;
    h1 *= UINT32_C(0x85ebca6b);
    h1 ^= h1 >> 13;
    h1 *= UINT32_C(0xc2b2ae35);
    h1 ^= h1 >> 16;

    return h1;
}


/*
 * g(k) & f(d, k)  S P E C I A L I Z A T I O N S
 *
 * For every key we first calculate g(k). Then for every group of collisions
 * from g(k) we search for a displacement value d such that f(d, k) places
 * each key into a unique hash slot.
 *
 * g() and f() are specialized for 32-bit, 64-bit, and string keys.
 *
 * g_mod_r() and f_mod_n() are specialized for the method of modular
 * reduction--modular division or bitwise AND. bitwise AND is substantially
 * faster than modular division, and more than makes up for any space
 * inefficiency, particularly for small hash tables.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* 32-bit, phf_string_t, and std::string keys */
static inline uint32_t phf_g(const phf_string_t k, uint32_t seed) {
    uint32_t h1 = seed;

    h1 = phf_round32(k.p, k.n, h1);

    return phf_mix32(h1);
}

static inline uint32_t phf_f(uint32_t d, const phf_string_t k, uint32_t seed) {
    uint32_t h1 = seed;

    h1 = phf_round32_i(d, h1);
    h1 = phf_round32(k.p, k.n, h1);

    return phf_mix32(h1);
}


/* g() and f() which parameterize modular reduction */
static inline uint32_t phf_g_mod_r(const phf_string_t k, uint32_t seed, size_t r, bool nodiv) {
    return nodiv ? phf_g(k, seed) & (r - 1) : phf_g(k, seed) % r;
}

static inline uint32_t phf_f_mod_m(uint32_t d, const phf_string_t k, uint32_t seed, size_t m, bool nodiv) {
    return nodiv ? phf_f(d, k, seed) & (m - 1) : phf_f(d, k, seed) % m;
}


/*
 * B U C K E T  S O R T I N G  I N T E R F A C E S
 *
 * For every key [0..n) we calculate g(k) % r, where 0 < r <= n, and
 * associate it with a bucket [0..r). We then sort the buckets in decreasing
 * order according to the number of keys. The sorting is required for both
 * optimal time complexity when calculating f(d, k) (less contention) and
 * optimal space complexity (smaller d).
 *
 * The actual sorting is done in the core routine. The buckets are organized
 * and sorted as a 1-dimensional array to minimize run-time memory (less
 * data structure overhead) and improve data locality (less pointer
 * indirection). The following section merely implements a templated
 * bucket-key structure and the comparison routine passed to qsort(3).
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static bool phf_str_eq(const phf_string_t * restrict a, const phf_string_t * restrict b) {
    return a->n == b->n && memcmp(a->p, b->p, a->n) == 0;
}

typedef struct {
    phf_string_t k;
    size_t *n;  /* number of keys in bucket g */
    phf_hash_t g; /* result of g(k) % r */
} phf_key;

static int phf_keycmp(const phf_key *a, const phf_key *b) {
    if (*(a->n) > *(b->n))
            return -1;
    if (*(a->n) < *(b->n))
            return 1;
    if (a->g > b->g)
            return -1;
    if (a->g < b->g)
            return 1;

    // Check for duplicate keys
    assert(!(phf_str_eq(&a->k, &b->k) && a != b));

    return 0;
}


/*
 * C O R E  F U N C T I O N  G E N E R A T O R
 *
 * The entire algorithm is contained in phf_init. Everything else in this
 * source file is either a simple utility routine used by phf_init, or an
 * interface to phf_init or the generated function state.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

PHF_PUBLIC phf_error_t phf_init(struct phf *phf, phf_string_t * const k, size_t n, size_t l, size_t a, phf_seed_t seed, bool nodiv, yu_allocator *mctx) {
    size_t n1 = PHF_MAX(n, 1); /* for computations that require n > 0 */
    size_t l1 = PHF_MAX(l, 1);
    size_t a1 = PHF_MAX(PHF_MIN(a, 100), 1);
    size_t r; /* number of buckets */
    size_t m; /* size of output array */
    phf_key *B_k = NULL; /* linear bucket-slot array */
    size_t *B_z = NULL;         /* number of slots per bucket */
    phf_key *B_p, *B_pe;
    phf_bits_t *T = NULL; /* bitmap to track index occupancy */
    phf_bits_t *T_b;      /* per-bucket working bitmap */
    size_t T_n;
    uint32_t *g = NULL; /* displacement map */
    uint32_t d_max = 0; /* maximum displacement value */
    phf_error_t error;

    if ((phf->nodiv = nodiv)) {
        /* round to power-of-2 so we can use bit masks instead of modulo division */
        r = phf_powerup(n1 / PHF_MIN(l1, n1));
        m = phf_powerup((n1 * 100) / a1);
    } else {
        r = phf_primeup(PHF_HOWMANY(n1, l1));
        /* XXX: should we bother rounding m to prime number for small n? */
        m = phf_primeup((n1 * 100) / a1);
    }

    if (r == 0 || m == 0)
        return ERANGE;

    phf->mem_ctx = mctx;

    B_k = yu_xalloc(mctx, n1, sizeof(*B_k));
    B_z = yu_xalloc(mctx, r, sizeof(*B_z));

    for (size_t i = 0; i < n; i++) {
        phf_hash_t g = phf_g_mod_r(k[i], seed, r, nodiv);

        B_k[i].k = k[i];
        B_k[i].g = g;
        B_k[i].n = &B_z[g];
        ++*B_k[i].n;
    }

    qsort(B_k, n1, sizeof *B_k, (int (*)(const void *, const void *))phf_keycmp);

    T_n = PHF_HOWMANY(m, PHF_BITS(*T));
    T = yu_xalloc(mctx, T_n * 2, sizeof(*T));
    T_b = &T[T_n]; /* share single allocation */

    /*
     * FIXME: T_b[] is unnecessary. We could clear T[] the same way we
     * clear T_b[]. In fact, at the end of generation T_b[] is identical
     * to T[] because we don't clear T_b[] on success.
     *
     * We just need to tweak the current reset logic to stop before the
     * key that failed, and then we can elide the commit to T[] at the
     * end of the outer loop.
     */

    g = yu_xalloc(mctx, r, sizeof(*g));

    B_p = B_k;
    B_pe = &B_k[n];

    for (; B_p < B_pe && *B_p->n > 0; B_p += *B_p->n) {
        phf_key *Bi_p, *Bi_pe;
        size_t d = 0;
        uint32_t f;
retry:
        d++;
        Bi_p = B_p;
        Bi_pe = B_p + *B_p->n;

        for (; Bi_p < Bi_pe; Bi_p++) {
            f = phf_f_mod_m(d, Bi_p->k, seed, m, nodiv);

            if (phf_isset(T, f) || phf_isset(T_b, f)) {
                /* reset T_b[] */
                for (Bi_p = B_p; Bi_p < Bi_pe; Bi_p++) {
                    f = phf_f_mod_m(d, Bi_p->k, seed, m, nodiv);
                    phf_clrbit(T_b, f);
                }

                goto retry;
            } else {
                phf_setbit(T_b, f);
            }
        }

        /* commit to T[] */
        for (Bi_p = B_p; Bi_p < Bi_pe; Bi_p++) {
            f = phf_f_mod_m(d, Bi_p->k, seed, m, nodiv);
            phf_setbit(T, f);
        }

        /* commit to g[] */
        g[B_p->g] = d;
        d_max = PHF_MAX(d, d_max);
    }

    phf->seed = seed;
    phf->r = r;
    phf->m = m;

    phf->g = g;
    g = NULL;

    phf->d_max = d_max;
    phf->g_op = nodiv ? PHF_G_UINT32_BAND_R : PHF_G_UINT32_MOD_R;
    phf->g_jmp = NULL;

    error = 0;

    goto clean;

    clean:
    yu_free(mctx, g);
    yu_free(mctx, T);
    yu_free(mctx, B_z);
    yu_free(mctx, B_k);

    return error;
}


/*
 * D I S P L A C E M E N T  M A P  C O M P A C T I O N
 *
 * By default the displacement map is an array of uint32_t. This routine
 * compacts the map by using the smallest primitive type that will fit the
 * largest displacement value.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

PHF_PUBLIC void phf_compact(struct phf *phf) {
    size_t size = 0;
    void *tmp;

    switch (phf->g_op) {
    case PHF_G_UINT32_MOD_R:
    case PHF_G_UINT32_BAND_R:
        break;
    default:
        return; /* already compacted */
    }

    if (phf->d_max <= 255) {
        for (size_t i = 0; i < phf->r; i++)
            ((uint8_t *)phf->g)[i] = (uint8_t)phf->g[i];
        phf->g_op = phf->nodiv ? PHF_G_UINT8_BAND_R : PHF_G_UINT8_MOD_R;
        size = sizeof(uint8_t);
    }
    else if (phf->d_max <= 65535) {
        for (size_t i = 0; i < phf->r; i++)
            ((uint16_t *)phf->g)[i] = (uint16_t)phf->g[i];
        phf->g_op = phf->nodiv ? PHF_G_UINT16_BAND_R : PHF_G_UINT16_MOD_R;
        size = sizeof(uint16_t);
    }
    else
        return; /* nothing to compact */

    /* simply keep old array if realloc fails */
    phf->g = yu_xrealloc(phf->mem_ctx, phf->g, phf->r, size);
}


/*
 * F U N C T I O N  G E N E R A T O R  &  S T A T E  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static inline phf_hash_t phf_hash_(const void *g_ptr, const phf_string_t k, uint32_t seed, size_t r, size_t m, bool nodiv) {
    const uint32_t *g = g_ptr;
    if (nodiv) {
        uint32_t d = g[phf_g(k, seed) & (r - 1)];

        return phf_f(d, k, seed) & (m - 1);
    }
    else {
        uint32_t d = g[phf_g(k, seed) % r];

        return phf_f(d, k, seed) % m;
    }
}

PHF_PUBLIC phf_hash_t phf_hash(struct phf *phf, const phf_string_t k) {
#if PHF_HAVE_COMPUTED_GOTOS && !PHF_NO_COMPUTED_GOTOS
    static const void *const jmp[] = {
        NULL,
        &&uint8_mod_r, &&uint8_band_r,
        &&uint16_mod_r, &&uint16_band_r,
        &&uint32_mod_r, &&uint32_band_r,
    };

    goto *((phf->g_jmp)? phf->g_jmp : (phf->g_jmp = jmp[phf->g_op]));

    uint8_mod_r:
        return phf_hash_((const uint8_t *)phf->g, k, phf->seed, phf->r, phf->m, false);
    uint8_band_r:
        return phf_hash_((const uint8_t *)phf->g, k, phf->seed, phf->r, phf->m, true);
    uint16_mod_r:
        return phf_hash_((const uint16_t *)phf->g, k, phf->seed, phf->r, phf->m, false);
    uint16_band_r:
        return phf_hash_((const uint16_t *)phf->g, k, phf->seed, phf->r, phf->m, true);
    uint32_mod_r:
        return phf_hash_((const uint32_t *)phf->g, k, phf->seed, phf->r, phf->m, false);
    uint32_band_r:
        return phf_hash_((const uint32_t *)phf->g, k, phf->seed, phf->r, phf->m, true);
#else
    switch (phf->g_op) {
    case PHF_G_UINT8_MOD_R:
        return phf_hash_((uint8_t *)phf->g, k, phf->seed, phf->r, phf->m, false);
    case PHF_G_UINT8_BAND_R:
        return phf_hash_((uint8_t *)phf->g, k, phf->seed, phf->r, phf->m, true);
    case PHF_G_UINT16_MOD_R:
        return phf_hash_((uint16_t *)phf->g, k, phf->seed, phf->r, phf->m, false);
    case PHF_G_UINT16_BAND_R:
        return phf_hash_((uint16_t *)phf->g, k, phf->seed, phf->r, phf->m, true);
    case PHF_G_UINT32_MOD_R:
        return phf_hash_((uint32_t *)phf->g, k, phf->seed, phf->r, phf->m, false);
    case PHF_G_UINT32_BAND_R:
        return phf_hash_((uint32_t *)phf->g, k, phf->seed, phf->r, phf->m, true);
    default:
        assert(!"PHF: Unknown operation");
        return 0;
    }
#endif
}

PHF_PUBLIC void phf_free(struct phf *phf) {
    yu_free(phf->mem_ctx, phf->g);
    phf->g = NULL;
}
