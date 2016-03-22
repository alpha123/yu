/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#ifndef YU_INTERNAL_INCLUDES
#error "Don't include platform.h directly! Include yu_common.h instead."
#endif

/**
 * Platform detection (OS, 32/64-bit, endianness)
 *
 * Defines the following:
 *   - YU_OSAPI: ‘posix’ or ‘win32’
 *   - YU_32BIT: 0 if 64-bit, 1 if 32-bit
 *   - YU_BIG_ENDIAN: 0 if LE, 1 if BE
 */

// The platform detection code was taken more-or-less verbatim from
// https://github.com/zuiderkwast/nanbox, which in turn was adapted
// from WTF/wtf/Platform.h in WebKit.

// Note that Yu currently doesn't support big-endian platforms,
// but it's detected here anyway for the sake of completeness and
// future portability.

#define YU_OSAPI_POSIX 1
#define YU_OSAPI_WIN32 2

#if defined(_AIX) \
    || defined(__APPLE__) /* Darwin */ \
    || defined(__FreeBSD__) || defined(__DragonFly__) \
    || defined(__FreeBSD_kernel__) \
    || defined(__GNU__) /* GNU/Hurd */ \
    || defined(__linux__) \
    || defined(__NetBSD__) \
    || defined(__OpenBSD__) \
    || defined(__QNXNTO__) \
    || defined(sun) || defined(__sun) /* Solaris */ \
    || defined(unix) || defined(__unix) || defined(__unix__)
#define YU_OSAPI YU_OSAPI_POSIX
#elif defined(WIN32) || defined(_WIN32)
#define YU_OSAPI YU_OSAPI_WIN32
#endif

#ifndef YU_OSAPI
#error Unsupported operating system ¯\ ̱(°͡˷°͡) ̱/¯
#endif

#if (defined(__x86_64__) || defined(_M_X64)) \
    || (defined(__ia64__) && defined(__LP64__)) /* Itanium in LP64 mode */ \
    || defined(__alpha__) /* DEC Alpha */ \
    || (defined(__sparc__) && defined(__arch64__) || defined (__sparcv9)) /* BE */ \
    || defined(__s390x__) /* S390 64-bit (BE) */ \
    || (defined(__ppc64__) || defined(__PPC64__)) \
    || defined(__aarch64__) /* ARM 64-bit */
#define YU_32BIT 0
#else
#define YU_32BIT 1
#endif

#if YU_32BIT
#warn Please compile me in 64-bit mode! ＼(`ω′)／
#endif

// Endianness — POWER/SPARC/MIPS
// Yu currently assumes little-endian, so this is all kind of moot.
#if defined(__MIPSEB__) /* MIPS 32-bit */ \
    || defined(__ppc__) || defined(__PPC__) /* CPU(PPC) - PowerPC 32-bit */ \
    || defined(__powerpc__) || defined(__powerpc) || defined(__POWERPC__) \
    || defined(_M_PPC) || defined(__PPC) \
    || defined(__ppc64__) || defined(__PPC64__) /* PowerPC 64-bit */ \
    || defined(__sparc)   /* Sparc 32bit */  \
    || defined(__sparc__) /* Sparc 64-bit */ \
    || defined(__s390x__) /* S390 64-bit */ \
    || defined(__s390__)  /* S390 32-bit */ \
    || defined(__ARMEB__) /* ARM big endian */ \
    || ((defined(__CC_ARM) || defined(__ARMCC__)) /* ARM RealView compiler */ \
        && defined(__BIG_ENDIAN))
#define YU_BIG_ENDIAN 1
#else
#define YU_BIG_ENDIAN 0
#endif

#if YU_BIG_ENDIAN
#error Unsupported processor `(°̆︵°̆)’
#endif

/**
 * OS Interface
 *
 * Relatively thin wrapper over APIs that don't differ significantly between
 * operating systems, such as virtual memory.
 */

/**
 * RESERVE/COMMIT cannot be combined with RELEASE/DECOMMIT, but both pairs can
 * be combined with each other.
 *
 * FIXED_ADDR can be combined with reserve or commit or both. If so, it strongly
 * suggests that the allocation take place at a provided address. If that is not
 * possible, virtual_alloc() will fail.
 *
 * LARGE_PAGES suggests that the allocation should use large pages if the OS+CPU
 * combo supports it. This flag does *not* force the allocation to use large
 * pages, it is merely a hint. It may only be used in conjunction with the
 * RESERVE flag.
 */
typedef enum {
  YU_VIRTUAL_RESERVE = 1 << 1,
  YU_VIRTUAL_COMMIT = 1 << 2,
  YU_VIRTUAL_FIXED_ADDR = 1 << 3,
  YU_VIRTUAL_LARGE_PAGES = 1 << 4,

  YU_VIRTUAL_RELEASE = 1 << 5,
  YU_VIRTUAL_DECOMMIT = 1 << 6
} yu_virtual_mem_flags;

/**
 * Returns the page size in bytes used by this OS+CPU combination. If
 * YU_VIRTUAL_LARGE_PAGES is provided in `flags`, the system large page size is
 * returned. If the system supports multiple large page sizes, the result will
 * be the smallest supported size. If the system does not support large pages,
 * the result will be the same as if the LARGE_PAGES flag was not specified.
 */
size_t yu_virtual_pagesize(yu_virtual_mem_flags flags);

/**
 * Reserve and/or commit pages of virtual memory from the OS.
 *
 * If `addr` is non-NULL, it is used as a hint for where to allocate the memory.
 * If YU_VIRTUAL_FIXED_ADDR is provided in the `flags` argument, `addr` must be
 * a multiple of the page size and is taken as the exact location at which to
 * reserve memory. If that is not possible, this function fails (see failure).
 *
 * Returns:
 * The number of bytes actually reserved or committed. This will be at least the
 * requested `sz`, but may be greater if the underlying system rounds virtual
 * memory calls to page size (I think all common systems do this).
 * Additionally, *out is set to the address at which the allocation starts.
 *
 * Failure:
 * If this function fails for any reason, *out is set to NULL and 0 is returned.
 */
size_t yu_virtual_alloc(void **out, void *addr, size_t sz, yu_virtual_mem_flags flags);

/**
 * Release and/or decommit pages of virtual memory from the OS.
 *
 * If YU_VIRTUAL_RELEASE is provided in the `flags` argument, `sz` *must* be
 * equal to the size returned from yu_virtual_alloc() and `ptr` *must* be the
 * value of the out pointer from that function.
 */
void yu_virtual_free(void *ptr, size_t sz, yu_virtual_mem_flags flags);
