/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#define SETUP

#define TEARDOWN

#define LIST_PLATFORM_TESTS(X) \
  X(virtual_reserve, "Reserving a huge address space should not run out of physical memory")

TEST(virtual_reserve)
  void *big;
  // Reserve 256GB just for fun
  const size_t req_sz = 256*1024*1024*1024;
  size_t usable_sz = yu_virtual_alloc(NULL, &big, req_sz, YU_VIRTUAL_RESERVE);
  PT_ASSERT_GTE(usable_sz, req_sz);
  yu_virtual_free(big, usable_sz);
END(virtual_reserve)


SUITE(platform, LIST_PLATFORM_TESTS)
