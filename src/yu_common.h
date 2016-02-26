#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/**
 * A few things to keep in mind —
 *
 * 1. yu_common and associated files make _heavy_ use of the C preprocessor.
 *    Turn back now if you don't like cpp.
 * 2. You should be familiar with X macros <https://en.wikipedia.org/wiki/X_Macro>.
 * 3. You should probably sacrifice a young goat to Cthulhu before digging
 *    around in too much of the higher-order macro stuff.
 */

/* Integer types */

typedef   int8_t  s8;
typedef  uint8_t  u8;
typedef  int16_t s16;
typedef uint16_t u16;
typedef  int32_t s32;
typedef uint32_t u32;
typedef  int64_t s64;
typedef uint64_t u64;

/* Compiler-specific directives */
#ifdef __GNUC__

#define YU_UNUSED(x) x __attribute__((unused))

#define YU_INLINE inline __attribute__((always_inline))
#define YU_PURE __attribute__((pure))
#define YU_CONST __attribute__((const))
#define YU_RETURN_ALIGNED(n) __attribute__((assume_aligned(n)))
#define YU_MALLOC_LIKE __attribute__((malloc))

#elif _MSC_VER

// Thanks to http://stackoverflow.com/a/9602511 for the warning number for MSVC
#define YU_UNUSED(x) __pragma(warning(surpress:4100)) x

#define YU_INLINE inline __forceinline
#define YU_PURE
#define YU_CONST
#define YU_RETURN_ALIGNED(n)
#define YU_MALLOC_LIKE

#else

#define YU_INLINE inline
#define YU_PURE
#define YU_CONST
#define YU_RETURN_ALIGNED(n)
#define YU_MALLOC_LIKE

#endif

#define YU_INTERNAL_INCLUDES

/** Common functions **/

/* Math */

u32 yu_ceil_log2(u64 n);

/* Memory */

// *BSD puts alloca in stdlib.h (and doesn't have alloca.h)
// while other *nixes don't (OS X has it in both).
// Windows puts it in malloc.h and calls it _alloca.
#if defined(__unix__)
#include <sys/param.h>
#ifndef BSD
#include <alloca.h>
#endif
#elif defined(_WIN32)
#include <malloc.h>
#define alloca _alloca
#endif

/** Common macros **/

/* Macro manipulating macros */
#define PASTE(x, y) x ## y
#define CAT(x, y) PASTE(x, y)
#define NOTHING()
#define DEFER(x) x NOTHING()
#define EXPAND(...) __VA_ARGS__

/* Safe min/max */
#define min(a,b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b; \
})
#define max(a,b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _b : _a; \
})

/* For namespacing various types of tables, trees, lists, etc */
#define YU_NAME(ns, name) EXPAND(DEFER(PASTE)(PASTE(ns, _), name))

/* Define enums with printable string names using X-macros */
#define DEF_ENUM_MEMBER(name, ...) name,
#define DEF_ENUM_NAME_CASE(name, ...) case name: return #name;
#define DEF_ENUM(typename, XX) \
    typedef enum { XX(DEF_ENUM_MEMBER) } typename; \
    inline const char *YU_NAME(typename, name)(typename x) { \
        switch (x) { \
        XX(DEF_ENUM_NAME_CASE) \
        default: return "unknown"; \
        } \
    }

/* Assertions */
#include <assert.h>

/** External library includes **/
#include <gmp.h>
#include <mpfr.h>
#include "SFMT/SFMT.h"

/** Core common Yu functions **/

/**
 * ERROR HANDLING
 *
 * There are quite a lot of macros defined in yu_err. Most of it is not
 * particularly important to understanding the rest of the source, but
 * read it for more details.
 * The gist of it is just a wrapper for code like
 *     `if ((yu_local_err = expr) != YU_OK) goto error;`
 */
#include "yu_err.h"

/* Math with a variety of types */
//#include "yu_numeric.h"

/**
 * DATA STRUCTURES
 *
 * A small collection of useful, cache-friendly[1] data structures.
 *
 * These are implemented as giant macros that sort of emulate C++ templates.
 * They come in pairs: YU_DATASTRUCTURE(...) AND YU_DATASTRUCTURE_IMPL(...)
 * where the former defines the structs and prototypes and the latter
 * defines the implementation. The arguments are always the same to the both
 * of them.
 *
 * Yes, this does make debugging them something of a pain. In general, compiling
 * with -g3 -gdwarf-4 eases the pain slightly. Otherwise, try the ‘debug_templates’
 * rule in the Makefile.
 *
 *
 * 1. splaytree is not particularly cache friendly. I'm working on a cache-oblivious
 *    layout for it.
 */

#include "yu_hashtable.h"
#include "yu_splaytree.h"
#include "yu_quickheap.h"


/**
 * DYNAMIC BUFFERS
 *
 * Heap-allocated buffers implemented with ‘fat pointers’.
 *
 * These keep some metadata in front of the memory that they allocate,
 * for things like length, buffer capacity, etc. The memory layout looks
 * like so:
 *
 *     +-----------------------------------------------+
 *     | ... | struct yu_buf_dat | buffer contents ... |
 *     +-----------------------------------------------+
 *                               ^^^^^^^^^^^^^^^^^^^^^^^
 *                               returned yu_buf pointer
 *
 * Where the first section is some garbage data in case the pointer we
 * got from malloc isn't 16-byte aligned.
 *
 * yu_str is slightly more interesting, in that it's fully Unicode-aware,
 * including the Extended Grapheme Cluster.
 */

#include "yu_buf.h"
#include "yu_str.h"

#undef YU_INTERNAL_INCLUDES
