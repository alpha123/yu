changecom(`--')dnl
define(`list_tests_macro', `LIST_`'translit(test_name,`a-z',`A-Z')`'_TESTS')dnl
include(`m4/copyright.m4')dnl

#include "test.h"

#define SETUP \
    ...

#define TEARDOWN \
    ...

#define list_tests_macro`'(X) \
    X(...)


SUITE(test_name, list_tests_macro)
