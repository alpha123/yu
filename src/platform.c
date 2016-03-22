/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "yu_common.h"

// Link with proper implementation of platform layer

#if YU_OSAPI == YU_OSAPI_WIN32
#include "platform/win32.h"
#elif __linux__
#include "platform/linux.h"
#else
#include "platform/posix.h"
#endif
