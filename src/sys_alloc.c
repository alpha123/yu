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

YU_HASHTABLE_IMPL(sysmem_pgtbl, void *, size_t, addrhash_1, addrhash_2, addr_eq)

#undef addr_eq
#undef addrhash_1
#undef addrhash_2

yu_err sys_alloc_ctx_init(sys_allocator *ctx) {
  if (internal_alloc_ctx_init(&ctx->tbl_mctx) != YU_OK)
    return YU_ERR_ALLOC_FAIL;

  sysmem_tbl_init(&ctx->allocd, 2000, &ctx->tbl_mctx);
  sysmem_pgtbl_init(&ctx->pgs, 20, &ctx->tbl_mctx);

  ctx->base.alloc = (yu_alloc_fn)sys_alloc;
  ctx->base.realloc = (yu_realloc_fn)sys_realloc;
  ctx->base.free = (yu_free_fn)sys_free;
  ctx->base.free_ctx = (yu_ctx_free_fn)sys_alloc_ctx_free;
  ctx->base.allocated_size = (yu_allocated_size_fn)sys_allocated_size;
  ctx->base.usable_size = (yu_usable_size_fn)sys_usable_size;
  ctx->base.reserve = (yu_reserve_fn)sys_reserve;
  ctx->base.release = (yu_release_fn)sys_release;
  ctx->base.commit = (yu_commit_fn)sys_commit;
  ctx->base.decommit = (yu_decommit_fn)sys_decommit;

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

void sys_alloc_ctx_free(sys_allocator *ctx) {
  sysmem_tbl_iter(&ctx->allocd, free_ctx_all, NULL);
  sysmem_tbl_free(&ctx->allocd);
  sysmem_pgtbl_iter(&ctx->pgs, release_ctx_all, (void *)(yu_virtual_pagesize(0)-1));
  sysmem_pgtbl_free(&ctx->pgs);
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
  // Avoid checking two hash tables unless `ptr` looks like it could be a page
  size_t pgsz = yu_virtual_pagesize(0)-1, sz, free_sz;
  if (((uintptr_t)ptr & pgsz) == 0) {
    if (sysmem_pgtbl_remove(&ctx->pgs, ptr, &sz)) {
      free_sz = (sz & pgsz) == 0 ? sz : (sz+pgsz) & ~pgsz;
      yu_virtual_free(ptr, free_sz, YU_VIRTUAL_DECOMMIT | YU_VIRTUAL_RELEASE);
      return;
    }
  }
  sysmem_tbl_remove(&ctx->allocd, ptr, NULL);
  free(ptr);
}

size_t sys_allocated_size(sys_allocator *ctx, void *ptr) {
  bool ok;
  size_t sz;

  if (((uintptr_t)ptr & (yu_virtual_pagesize(0)-1)) == 0) {
    if (sysmem_pgtbl_get(&ctx->pgs, ptr, &sz))
      return sz;
  }

  ok = sysmem_tbl_get(&ctx->allocd, ptr, &sz);
  assert(ok);
  return sz;
}

size_t sys_usable_size(sys_allocator *ctx, void *ptr) {
  // We do keep track of usable page sizes, or at least since we always reserve
  // a multiple of the page size we can calculate it from the requested size.
  size_t pgsz = yu_virtual_pagesize(0)-1, sz;
  if (((uintptr_t)ptr & pgsz) == 0) {
    if (sysmem_pgtbl_get(&ctx->pgs, ptr, &sz))
      return (sz & pgsz) == 0 ? sz : (sz+pgsz) & ~pgsz;
  }

  // Since we're mostly just wrapping malloc, we actually have no idea what the
  // usable space of this thing is.
  return sys_allocated_size(ctx, ptr);
}

yu_err sys_reserve(sys_allocator *ctx, void **out, size_t num, size_t elem_size) {
  void *pg;
  size_t pgsz = yu_virtual_pagesize(0)-1, alloc_sz = num * elem_size, usable_sz;
  if ((alloc_sz & pgsz) != 0)
    alloc_sz = (alloc_sz+pgsz) & ~pgsz;
  usable_sz = yu_virtual_alloc(&pg, NULL, alloc_sz, YU_VIRTUAL_RESERVE);
  if (usable_sz == 0)
    return YU_ERR_ALLOC_FAIL;
  // TODO since we round up to a multiple of the page size, usable_sz should
  // never differ from alloc_sz. However, this makes some assumptions about
  // yu_virtual_alloc() and isn't defined in its contract. The reason we do it
  // here is to keep track of only one size (the requested one), but we need to
  // make sure that we pass the usable size to yu_virtual_free().
  assert(usable_sz == alloc_sz);
  *out = pg;
  // Doesn't actually matter if this was already reserved
  sysmem_pgtbl_put(&ctx->pgs, pg, num * elem_size, NULL);
  return YU_OK;
}

yu_err sys_commit(sys_allocator * YU_UNUSED(ctx), void *ptr, size_t num, size_t elem_size) {
  if (yu_virtual_alloc(&ptr, ptr, num * elem_size, YU_VIRTUAL_COMMIT) == 0)
    return YU_ERR_ALLOC_FAIL;
  return YU_OK;
}

yu_err sys_release(sys_allocator *ctx, void *ptr) {
  size_t pgsz = yu_virtual_pagesize(0)-1, sz, free_sz;
  bool ok = sysmem_pgtbl_get(&ctx->pgs, ptr, &sz);
  assert(ok);
  free_sz = (sz & pgsz) == 0 ? sz : (sz+pgsz) & ~pgsz;
  yu_virtual_free(ptr, free_sz, YU_VIRTUAL_RELEASE);
  return YU_OK;
}

yu_err sys_decommit(sys_allocator * YU_UNUSED(ctx), void *ptr, size_t num, size_t elem_size) {
  // Decommitting both here and in context close is fine because it's fine to
  // decommit a page twice (this is explicitly stated in Win32 and POSIX docs).
  yu_virtual_free(ptr, num * elem_size, YU_VIRTUAL_DECOMMIT);
  return YU_OK;
}
