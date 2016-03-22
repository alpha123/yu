/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "internal_alloc.h"
#include "sys_alloc.h"
#include "ptest.h"

#define ASSERT_YU_STR_EQ(expr, expect) do{ \
    yu_str _actual = (expr), _hope = (expect); \
    char *_msg; \
    asprintf(&_msg, "%s yu_str_eq %s: Expected '%.*s', got '%.*s'.", \
        #expr, #expect, \
        (int)yu_str_len(_hope), _hope, \
        (int)yu_str_len(_actual), _actual); \
    pt_assert_run(yu_str_cmp(_actual,_hope)==0, _msg, __func__, __FILE__, __LINE__); \
    free(_msg); \
}while(0)

#define SUITE_NAME(name) YU_NAME(suite, name)
#define TEST_NAME(name) YU_NAME(test, name)

#define SUITE_ENUMERATE_ADD_TESTS(name, desc) \
    pt_add_test(TEST_NAME(name), desc, sname);

#define TEST_USE_SYS_ALLOC 1
#define TEST_USE_BUMP_ALLOC 2
#define TEST_USE_DEBUG_ALLOC 3

#ifndef TEST_ALLOC
#define TEST_ALLOC TEST_USE_SYS_ALLOC
#endif

#ifndef BUMP_ALLOC_SIZE
#define BUMP_ALLOC_SIZE (64 * 1024 * 1024)
#endif

#if TEST_ALLOC == TEST_USE_BUMP_ALLOC

#error Bump allocator is deprecated

#elif TEST_ALLOC == TEST_USE_DEBUG_ALLOC

#error Debug allocator not yet ported to new API

#else

#define TEST_GET_ALLOCATOR(ctx) \
  sys_allocator ctx; \
  do{ \
      yu_err _allocerr = sys_alloc_ctx_init(&ctx); \
      assert(_allocerr == YU_OK); \
  }while(0)

#define TEST_GET_INTERNAL_ALLOCATOR(ctx) \
  internal_allocator ctx; \
  do{ \
      yu_err _allocerr = internal_alloc_ctx_init(&ctx); \
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
