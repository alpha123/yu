/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include <linux/version.h>

/**
 * Linux's madvise() makes slightly stronger guarantees about decommitting
 * memory than posix_madvise() does. Specifically, MADV_FREE (on 4.5 or newer)
 * will decommit, and MADV_DONTNEED might decommit. Also, posix_madvise(...,
 * DONTNEED) is a no-op on Linux with most (all?) glibc versions.
 */

#define MEMADVISE madvise

#define MEMADVISE_COMMIT MADV_WILLNEED

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
#define MEMADVISE_DECOMMIT MADV_FREE
#else
#define MEMADVISE_DECOMMIT MADV_DONTNEED
#endif

#include "platform/posix.h"

// TODO MAP_POPULATE allows reserve+commit in one operation on Linux 2.5+, as
// well as reducing blocking on page faults for future accesses.
