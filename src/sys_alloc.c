/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "yu_common.h"
#include "internal_alloc.h"
#include "sys_alloc.h"

#define addrhash_1(x) ((uintptr_t)(x))
#define addrhash_2(x) (~(uintptr_t)(x))
#define addr_eq(x,y) ((x)==(y))

YU_HASHTABLE_IMPL(sysmem_tbl, void *, size_t, addrhash_1, addrhash_2, addr_eq)

#undef addr_eq
#undef addrhash_1
#undef addrhash_2

yu_err sys_alloc_ctx_init(sys_allocator *ctx) {
  if (internal_alloc_ctx_init(&ctx->tbl_mctx) != YU_OK)
    return YU_ERR_ALLOC_FAIL;

  sysmem_tbl_init(&ctx->allocd, 2000, &ctx->tbl_mctx);

  ctx->base.alloc = (yu_alloc_fn)sys_alloc;
  ctx->base.realloc = (yu_realloc_fn)sys_realloc;
  ctx->base.free = (yu_free_fn)sys_free;
  ctx->base.free_ctx = (yu_ctx_free_fn)sys_alloc_ctx_free;
  ctx->base.allocated_size = (yu_allocated_size_fn)sys_allocated_size;
  ctx->base.usable_size = (yu_usable_size_fn)sys_usable_size;

  return YU_OK;
}

static
u32 free_ctx_all(void *address, size_t YU_UNUSED(sz), void * YU_UNUSED(data)) {
  free(address);
  return 0;
}

void sys_alloc_ctx_free(sys_allocator *ctx) {
  sysmem_tbl_iter(&ctx->allocd, free_ctx_all, NULL);
  sysmem_tbl_free(&ctx->allocd);
  yu_alloc_ctx_free(&ctx->tbl_mctx);
}

yu_err sys_alloc(sys_allocator *ctx, void **out, size_t num, size_t elem_size, size_t alignment) {
  size_t alloc_sz = num * elem_size;

  // Default system alignment requested — we can return calloc directly.
  if (alignment == 0) {
    if ((*out = calloc(num, elem_size)) == NULL)
	    return YU_ERR_ALLOC_FAIL;
    goto ok;
  }

  // Otherwise use aligned_alloc and memset, which have a few invariants:

  // Alignment must be a power of 2.
  assert((alignment & (alignment - 1)) == 0);

  // Total allocated bytes must be a multiple of the alignment.
  if ((alloc_sz & (alignment - 1)) != 0)
    alloc_sz = (alloc_sz + alignment - 1) & ~(alignment - 1);

  if ((*out = aligned_alloc(alignment, alloc_sz)) == NULL)
    return YU_ERR_ALLOC_FAIL;
  memset(*out, 0, alloc_sz);

  // Now we have to store the original size so that we can reallocate properly.
  // Use a hash table keyed by address. This is also used to clean up a context.
  ok:
  sysmem_tbl_put(&ctx->allocd, *out, alloc_sz, NULL);
  return YU_OK;
}

yu_err sys_realloc(sys_allocator *ctx, void **ptr, size_t num, size_t elem_size, size_t alignment) {
  // It's slightly unclear if pointers from aligned_alloc can safely be reallocated with
  // realloc. The POSIX standard doesn't mention it, but Linux manpages say it's okay.
  // Unsure about the ISO C11 standard.
  //
  // For now, sys_realloc to a alignment of 0 on a pointer that was originally aligned
  // with a non-0 argument to sys_alloc *MAY FAIL CATASTROPHICALLY AND WITHOUT WARNING*.
  // Fortunately, Yu never does that. ;-)
  // But this code is technically possibly broken.

  size_t old_sz, alloc_sz = num * elem_size;
  bool we_own_this = sysmem_tbl_remove(&ctx->allocd, *ptr, &old_sz);
  assert(we_own_this);

  void *safety_first;
  if (alignment == 0) {
    if ((safety_first = realloc(*ptr, alloc_sz)) == NULL)
	    return YU_ERR_ALLOC_FAIL;
    goto ok;
  }

  // aligned_alloc's invariants, as above
  assert((alignment & (alignment - 1)) == 0);

  if ((alloc_sz & (alignment - 1)) != 0)
    alloc_sz = (alloc_sz + alignment - 1) & ~(alignment - 1);

  if ((safety_first = aligned_alloc(alignment, alloc_sz)) == NULL)
    return YU_ERR_ALLOC_FAIL;

  memcpy(safety_first, *ptr, old_sz);
  free(*ptr);

  ok:
  // 0 out the newly allocated portion
  memset((u8 *)safety_first + old_sz, 0, alloc_sz - old_sz);
  *ptr = safety_first;
  sysmem_tbl_put(&ctx->allocd, *ptr, alloc_sz, NULL);
  return YU_OK;
}

void sys_free(sys_allocator *ctx, void *ptr) {
  sysmem_tbl_remove(&ctx->allocd, ptr, NULL);
  free(ptr);
}

size_t sys_allocated_size(sys_allocator *ctx, void *ptr) {
  bool ok;
  size_t sz;
  ok = sysmem_tbl_get(&ctx->allocd, ptr, &sz);
  assert(ok);
  return sz;
}

size_t sys_usable_size(sys_allocator *ctx, void *ptr) {
  // Since we're mostly just wrapping malloc, we actually have no idea what the
  // usable space of this thing is.
  return sys_allocated_size(ctx, ptr);
}
