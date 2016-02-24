#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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

#define YU_INLINE inline __attribute__((always_inline))
#define YU_PURE __attribute__((pure))
#define YU_CONST __attribute__((const))
#define YU_RETURN_ALIGNED(n) __attribute__((assume_aligned(n)))

#elif _MSC_VER

#define YU_INLINE inline __forceinline
#define YU_PURE
#define YU_CONST
#define YU_RETURN_ALIGNED(n)

#else

#define YU_INLINE inline
#define YU_PURE
#define YU_CONST
#define YU_RETURN_ALIGNED(n)

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

/* Error handling */
#include "yu_err.h"

/* Math with a variety of types */
//#include "yu_numeric.h"

/* Data structures */
#include "yu_hashtable.h"
#include "yu_splaytree.h"
#include "yu_quickheap.h"
#include "yu_buf.h"
#include "yu_str.h"

#undef YU_INTERNAL_INCLUDES
