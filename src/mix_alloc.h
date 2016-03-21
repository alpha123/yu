/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

/**
 * Allocator that wraps two abstract allocators to form a full allocator.
 * It expects one allocator to implement page-level allocations and another
 * to implement object-level allocations. This allows mixing and matching
 * allocators for different purposes. The object allocator may be a standard
 * high-performance allocator like jemalloc and the page allocator may be
 * a platform dependent allocator.
 *
 * The hierarchy will look something like this in a regular configuration:
 *
 *          +---------------------------------------+
 *          |              mix_alloc                |
 *          |            /          \               |
 *          | page allocation     object allocation |
 *          +---------------------------------------+
 *               //                       \\
 *              //                         \\
 *     +--------------------+         +----------------------------+
 *     |     page_alloc     |         | je_alloc, debug_alloc, etc |
 *     |    /          \    |         +----------------------------+
 *     | manages * os calls |                       \\
 *     | context *          |                     +--------------------+
 *     +--------------------+                     | underlying library |
 *                      \\                        +--------------------+
 *                 +----------------+
 *                 | posix_alloc or |
 *                 | win32_alloc    |
 *                 +----------------+
 *                          \\
 *                     +--------------+
 *                     | mmap or      |
 *                     | VirtualAlloc |
 *                     +--------------+
 */
