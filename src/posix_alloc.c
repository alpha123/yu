/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include <sys/mman.h>
#include <unistd.h>
#include "posix_alloc.h"

yu_err posix_alloc_ctx_init(posix_allocator *ctx) {
  memset(ctx, 0, sizeof(posix_allocator));
  ctx->reserve = (yu_reserve_fn)posix_reserve;
  ctx->release = (yu_release_fn)posix_release;
  ctx->commit = (yu_commit_fn)posix_commit;
  ctx->decommit = (yu_decommit_fn)posix_decommit;
}

yu_err posix_reserve(posix_allocator * YU_UNUSED(ctx), void **out, size_t num, size_t elem_size) {
  assert(out != NULL);
  assert(num > 0);
  assert(elem_size > 0);
  size_t reserve_sz = num * elem_size;
  assert(reserve_sz / elem_size == num);  // check for overflow
  void *base = mmap(NULL, reserve_sz + sizeof(size_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (base == MAP_FAILED) {
    *out = NULL;
    return YU_ERR_ALLOC_FAIL;
  }
  *((size_t *)base) = reserve_sz;
  *out = base + sizeof(size_t);
  return YU_OK;
}

yu_err posix_release(posix_allocator * YU_UNUSED(ctx), void *ptr) {
  assert(ptr != NULL);
  if (munmap(ptr, ((size_t *)ptr)[-1]) != 0)
    return YU_ERR_UNKNOWN;
  return YU_OK;
}

yu_err posix_commit(posix_allocator * YU_UNUSED(ctx), void *addr, size_t num, size_t elem_size) {
  assert(addr != NULL);
  assert(num > 0);
  assert(elem_size > 0);
  size_t page_sz = sysconf(_SC_PAGESIZE), commit_sz = num * elem_size;
  assert(commit_sz / elem_size == num);
  // Round down to nearest page boundary
  if (((uintptr_t)addr & (page_sz-1)) != 0)
    addr = (void *)((uintptr_t)addr & ~(page_sz-1));
  if (posix_madvise(addr, commit_sz, POSIX_MADV_WILLNEED) != 0)
    return YU_ERR_ALLOC_FAIL;
  return YU_OK;
}

yu_err posix_decommit(posix_allocator * YU_UNUSED(ctx), void *addr, size_t num, size_t elem_size) {
  assert(addr != NULL);
  assert(num > 0);
  assert(elem_size > 0);
  size_t page_sz = sysconf(_SC_PAGESIZE), decommit_sz = num * elem_size;
  assert(decommit_sz / elem_size == num);
  if (((uintptr_t)addr & (page_sz-1)) != 0)
    addr = (void *)((uintptr_t)addr & ~(page_sz-1));
  if (posix_madvise(addr, decommit_sz, POSIX_MADV_DONTNEED) != 0)
    return YU_ERR_UNKNOWN;
  return YU_OK;
}
