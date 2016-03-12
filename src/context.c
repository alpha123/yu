
/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "context.h"

void yu_tuple_append(yu_ctx *ctx, value_t tup, value_t value) {
    struct boxed_value *val = value_to_ptr(tup);
    assert(boxed_value_get_type(val) == VALUE_TUPLE);
    gc_barrier(ctx->gc, val);

    u8 i = 0;
    value_t v;
    while (true) {
	v = val->v.tup[i];
	if (value_is_empty(v)) {
	    val->v.tup[i] = value;
	    break;
	}
	if (i < 2)
	    ++i;
	else if (value_is_empty(val->v.tup[2])) {
            struct boxed_value *rest = gc_alloc_val(ctx->gc, VALUE_TUPLE);
	    val->v.tup[2] = value_from_ptr(rest);
	    val = rest;
	}
    }
}
