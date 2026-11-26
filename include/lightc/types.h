#ifndef LIGHTC_TYPES_H
#define LIGHTC_TYPES_H

/*
 * Freestanding headers — provided by GCC, not libc.
 * These are correct on every arch the compiler supports.
 */
#include <stdint.h>   /* uint8_t, int32_t, uint64_t, ... */
#include <stddef.h>   /* size_t, ptrdiff_t, NULL, nullptr_t */

/* The version, LC_VERSION_* and lc_version(), and the ABI tag. */
#include "version.h"

/*
 * The settings an install of lightc was built with. ninja install writes
 * <lightc/config.h>, which fixes LC_STATS to the library's and stops a
 * program that sets it otherwise. A tree used in place, as a git submodule
 * is, has none, and LC_STATS then follows NDEBUG, as memory.h says; no
 * struct depends on it, so a program built with another NDEBUG than the
 * library still links and runs.
 */
#if defined(__has_include)
# if __has_include(<lightc/config.h>)
#  include <lightc/config.h>
# endif
#endif

/* Syscall return: negative values are -errno */
typedef int64_t lc_sysret;

#include "result.h"

#endif /* LIGHTC_TYPES_H */
