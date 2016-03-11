/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

/**
 * Copying, compacting, generational, precise, quad-color incremental updating
 * mark-and-sweep garbage collector. ^_^
 *
 * Anyone attempting to familiarize themselves with this should read Mike Pall's
 * design for the LuaJIT 3.0 GC: http://wiki.luajit.org/New-Garbage-Collector
 *
 * A brief summary is offered here:
 *
 * 1. The allocator keeps track of a black/white ‘mark bit’ by itself; this information
 *    is not kept in the object.
 * 2. The object instead keeps a gray bit in itself.
 * 3. Traversable objects (i.e. lists and tables) are initially light gray.
 * 4. When an object is marked, its mark bit is set and it is pushed onto the gray stack.
 *    At this point it is dark gray—the gray bit and the mark bit are both set.
 * 5. After the object has been traversed, its gray bit is cleared.
 * 6. If an object survives a GC cycle, both its gray and mark bits are cleared.
 * 7. It is then copied to the next generation, and the younger generations are
 *    freed en masse. The final generation must be freed one-by-one.
 * 8. If the object is then written to again, its gray bit is set but it is
 *    not pushed onto the gray stack until it is turned dark gray.
 * 9. For non-traversable objects, marking simply changes it from white to black.
 * 10. Every now and then, the oldest generation is collected with a standard sweep
 *     phase then compacted.
 *
 * It is incremental in that the top of the gray stack is popped, traversed, and then
 * execution goes back to the runtime.
 */

// Most GC constants are in arena.h

#include "arena.h"
#include "value.h"

#define gc_root_list_ptr_cmp(...) _fake()
#define gc_arena_gray_cmp(...) _fake()

YU_SPLAYTREE(root_list, struct boxed_value *, gc_root_list_ptr_cmp, true)
YU_QUICKHEAP(arena_heap, struct arena_handle *, gc_arena_gray_cmp, YU_QUICKHEAP_MAXHEAP)

#undef gc_root_list_ptr_cmp
#undef gc_arena_gray_cmp

struct gc_info {
    arena_heap a_gray; // Priority queue of arenas for looking at the next gray object.
                       // Won't necessarily be 100% accurate in terms of counts, but it
                       // should be good enough.
    root_list roots;
    struct arena_handle *arenas[GC_NUM_GENERATIONS];

    yu_memctx_t *mem_ctx;
    struct arena_handle *active_gray;
    u32 alloc_pressure_score;
};

YU_ERR_RET gc_init(struct gc_info *gc, yu_memctx_t *mctx);
void gc_free(struct gc_info *gc);

void gc_collect(struct gc_info *gc);

void gc_root(struct gc_info *gc, struct boxed_value *v);
void gc_unroot(struct gc_info *gc, struct boxed_value *v);

void gc_set_gray(struct gc_info *gc, struct boxed_value *v);

struct boxed_value *gc_next_gray(struct gc_info *gc);
void gc_scan_step(struct gc_info *gc);
