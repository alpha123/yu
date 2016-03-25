/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include <windows.h>
#include <Ntsecapi.h>

//  /¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\
// /\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/\
// \/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\/
// ×  THIS FILE IS 100% UNTESTED  ×
// /\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/\
// \/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\/
//  \_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/

// TODO Does Windows use a closed interval for pages containing addresses? Linux
// uses half-open [addr,addr+sz), but it's unclear from a cursory reading of the
// VirtualAlloc docs if Windows uses [addr,addr+sz].
hhh
size_t yu_virtual_pagesize(yu_virtual_mem_flags flags) {
  // You can tell which API is newer... ¯\(°ω°)/¯
  if (flags & YU_VIRTUAL_LARGE_PAGES)
    return GetLargePageMinimum();

  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwPageSize;
}

// Windows essentially always behaves as if YU_VIRTUAL_FIXED_ADDR was provided.
size_t yu_virtual_alloc(void **out, void *addr, size_t sz, yu_virtual_mem_flags flags) {
  assert(!(flags & YU_VIRTUAL_DECOMMIT) && !(flags & YU_VIRTUAL_RELEASE));
  assert(addr != NULL || !(flags & YU_VIRTUAL_FIXED_ADDR));

  bool large_pages = flags & YU_VIRTUAL_LARGE_PAGES;
  size_t page_sz = yu_virtual_pagesize(large_pages ? YU_VIRTUAL_LARGE_PAGES : 0) - 1, real_sz;
  void *ptr;
  int opts = large_pages ? MEM_LARGE_PAGES : 0;

  // Always round up to a multiple of the page size; this is particularly
  // important for large pages (I think Windows fails otherwise).
  real_sz = sz;
  if ((sz & page_sz) != 0)
    real_sz = (sz + page_sz) & ~page_sz;

  if ((flags & YU_VIRTUAL_RESERVE) && (flags & YU_VIRTUAL_COMMIT)) {
    if ((ptr = VirtualAlloc(addr, real_sz, MEM_RESERVE | MEM_COMMIT | opts, PAGE_READWRITE)) == NULL) {
      *out = NULL;
      return 0;
    }
    if ((flags & YU_VIRTUAL_FIXED_ADRR) && ptr != addr) {
      VirtualFree(addr, real_sz, MEM_DECOMMIT | MEM_RELEASE);
      *out = NULL;
      return 0;
    }
    *out = ptr;
    return real_sz;
  }

  if (flags & YU_VIRTUAL_RESERVE) {
    if ((ptr = VirtualAlloc(addr, real_sz, MEM_RESERVE | opts, PAGE_READWRITE)) == NULL) {
      *out = NULL;
      return 0;
    }
    if ((flags & YU_VIRTUAL_FIXED_ADDR) && ptr != addr) {
      VirtualFree(ptr, real_sz, MEM_RELEASE);
      *out = NULL;
      return 0;
    }
    *out = ptr;
    return real_sz;
  }

  assert(flags & VIRTUAL_COMMIT);
  assert(addr != NULL);

  if ((ptr = VirtualAlloc(addr, real_sz, MEM_COMMIT | opts, PAGE_READWRITE)) == NULL) {
    *out = NULL;
    return 0;
  }

  *out = ptr;
  return real_sz;
}

void yu_virtual_free(void *ptr, size_t sz, yu_virtual_mem_flags flags) {
  assert(ptr != NULL);
  assert(flags == YU_VIRTUAL_DECOMMIT || flags == YU_VIRTUAL_RELEASE || flags == (YU_VIRTUAL_DECOMMIT | YU_VIRTUAL_RELEASE));

  size_t page_sz = yu_virtual_pagesize(flags) - 1;
  if (((uintptr_t)ptr & page_sz) != 0)
    ptr = (void *)((uintptr_t)ptr & ~page_sz);
  if ((flags & YU_VIRTUAL_DECOMMIT) && (flags & YU_VIRTUAL_RELEASE))
    VirtualFree(ptr, sz, MEM_DECOMMIT | MEM_RELEASE);
  if (flags & YU_VIRTUAL_DECOMMIT)
    VirtualFree(ptr, sz, MEM_DECOMMIT);
  if (flags & YU_VIRTUAL_RELEASE)
    VirtualFree(ptr, sz, MEM_RELEASE);
}


u32 yu_sys_random(void) {
  char rand[sizeof(u32)];
  // The Windows docs say this function needs to be imported with LoadLibrary
  // and GetProcAddress, but that doesn't actually seem to be true. Also, it's
  // theoretically a semi-internal API, but it hasn't changed since XP so we're
  // probably fine. The alternative is using the awful Crypt* APIs.
  if (RtlGenRandom(rand, sizeof(u32)))
    return *(u32 *)rand;
  // I _think_ Windows always enables ASLR so the stack address of `rand` is
  // probably random-ish.
  return (u32)rand;
}
