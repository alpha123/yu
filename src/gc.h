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
 *    freed en masse. The final generation is freed one-by-one and compacted.
 * 8. If the object is then written to again (the "write barrier"):
 *      - If the object is already light/dark gray, nothing happens. This is the
 *        common case and the write barrier is essentially avoided entirely.
 *      - If the object has already been marked (is black) it is reverted to dark gray
 *        and rescanned.
 *      - If the object is still white, then it simply gets set to light gray.
 * 9. For non-traversable objects, marking simply changes it from white to black.
 * 10. Every now and then, the oldest generation is collected with a standard sweep
 *     phase then compacted.
 *
 * It is incremental in that the top of the gray stack is popped, traversed, and then
 * execution goes back to the runtime.
 */

// Other GC constants are in arena.h

// Number of separate object heaps to manage
#ifndef GC_NUM_GENERATIONS
#define GC_NUM_GENERATIONS 3
#endif

// Number of objects to pop off the gray stack and mark during a GC pause.
#ifndef GC_INCREMENTAL_STEP_COUNT
#define GC_INCREMENTAL_STEP_COUNT 10
#endif

#include "arena.h"
#include "value.h"

YU_SPLAYTREE(root_list, value_handle, root_list_ptr_cmp, true)
YU_QUICKHEAP(arena_heap, struct arena_handle *, arena_gray_cmp, YU_QUICKHEAP_MAXHEAP)

struct gc_handle_set {
    struct boxed_value *handles[1023];
    struct gc_handle_set *next;
};

struct gc_info {
    // Priority queue of arenas for looking at the next gray object.
    // Won't necessarily be 100% accurate in terms of counts, but it
    // should be good enough.
    arena_heap a_gray;

    // Since this is a copying GC and values move around, we need to
    // keep track of them.
    struct gc_handle_set *hs;

    // A splay tree happens to be a good data structure for storing
    // roots. We don't want duplicates and roots are frequently
    // removed in LIFO order (as the stack is the primary provider
    // of root values). With a splay tree, removing roots will be
    // amortized constant time, which is better than using a skiplist
    // since we don't care about in-order iteration.
    root_list roots;

    struct arena_handle *arenas[GC_NUM_GENERATIONS];

    yu_allocator *mem_ctx;
    struct arena_handle *active_gray; // Popped off the gray priority heap

    u8 collecting_generation;
};

YU_ERR_RET gc_init(struct gc_info *gc, yu_allocator *mctx);
void gc_free(struct gc_info *gc);

value_handle gc_make_handle(struct gc_info *gc, struct boxed_value *v);
value_handle gc_alloc_val(struct gc_info *gc, value_type type);

void gc_root(struct gc_info *gc, value_handle v);
void gc_unroot(struct gc_info *gc, value_handle v);

void gc_barrier(struct gc_info *gc, value_handle v);

void gc_set_gray(struct gc_info *gc, struct boxed_value *v);
void gc_mark(struct gc_info *gc, struct boxed_value *v);

struct boxed_value *gc_next_gray(struct gc_info *gc);
bool gc_scan_step(struct gc_info *gc);

void gc_sweep(struct gc_info *gc);
void gc_full_collect(struct gc_info *gc);
