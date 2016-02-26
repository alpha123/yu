#pragma once
#include "yu_common.h"
#include "ptest.h"

#define SUITE_NAME(name) YU_NAME(suite, name)
#define TEST_NAME(name) YU_NAME(test, name)

#define SUITE_ENUMERATE_ADD_TESTS(name, desc) \
    pt_add_test(TEST_NAME(name), desc, sname);

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
