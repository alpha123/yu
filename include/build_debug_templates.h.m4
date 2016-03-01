/**
 * This file includes the "real" versions of all the files in include/dummy.
 * Used for debugging the macro-templates; see the build_debug_test rule
 * in the Makefile.
 */

patsubst(dnl
patsubst(dnl
patsubst(esyscmd(`find include/dummy -name "*.h"'), `include/dummy/', `'),dnl
`alloca\.h', `'),dnl
`^.+', `#include "\&"')

#if defined(__unix__)
#include <sys/param.h>
#ifndef BSD
#include <alloca.h>
#endif
#elif defined(_WIN32)
#include <malloc.h>
#define alloca _alloca
#endif
