#ifndef LIGHTC_LIBRARY_H
#define LIGHTC_LIBRARY_H

#include "types.h"

/*
 * Dynamic library loader — parse ELF, map segments, resolve symbols.
 * Our own dlopen/dlsym, built from scratch.
 *
 *   lc_library *lib = lc_library_open("./plugin.so");
 *   int (*add)(int, int) = lc_library_find_symbol(lib, "add");
 *   int result = add(2, 3);  // 5
 *   lc_library_close(lib);
 *
 * lc_library_open loads an ELF64 shared object of this arch that needs no
 * other object: it searches none for the symbols the library imports. It
 * maps the library with the kernel's page size, at a base that keeps the
 * largest p_align of its PT_LOADs, applies every relocation at once
 * (DT_RELA, DT_JMPREL and DT_RELR; RELATIVE, GLOB_DAT, JUMP_SLOT and
 * R_X86_64_64 or R_AARCH64_ABS64), makes PT_GNU_RELRO read-only, then runs
 * DT_INIT and .init_array. The file is open close-on-exec while it reads
 * it. It fails, and leaves nothing mapped, with
 *   the kernel's errno    when open, read, lseek, mmap or mprotect fails:
 *                         ENOENT, EACCES, ENOTDIR, EISDIR for a directory,
 *                         ENOMEM and the rest;
 *   LC_ERR_UNSUPPORTED    for what a valid library may need and the loader
 *                         does not do: thread-local storage (PT_TLS,
 *                         DF_STATIC_TLS, a TLS relocation), IFUNC (an
 *                         IRELATIVE relocation, or one against an IFUNC
 *                         symbol) and symbol versions (DT_VERSYM); and for
 *                         a library with its own copy of lightc, one that
 *                         defines lc_heap_*, lc_tls_* or lc_abi_* symbols;
 *   LC_ERR_RELOC_FAILED   for a relocation against an undefined symbol that
 *                         is not weak (a weak one is 0), of a type no
 *                         psABI of the arch gives a shared object (COPY,
 *                         ...), or with a target outside the library;
 *   LC_ERR_MALFORMED_ELF  for a file shorter than its headers say, a
 *                         p_align that is not a power of two, a table entry
 *                         size other than ELF64's, a DT_PLTREL other than
 *                         DT_RELA, two PT_LOADs with different protections
 *                         on one page, or a table outside the library.
 *
 * A library must not link liblightc.a. Its copy of lightc would keep
 * globals of its own but find its per-thread block through the thread
 * pointer, which is the host's: its TLS keys would overwrite the host's,
 * and its heap would run on the host's per-thread heap state. The check
 * above is best effort, as --exclude-libs or hidden visibility hide the
 * names it looks for. A library may use lightc's inline functions, which
 * keep no state. docs/contracts.html gives the rules for plugins.
 *
 * lc_library_find_symbol gives the address a relocation against the symbol
 * would get: an absolute symbol's value as it is. It fails with
 * LC_ERR_NOENT for a name the library does not define, and with
 * LC_ERR_UNSUPPORTED for an IFUNC or TLS symbol, which has no address to
 * give without its resolver or a TLS block.
 */

typedef struct lc_library lc_library;

/* Load a shared library (.so file). */
[[nodiscard]] lc_result_ptr lc_library_open(const char *path);

/* Find a symbol (function or variable) by name. */
[[nodiscard]] lc_result_ptr lc_library_find_symbol(lc_library *lib, const char *name);

/* Unload the library and free resources. */
void lc_library_close(lc_library *lib);

#endif /* LIGHTC_LIBRARY_H */
