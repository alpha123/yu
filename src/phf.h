/* ==========================================================================
 * phf.h - Tiny perfect hash function library.
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

#pragma once

#include "yu_common.h"


/*
 * C O M P I L E R  F E A T U R E S  &  D I A G N O S T I C S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef PHF_HAVE_COMPUTED_GOTOS
#define PHF_HAVE_COMPUTED_GOTOS (defined __GNUC__)
#endif

/*
 * C / C + +  V I S I B I L I T Y
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef PHF_PUBLIC
#define PHF_PUBLIC
#endif


typedef int phf_error_t;

#define PHF_HASH_MAX UINT32_MAX
#define PHF_PRIuHASH PRIu32
#define PHF_PRIxHASH PRIx32

typedef uint32_t phf_hash_t;
typedef uint32_t phf_seed_t;

typedef struct phf_string {
    void *p;
    size_t n;
} phf_string_t;

struct phf {
    size_t r; /* number of elements in g */
    size_t m; /* number of elements in perfect hash */

    size_t d_max; /* maximum displacement value in g */

    uint32_t *g; /* displacement map indexed by g(k) % r */

    const void *g_jmp;

    yu_memctx_t *mem_ctx;

    phf_seed_t seed;

    enum {
        PHF_G_UINT8_MOD_R = 1,
        PHF_G_UINT8_BAND_R,
        PHF_G_UINT16_MOD_R,
        PHF_G_UINT16_BAND_R,
        PHF_G_UINT32_MOD_R,
        PHF_G_UINT32_BAND_R,
    } g_op;

    bool nodiv;
};

PHF_PUBLIC phf_error_t phf_init(struct phf *, phf_string_t * const, size_t, size_t, size_t, phf_seed_t, bool, yu_memctx_t *);
PHF_PUBLIC void phf_free(struct phf *);

PHF_PUBLIC phf_hash_t phf_hash(struct phf *, const phf_string_t);

PHF_PUBLIC size_t phf_uniq(phf_string_t *, size_t);
PHF_PUBLIC void phf_compact(struct phf *);
