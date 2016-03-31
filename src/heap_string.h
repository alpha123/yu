/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"
#include "string.h"

typedef struct heap_str_t {
  heap_str_iter_fn next;
  heap_str_len_fn len;
} heap_string;
