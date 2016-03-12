/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "gc.h"
#include "value.h"

typedef struct {
    struct gc_info *gc;
} yu_ctx;

void yu_tuple_append(yu_ctx *ctx, value_t tup, value_t value);
