/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "value.h"

/**
 * Objects are allocated in arenas. Each generation has two or more arenas, one for
 * traversable objects and one for non-traversable objects. Objects are allocated with
 * a simple bump allocator.
 *
 * Assuming 8-byte pointers:
 * Arenas are 512 + 8 + 2048 * sizeof(boxed_value) bytes, arranged like so:
 * Traversable arena:
 *    +---------------------------------------------------+
 *    | gray queue (256 bytes)  | mark bitmap (256 bytes) |
 *    +---------------------------------------------------+
 *    |     pointer to this arena's metadata (8 bytes)    |
 *    +---------------------------------------------------+
 *    |    pointer to next object to allocate (8 bytes)   |
 *    +---------------------------------------------------+
 *    |                   object space                    |
 *    |         2048 * sizeof(boxed_value) bytes          |
 *    +---------------------------------------------------+
 */

#ifndef GC_ARENA_NUM_OBJECTS
#define GC_ARENA_NUM_OBJECTS 2048
#endif

#if (GC_ARENA_NUM_OBJECTS & (GC_ARENA_NUM_OBJECTS - 1)) != 0
#error GC_ARENA_NUM_OBJECTS must be a power of 2
#endif

#define GC_BITMAP_SIZE (GC_ARENA_NUM_OBJECTS/8)

struct arena;

struct arena_handle {
    yu_memctx_t *mem_ctx;
    struct arena * restrict self;
    struct arena_handle * restrict next;
    struct arena_handle *next_gen;
    u8 generation;
};

struct arena {
    // Indices with a bit set in this map should be considered part of the
    // gray queue.
    u64 graymap[GC_BITMAP_SIZE/sizeof(u64)];

    u64 markmap[GC_BITMAP_SIZE/sizeof(u64)];

    struct arena_handle *meta;
    struct boxed_value *next;
    struct boxed_value objs[GC_ARENA_NUM_OBJECTS];
};

struct arena_handle *arena_new(yu_memctx_t *mctx);
void arena_free(struct arena_handle *a);

struct boxed_value *arena_alloc_val(struct arena_handle *a);

u32 arena_allocated_count(struct arena_handle *a);
u32 arena_gray_count(struct arena_handle *a);

struct boxed_value *arena_pop_gray(struct arena_handle *a);
void arena_push_gray(struct arena_handle *a, struct boxed_value *v);
void arena_mark(struct arena_handle *a, struct boxed_value *v);
void arena_unmark(struct arena_handle *a, struct boxed_value *v);
bool arena_is_marked(struct arena_handle *a, struct boxed_value *v);

typedef void (* arena_move_fn)(struct boxed_value *, struct boxed_value *, void *);

void arena_promote(struct arena_handle *a, arena_move_fn move_cb, void *data);
void arena_empty(struct arena_handle *a);

struct arena_handle *arena_compact(struct arena_handle *a, arena_move_fn move_cb, void *data);
