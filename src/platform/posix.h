/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// platform/linux.h redefines these
#ifndef MEMADVISE
#define MEMADVISE posix_madvise
#endif

#ifndef MEMADVISE_COMMIT
#define MEMADVISE_COMMIT POSIX_MADV_WILLNEED
#endif

#ifndef MEMADVISE_DECOMMIT
#define MEMADVISE_DECOMMIT POSIX_MADV_DONTNEED
#endif

size_t yu_virtual_pagesize(yu_virtual_mem_flags flags) {
  if (flags & YU_VIRTUAL_LARGE_PAGES) {
    // TODO return large page size
  }
#ifdef _SC_PAGESIZE
  return sysconf(_SC_PAGESIZE);
#else
  return sysconf(_SC_PAGE_SIZE);
#endif
}

// TODO support large pages
// At the moment the YU_VIRTUAL_LARGE_PAGES flag is ignored.
// Is there even a portable POSIX API for large pages?
size_t yu_virtual_alloc(void **out, void *addr, size_t sz, yu_virtual_mem_flags flags) {
  assert(!(flags & YU_VIRTUAL_DECOMMIT) && !(flags & YU_VIRTUAL_RELEASE));
  // If FIXED_ADDR is provided, addr can't be NULL
  assert(addr != NULL || !(flags & YU_VIRTUAL_FIXED_ADDR));

  size_t page_sz = yu_virtual_pagesize(0) - 1, real_sz;
  void *ptr;
  bool fresh_reserve = false;
  if (flags & YU_VIRTUAL_RESERVE) {
    int opts = 0;
    if (flags & YU_VIRTUAL_FIXED_ADDR) {
      opts = MAP_FIXED;
      // Mapping a fixed address requires it to be on a page boundary
      if (((uintptr_t)addr & page_sz) != 0)
        addr = (void *)((uintptr_t)addr & ~page_sz);
    }
    // Always reserve a multiple of the page size; I think all OSes do this
    // anyway but be explicit about it.
    real_sz = (sz + page_sz) & ~page_sz;
    ptr = mmap(addr, real_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | opts, -1, 0);
    if (ptr == MAP_FAILED) {
      *out = NULL;
      return 0;
    }
    if ((flags & YU_VIRTUAL_FIXED_ADDR) && ptr != addr) {
      munmap(ptr, real_sz);
      *out = NULL;
      return 0;
    }
    fresh_reserve = true;
  }
  else {
    ptr = addr;
    real_sz = (sz + page_sz) & ~page_sz;
  }

  if (flags & YU_VIRTUAL_COMMIT) {
    assert(ptr != NULL);
    if (((uintptr_t)ptr & page_sz) != 0)
      ptr = (void *)((uintptr_t)ptr & ~page_sz);
    if (MEMADVISE(ptr, real_sz, MEMADVISE_COMMIT) != 0) {
      if (fresh_reserve)
        munmap(ptr, real_sz);
      *out = NULL;
      return 0;
    }
    // Ensure that committed memory is zeroed-out. Matches the behavior of
    // Windows VirtualAlloc, which I think is better behavior. Windows' virtual
    // memory API in general is a little nicer than *nix. Note that memory that
    // was just reserved with mmap() will already be zero.
    if (!fresh_reserve)
      memset(ptr, 0, real_sz);
  }

  *out = ptr;
  return real_sz;
}

void yu_virtual_free(void *ptr, size_t sz, yu_virtual_mem_flags flags) {
  assert(ptr != NULL);
  assert(flags == YU_VIRTUAL_DECOMMIT || flags == YU_VIRTUAL_RELEASE || flags == (YU_VIRTUAL_DECOMMIT | YU_VIRTUAL_RELEASE));
  size_t page_sz = yu_virtual_pagesize(0) - 1;
  // Round down to nearest page boundary
  if (((uintptr_t)ptr & page_sz) != 0)
    ptr = (void *)((uintptr_t)ptr & ~page_sz);
  if (flags & YU_VIRTUAL_DECOMMIT)
    MEMADVISE(ptr, sz, MEMADVISE_DECOMMIT);
  if (flags & YU_VIRTUAL_RELEASE)
    munmap(ptr, sz);
}


// Linux platform implementation uses its own implementation based on
// getrandom() for ≥3.19 kernels.
#ifndef USING_GETRANDOM
u32 yu_sys_random(void) {
  int rng_fd = open("/dev/urandom", O_RDONLY);
  if (rng_fd == -1)
    goto todo_fixme;
  char rand[sizeof(u32)];
  size_t progress = 0;
  // We will probabably always be able to read 4 bytes at once, but
  // theoretically /dev/urandom could return 1-3, hence the loop.
  while (progress < sizeof rand) {
    ssize_t read_sz = read(rng_fd, rand + progress, sizeof rand - progress);
    if (read_sz < 0)
      goto todo_fixme;
    progress += read_sz;
  }
  close(rng_fd);
  return *(u32 *)rand;

  // TODO figure out a better way to cope with /dev/urandom failing
  todo_fixme:
  if (rng_fd > -1) close(rng_fd);
  // Why does this seem like a terrible idea
  return *(u32 *)((uintptr_t)&rng_fd-sizeof(u64)*2);
}
#endif
