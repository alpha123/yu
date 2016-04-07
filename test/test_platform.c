/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#define SETUP

#define TEARDOWN

#define LIST_PLATFORM_TESTS(X) \
  X(virtual_reserve, "Reserving a huge address space should not run out of physical memory") \
  X(virtual_commit, "Reserved pages can be committed individually") \
  X(virtual_both, "Reserving and committing at the same time should be equivalent to sequential") \
  X(reserve_address, "virtual_alloc should attempt to obey the requested address") \
  X(reserve_fixed_address, "With the FIXED_ADDR option, virtual_alloc should reserve starting at the provided address or commit sudoku")

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

TEST(virtual_both)
  char *ptr;
  size_t page_sz = yu_virtual_pagesize(0)-1,
         usable_sz = yu_virtual_alloc((void **)&ptr, NULL, page_sz*2+2, YU_VIRTUAL_RESERVE | YU_VIRTUAL_COMMIT);
  PT_ASSERT_GTE(usable_sz, page_sz*2+2);
  ptr[0] = 42;
  ptr[page_sz+1] = 32;
  yu_virtual_free((void **)&ptr, page_sz+1, YU_VIRTUAL_DECOMMIT);
  PT_ASSERT_EQ(ptr[page_sz+1], 32);
  void *check;
  size_t check_sz = yu_virtual_alloc(&check, (void *)ptr, page_sz+1, YU_VIRTUAL_COMMIT);
  PT_ASSERT(check_sz > 0);
  PT_ASSERT_EQ(ptr[0], 0);
  PT_ASSERT_EQ(ptr[page_sz+1], 32);
  yu_virtual_free(ptr, page_sz*2+2, YU_VIRTUAL_DECOMMIT | YU_VIRTUAL_RELEASE);
END(virtual_both)

TEST(reserve_address)
  // First allocate with no particular location preference in order to get the
  // address of some free space.
  char *addr;
  size_t page_sz = yu_virtual_pagesize(0)-1,
         addr_check = yu_virtual_alloc((void **)&addr, NULL, page_sz+1, YU_VIRTUAL_RESERVE);
  assert(addr_check > 0);
  yu_virtual_free(addr, page_sz+1, YU_VIRTUAL_RELEASE);
  for (int i = 0; i < 10; i++) {
    addr += page_sz;
    if (((uintptr_t)addr & page_sz) != 0)
      addr = (char *)(((uintptr_t)addr+page_sz) & ~page_sz);

    void *ptr, *prev;
    size_t usable_sz = yu_virtual_alloc(&ptr, addr, page_sz+1, YU_VIRTUAL_RESERVE);
    PT_ASSERT(usable_sz > 0);
    prev = ptr;
    // Allow a 3 page difference in where it was actually reserved. Dunno if
    // this is a good number.
    PT_ASSERT_LT((size_t)(max((uintptr_t)addr,(uintptr_t)ptr) - min((uintptr_t)addr,(uintptr_t)ptr)), page_sz*3+3);
    // Now do it again on the same location and make sure we're still within a
    // few pages.
    usable_sz = yu_virtual_alloc(&ptr, addr, page_sz+1, YU_VIRTUAL_RESERVE);
    PT_ASSERT(usable_sz > 0);
    PT_ASSERT_LT((size_t)(max((uintptr_t)addr,(uintptr_t)ptr) - min((uintptr_t)addr,(uintptr_t)ptr)), page_sz*3+3);
    yu_virtual_free(ptr, page_sz+1, YU_VIRTUAL_RELEASE);
    yu_virtual_free(prev, page_sz+1, YU_VIRTUAL_RELEASE);
  }
END(reserve_address)

TEST(reserve_fixed_address)
  char *addr;
  size_t page_sz = yu_virtual_pagesize(0)-1,
         addr_check = yu_virtual_alloc((void **)&addr, NULL, page_sz+1, YU_VIRTUAL_RESERVE);
  assert(addr_check > 0);
  yu_virtual_free(addr, page_sz+1, YU_VIRTUAL_RELEASE);
  for (int i = 0; i < 10; i++) {
    addr += page_sz;
    if (((uintptr_t)addr & page_sz) != 0)
      addr = (char *)(((uintptr_t)addr+page_sz) & ~page_sz);

    char *ptr;
    size_t usable_sz = yu_virtual_alloc((void **)&ptr, addr, page_sz+1, YU_VIRTUAL_RESERVE | YU_VIRTUAL_FIXED_ADDR);
    PT_ASSERT(usable_sz == 0 || ptr == addr);
  }

  // Reserve an illegal address so we can be sure to have a failure
  addr = (char *)0x01;
  void *ptr;
  size_t usable_sz = yu_virtual_alloc(&ptr, addr, page_sz+1, YU_VIRTUAL_RESERVE | YU_VIRTUAL_FIXED_ADDR);
  PT_ASSERT_EQ(usable_sz, 0u);
  PT_ASSERT_EQ(ptr, NULL);
END(reserve_fixed_address)


SUITE(platform, LIST_PLATFORM_TESTS)
