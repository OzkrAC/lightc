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
 * maps the library with the kernel's page size, applies every relocation
 * at once (DT_RELA, DT_JMPREL and DT_RELR; RELATIVE, GLOB_DAT, JUMP_SLOT
 * and R_X86_64_64 or R_AARCH64_ABS64), makes PT_GNU_RELRO read-only, then
 * runs DT_INIT and .init_array. The file is open close-on-exec while it
 * reads it. It fails, and leaves nothing mapped, with
 *   LC_ERR_RELOC_FAILED   for a relocation against an undefined symbol that
 *                         is not weak (a weak one is 0), of a type it does
 *                         not apply (TLS, IFUNC, COPY, ...), or with a
 *                         target outside the library;
 *   LC_ERR_MALFORMED_ELF  for a PT_TLS segment, as it makes no TLS block,
 *                         a table entry size other than ELF64's, a
 *                         DT_PLTREL other than DT_RELA, two PT_LOADs with
 *                         different protections on one page, or a table
 *                         outside the library.
 */

typedef struct lc_library lc_library;

/* Load a shared library (.so file). */
[[nodiscard]] lc_result_ptr lc_library_open(const char *path);

/* Find a symbol (function or variable) by name. */
[[nodiscard]] lc_result_ptr lc_library_find_symbol(lc_library *lib, const char *name);

/* Unload the library and free resources. */
void lc_library_close(lc_library *lib);

#endif /* LIGHTC_LIBRARY_H */
