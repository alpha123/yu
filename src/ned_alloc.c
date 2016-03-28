/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "ned_alloc.h"

yu_err ned_alloc_ctx_init(ned_allocator *ctx, size_t init_capacity) {
  ctx->base.alloc = (yu_alloc_fn)ned_alloc;
  ctx->base.realloc = (yu_realloc_fn)ned_realloc;
  ctx->base.free = (yu_free_fn)ned_free;
  ctx->base.free_ctx = (yu_ctx_free_fn)ned_alloc_ctx_free;
  ctx->base.allocated_size = (yu_allocated_size_fn)ned_allocated_size;
  ctx->base.usable_size = (yu_usable_size_fn)ned_usable_size;
  ctx->base.reserve = (yu_reserve_fn)ned_reserve;
  ctx->base.release = (yu_release_fn)ned_release;
  ctx->base.commit = (yu_commit_fn)ned_commit;
  ctx->base.decommit = (yu_decommit_fn)ned_decommit;

  if ((ctx->pool = nedcreatepool(0, 1)) == NULL)
    return YU_ERR_ALLOC_FAIL;

  return YU_OK;
}

static
u32 free_ctx_all(void *address, size_t YU_UNUSED(sz), void * YU_UNUSED(data)) {
  free(address);
  return 0;
}

static
u32 release_ctx_all(void *page, size_t sz, void *data) {
  size_t pgsz = (size_t)data, free_sz;
  free_sz = (sz & pgsz) == 0 ? sz : (sz+pgsz) & ~pgsz;
  yu_virtual_free(page, free_sz, YU_VIRTUAL_DECOMMIT | YU_VIRTUAL_RELEASE);
  return 0;
}

void ned_alloc_ctx_free(ned_allocator *ctx) {
  neddestroypool(ctx->pool);
}

yu_err ned_alloc(ned_allocator *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
  void *ptr = nedpmalloc2(ctx->pool, num * elem_size, alignment, M2_ZERO_MEMORY);
  if (ptr == NULL) {
    *out = NULL;
    return YU_ERR_ALLOC_FAIL;
  }
  *out = ptr;
  return YU_OK;
}

yu_err ned_realloc(ned_allocator *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
  void *safety_first = *ptr, *newptr = nedprealloc2(ctx->pool, *ptr, num * elem_size, alignment,
                                                    M2_ZERO_MEMORY | M2_RESERVE_MULT(8) | NM_SKIP_TOLERANCE_CHECKS);
  if (newptr == NULL) {
    *ptr = safety_first;
    return YU_ERR_ALLOC_FAIL;
  }
  *ptr = newptr;
  return YU_OK;
}

void ned_free(ned_allocator *ctx, void *ptr) {
  nedpfree2(ctx->pool, ptr, NM_SKIP_TOLERANCE_CHECKS);
}

struct malloc_chunk {
  size_t prev_foot, head;
  struct malloc_chunk *fd, *bk;
};

#define PINUSE_BIT          ((size_t)1)
#define CINUSE_BIT          ((size_t)2)
#define FLAG4_BIT           ((size_t)4)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)
#define FLAG_BITS           (PINUSE_BIT|CINUSE_BIT|FLAG4_BIT)
#define mem2chunk(mem)      ((struct malloc_chunk *)((char*)(mem) - sizeof(size_t) * 2))
#define chunksize(p)        ((p)->head & ~FLAG_BITS)

size_t ned_allocated_size(ned_allocator *ctx, void *ptr) {
  return chunksize(mem2chunk(ptr));
}

size_t ned_usable_size(ned_allocator * YU_UNUSED(ctx), void *ptr) {
  return nedmemsize(ptr);
}

yu_err ned_reserve(ned_allocator *ctx, void **out, size_t num, size_t elem_size) {
  return YU_ERR_ALLOC_FAIL;
}

yu_err ned_commit(ned_allocator * YU_UNUSED(ctx), void *ptr, size_t num, size_t elem_size) {
  return YU_ERR_ALLOC_FAIL;
}

yu_err ned_release(ned_allocator *ctx, void *ptr) {
  return YU_ERR_ALLOC_FAIL;
}

yu_err ned_decommit(ned_allocator * YU_UNUSED(ctx), void *ptr, size_t num, size_t elem_size) {
  return YU_ERR_ALLOC_FAIL;
}
