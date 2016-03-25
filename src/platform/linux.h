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

// TODO MAP_POPULATE allows reserve+commit in one operation on Linux 2.5+, as
// well as reducing blocking on page faults for future accesses.

#define MEMADVISE madvise

#define MEMADVISE_COMMIT MADV_WILLNEED

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
#define MEMADVISE_DECOMMIT MADV_FREE
#else
#define MEMADVISE_DECOMMIT MADV_DONTNEED
#endif


/**
 * Linux ≥ 3.19 has getrandom() which has fewer points of failure than reading
 * from /dev/urandom directly.
 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
#define USING_GETRANDOM

u32 yu_sys_random(void) {
  char rand[sizeof(u32)];
  // getrandom() cannot fail if the buffer size is less than 256 bytes and
  // GRND_RANDOM is not specified.
  getrandom(rand, sizeof(u32), 0);
  return *(u32 *)rand;
}
#endif

#include "platform/posix.h"
