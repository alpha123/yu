/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    ptrdiff_t num_suites = suite_registry_end - suite_registry_begin;
    for (int i = 0; i < num_suites; i++)
        pt_add_suite(suite_registry_begin[i]);
    return pt_run();
}
