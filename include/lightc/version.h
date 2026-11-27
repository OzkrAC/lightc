#ifndef LIGHTC_VERSION_H
#define LIGHTC_VERSION_H

#include <stdint.h>

/*
 * The version of lightc, and the one place it is written: configure reads it
 * from here for lightc.pc. <lightc/types.h> includes this header, so every
 * lightc header gives the version.
 */
#define LC_VERSION_MAJOR 0
#define LC_VERSION_MINOR 2
#define LC_VERSION_PATCH 0

/* The version as one integer: major * 10000 + minor * 100 + patch. */
[[gnu::const]]
static inline int32_t lc_version(void) {
    return LC_VERSION_MAJOR * 10000 + LC_VERSION_MINOR * 100 + LC_VERSION_PATCH;
}

/*
 * The ABI tag, lc_abi_MAJOR_MINOR, such as lc_abi_0_2: liblightc.a defines
 * it, and each object compiled with these headers refers to it. A program
 * built with the headers of one minor version and linked with the library
 * of another then fails to link, with "undefined reference to `lc_abi_...'",
 * instead of running with structs laid out two ways. Patch versions share a
 * tag: they keep the ABI.
 *
 * The reference is a word in .lightc_abi, a section that is not loaded, so
 * it costs the program no memory, and the library defines the tag as an
 * absolute symbol, which takes no space. A shared object compiled with
 * -fPIC has no reference: a plugin is linked with no liblightc.a, and the
 * loader that runs it is the program's own lightc. A PIE program has one.
 */
#define LC_ABI_NAME_(major, minor) lc_abi_##major##_##minor
#define LC_ABI_NAME(major, minor)  LC_ABI_NAME_(major, minor)
#define LC_ABI_STRING_(name)       #name
#define LC_ABI_STRING(name)        LC_ABI_STRING_(name)
#define LC_ABI_TAG                 LC_ABI_NAME(LC_VERSION_MAJOR, LC_VERSION_MINOR)

#if !defined(__PIC__) || defined(__PIE__)
__asm__(".pushsection .lightc_abi,\"\",%progbits\n\t"
        ".quad " LC_ABI_STRING(LC_ABI_TAG) "\n\t"
        ".popsection");
#endif

#endif /* LIGHTC_VERSION_H */
