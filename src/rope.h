/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

#define YU_ROPE_NODE_LEAF 2
typedef enum {
  YU_ROPE_NODE_JOIN = 1,  // constant value for join type doesn't matter as long
                          // as 2nd lsb isn't set
  YU_ROPE_NODE_BYTES =  2 | YU_ROPE_NODE_LEAF,
  YU_ROPE_NODE_SLICE =  4 | YU_ROPE_NODE_LEAF,
  YU_ROPE_NODE_STREAM = 8 | YU_ROPE_NODE_LEAF,
  YU_ROPE_NODE_FUNC =  16 | YU_ROPE_NODE_LEAF
} yu_rope_node_type;

typedef size_t (* yu_rope_fn)(u8 *out_bytes, size_t start, size_t end, void *data);

struct yu_rnode {
  union {
    struct {
      struct yu_rnode *left, *right;
      u8 priority;
    } join;

    struct {
      yu_rope_fn func;
      void *udata;
    } fn;

    FILE *stream;

    u8 *bytes;  // length is stored as a size_t allocated immediately in front
                // of `bytes`.
  } val;
  yu_rope_node_type what;
};

typedef struct {
  sfmt_t rng;  // random numbers for treap priorities
  yu_allocator *mem_ctx;
  struct yu_rnode *root;
} yu_rope;

yu_rope *rope_new(yu_allocator *mctx);
void rope_add_node(yu_rope *r, struct yu_rnode *n);
// In general u8* parameters are restrict; since u8 is most likely a char type
// it can alias anything unless we explicitly say not to.
void rope_cat_bytes(yu_rope *r, const u8 * restrict bytes, size_t sz, bool take_ownership);
void rope_cat_func(yu_rope *r, yu_rope_fn func, void *data);
void rope_cat_stream(yu_rope *r, FILE *stream);

yu_rope *rope_slice(yu_rope *r, size_t start, size_t end);

size_t rope_write(yu_rope *r, FILE *out);
size_t rope_to_bytes(yu_rope *r, u8 **out);
