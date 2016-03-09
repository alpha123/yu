/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "arena.h"
#include "gc.h"
#include "gss.h"
#include "value.h"

typedef struct {
    struct gc_info *gc;
    gss *s;
} yu_ctx;
