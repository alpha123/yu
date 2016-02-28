#include "yu_common.h"

u32 yu_ceil_log2(u64 n) {
    // GCC complains about not having parens around n - 1
    // even though - is higher precedence than &
    // -Wno-parentheses turns it off, but also turns off
    // useful stuff like warning about assignments in
    // if/while that don't have an extra pair of parens.
    return 63 - __builtin_clzll(n) + !!(n & (n - 1));
}

#define YU_ERR_MSG(_code, desc) desc,

const char *yu_err_messages[] = {
    YU_ERR_LIST(YU_ERR_MSG)
};

#undef YU_ERR_MSG

void yu_exit_on_fatal(yu_err err) {
    exit(err);
}

yu_err_handler_func yu_global_fatal_handler = yu_exit_on_fatal;
