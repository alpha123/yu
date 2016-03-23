/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#define SETUP

#define TEARDOWN

#define LIST_PLATFORM_TESTS(X) \
  X(virtual_reserve, "Reserving a huge address space should not run out of physical memory") \
  X(virtual_commit, "Reserved pages can be committed individually")

TEST(virtual_reserve)
  void *big;
  // Reserve 256GB just for fun
  const size_t req_sz = UINT64_C(256)*1024*1024*1024;
  size_t usable_sz = yu_virtual_alloc(&big, NULL, req_sz, YU_VIRTUAL_RESERVE);
  PT_ASSERT(usable_sz > 0);
  PT_ASSERT(big != NULL);
  PT_ASSERT_GTE(usable_sz, req_sz);
  yu_virtual_free(big, usable_sz, YU_VIRTUAL_RELEASE);
END(virtual_reserve)

TEST(virtual_commit)
  void *ptr, *pg1, *pg2;
  size_t req_sz = yu_virtual_pagesize(0) * 3, committed_sz,
    usable_sz = yu_virtual_alloc(&ptr, NULL, req_sz, YU_VIRTUAL_RESERVE);
  PT_ASSERT(usable_sz > 0);
  committed_sz = yu_virtual_alloc(&pg1, ptr, 256, YU_VIRTUAL_COMMIT);
  PT_ASSERT(pg1 != NULL);
  PT_ASSERT_EQ(committed_sz, yu_virtual_pagesize(0));
  bool all_zero = true;
  for (size_t i = 0; i < committed_sz; i++) {
    if (((char *)pg1)[i] != 0) {
      all_zero = false;
      break;
    }
  }
  PT_ASSERT(all_zero);
  committed_sz = yu_virtual_alloc(&pg2, (char *)ptr + 256, yu_virtual_pagesize(0) * 2, YU_VIRTUAL_COMMIT);
  // Committing should round down to page boundary
  PT_ASSERT_EQ(pg2, pg1);
  PT_ASSERT_EQ(committed_sz, yu_virtual_pagesize(0) * 2);
  all_zero = true;
  for (size_t i = 0; i < committed_sz; i++) {
    if (((char *)pg2)[i] != 0) {
      all_zero = false;
      break;
    }
  }
  PT_ASSERT(all_zero);

  ((int *)pg1)[10] = 'y';
  ((int *)pg1)[20] = 'u';
  ((int *)pg1)[30] = 'i';

  yu_virtual_free(pg1, 10, YU_VIRTUAL_DECOMMIT);
  committed_sz = yu_virtual_alloc(&pg1, ptr, 100, YU_VIRTUAL_COMMIT);
  all_zero = true;
  for (size_t i = 0; i < committed_sz; i++) {
    if (((char *)pg1)[i] != 0) {
      all_zero = false;
      break;
    }
  }
  PT_ASSERT(all_zero);

  yu_virtual_free(ptr, usable_sz, YU_VIRTUAL_DECOMMIT | YU_VIRTUAL_RELEASE);
END(virtual_commit)


SUITE(platform, LIST_PLATFORM_TESTS)
