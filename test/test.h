/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "bump_alloc.h"
#include "debug_alloc.h"
#include "internal_alloc.h"
#include "sys_alloc.h"
#include "ptest.h"

#define SUITE_NAME(name) YU_NAME(suite, name)
#define TEST_NAME(name) YU_NAME(test, name)

#define SUITE_ENUMERATE_ADD_TESTS(name, desc) \
    pt_add_test(TEST_NAME(name), desc, sname);

#define TEST_USE_SYS_ALLOC 1
#define TEST_USE_INTERNAL_ALLOC 2
#define TEST_USE_BUMP_ALLOC 3
#define TEST_USE_DEBUG_ALLOC 4

#ifndef TEST_ALLOC
#define TEST_ALLOC TEST_USE_SYS_ALLOC
#endif

#ifndef BUMP_ALLOC_SIZE
#define BUMP_ALLOC_SIZE (64 * 1024 * 1024)
#endif

#if TEST_ALLOC == TEST_USE_BUMP_ALLOC

#define TEST_GET_ALLOCATOR(ctx) do{ \
    yu_err _allocerr = bump_alloc_ctx_init((ctx), BUMP_ALLOC_SIZE); \
    assert(_allocerr == YU_OK); \
}while(0)

#define TEST_GET_INTERNAL_ALLOCATOR(ctx) do{ \
    yu_err _allocerr = bump_alloc_ctx_init((ctx), BUMP_ALLOC_SIZE); \
    assert(_allocerr == YU_OK); \
}while(0)

#elif TEST_ALLOC == TEST_USE_DEBUG_ALLOC

#define TEST_GET_ALLOCATOR(ctx) do{ \
    yu_err _allocerr = debug_alloc_ctx_init((ctx)); \
    assert(_allocerr == YU_OK); \
}while(0)

#define TEST_GET_INTERNAL_ALLOCATOR(ctx) do{ \
    yu_err _allocerr = debug_alloc_ctx_init((ctx)); \
    assert(_allocerr == YU_OK); \
}while(0)

#else

#define TEST_GET_ALLOCATOR(ctx) do{ \
    yu_err _allocerr = sys_alloc_ctx_init((ctx)); \
    assert(_allocerr == YU_OK); \
}while(0)

#define TEST_GET_INTERNAL_ALLOCATOR(ctx) do{ \
    yu_err _allocerr = internal_alloc_ctx_init((ctx)); \
    assert(_allocerr == YU_OK); \
}while(0)

#endif

/* See http://stackoverflow.com/a/4152185 for what this is doing.
 * In addition to __section__ we also need __attribute__(used) or
 * the suites generally get compiled out (they're never referenced
 * explicitly.
 * See also: test/test.ld (the linker script that makes this all work).
 * Note: We also have to insert some padding (done in the linker script)
 *       or GDB refuses to accept the file as executable even though it
 *       is a fully valid ELF file.
 * TODO: Test suite doesn't need to run on Windows, does it? :(
 */
#define SUITE(name, list) \
    void SUITE_NAME(name)(void) { \
        const char *sname = #name; \
        list(SUITE_ENUMERATE_ADD_TESTS) \
    } \
    static void (*EXPAND(DEFER(PASTE)(registered, SUITE_NAME(name))))(void) \
        __attribute__((used,__section__(".suite_registry"))) = SUITE_NAME(name);

#define TEST(name) static void TEST_NAME(name)(void) { SETUP
#define END(name) TEARDOWN }

extern void (*suite_registry_begin[])(void);
extern void (*suite_registry_end[])(void);
