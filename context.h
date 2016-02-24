#pragma once

#include "yu_common.h"
#include "value.h"
#include "gc.h"
#include "gss.h"

typedef struct {
    struct gc_info *gc;
    gss *s;
} yu_ctx;
