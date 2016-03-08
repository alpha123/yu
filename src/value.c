/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "value.h"

value_type value_what(value_t val) {
    if (value_is_ptr(val))
        return boxed_value_get_type(value_to_ptr(val));
    if (value_is_bool(val))
        return VALUE_BOOL;
    if (value_is_int(val))
        return VALUE_FIXNUM;
    if (value_is_double(val))
        return VALUE_DOUBLE;
    return VALUE_ERR;
}
