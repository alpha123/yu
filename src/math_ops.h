/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "value.h"
#include "gc.h"

bool value_is_numeric(value_t val);

value_t value_add(struct gc_info *gc, value_t a, value_t b);
value_t value_sub(struct gc_info *gc, value_t a, value_t b);
value_t value_mul(struct gc_info *gc, value_t a, value_t b);
value_t value_div(struct gc_info *gc, value_t a, value_t b);
