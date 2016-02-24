#pragma once

#include "yu_common.h"

/**
 * Objects are allocated in arenas. Each generation has two or more arenas, one for
 * traversable objects and one for non-traversable objects.
 *
 * Assuming 8-byte pointers:
 * Arenas are 1024 + 32 + 4096 * sizeof(boxed_value) bytes, arranged like so:
 * Traversable arena:
 *    +---------------------------------------------------+
 *    | free bitmap (512 bytes) | mark bitmap (512 bytes) |
 *    +---------------------------------------------------+
 *    | pointer to the top of the gray stack (8 bytes)    |
 *    | pointer to the rest of the gray stack (8 bytes)   |
 *    | gray stack counter (2 bytes)                      |
 *    | padding (6 bytes)                                 |
 *    +---------------------------------------------------+
 *    | pointer to arena metadata, such as the arena to   |
 *    | promote objects to the next generation (8 bytes)  |
 *    +---------------------------------------------------+
 *    |                   object space                    |
 *    |         4096 * sizeof(boxed_value) bytes          |
 *    +---------------------------------------------------+
 */

#define GC_ARENA_NUM_OBJECTS 4096
#define GC_BITMAP_SIZE (4096/8)
#define GC_ARENA_PADDING 6

struct arena;

struct arena_metadata {
    struct arena *self;
    struct arena *next;
    struct arena *next_gen;
    u8 generation;
};

struct arena {
    u64 freemap[GC_BITMAP_SIZE/8];
    u64 markmap[GC_BITMAP_SIZE/8];

    struct boxed_value *gray_top;
    struct gray_stack *gray_rest;
    u16 grayc;
    u8 padding[GC_ARENA_PADDING];

    struct arena_metadata *meta;

    struct boxed_value[GC_ARENA_NUM_OBJECTS];
};
