/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

typedef enum { ROPE_NODE_BYTES, ROPE_NODE_SLICE, ROPE_NODE_STREAM, ROPE_NODE_FUNC } rope_node_type;

typedef size_t (* rope_fn)(u8 *out_bytes, size_t start, size_t end, void *data);

struct rope_node {
  union {
    struct {
      struct rope_node *ref;
      size_t start, end;
    } slice;

    struct {
      struct rope_node *left, *right, *parent;
      u8 priority;
    } join;

    struct {
      rope_fn func;
      void *udata;
    } fn;

    FILE *stream;

    struct {
      size_t len;
      u8 *bytes;
    } buf;
  } val;
  rope_node_type what;
  u8 depth;
};

typedef struct {
  sfmt_t rng;  // random numbers for treap priorities
  yu_allocator *mem_ctx;
  struct rope_node *root;
} rope;

rope *rope_new(yu_allocator *mctx);
void rope_add_node(rope *r, struct rope_node *n);
// In general u8* parameters are `restrict`; since u8 is most likely a char type
// it can alias anything unless we explicitly say not to.
void rope_cat_bytes(rope *r, const u8 * restrict bytes, size_t sz, bool take_ownership);
void rope_cat_func(rope *r, rope_fn func, void *data);
void rope_cat_stream(rope *r, FILE *stream);

rope *rope_slice(rope *r, size_t start, size_t end);

size_t rope_write(rope *r, FILE *out);
size_t rope_to_bytes(rope *r, u8 **out);
