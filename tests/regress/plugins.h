/*
 * plugins.h — the test plugins, for the LIB-* regress programs.
 *
 * Each LIB-* program loads a plugin of build/plugins, which ninja builds
 * from tests/plugins/ with the link flags that make what the program is
 * about: PLT relocations, DT_RELR, a nonzero base address. A toolchain
 * that ignored such a flag would build a plugin without that, and the
 * program would pass with the defect as well. So a program first reads its
 * plugin's file with plugin_file_read and requires what it tests.
 *
 * The paths are relative to the source root, where ninja regress runs the
 * programs.
 */

#ifndef LIGHTC_TEST_PLUGINS_H
#define LIGHTC_TEST_PLUGINS_H

#include <lightc/heap.h>
#include <lightc/io.h>
#include <lightc/library.h>
#include <lightc/string.h>
#include "../../src/kernel.h"   /* Elf64_*, PT_*, DT_* */

#define PLUGIN_DIR "build/plugins/"

/* A plugin's file, read whole. */
typedef struct {
    uint8_t *data;
    size_t   size;
} plugin_file;

/* Reads the ELF64 file at `path`; false when it cannot, or the file has no
 * whole program header table. */
static inline bool plugin_file_read(plugin_file *file, const char *path) {
    file->data = NULL;
    file->size = 0;
    if (lc_file_read_all(path, &file->data, &file->size).error) return false;
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *)file->data;
    if (file->size < sizeof(*ehdr) || ehdr->e_phentsize != sizeof(Elf64_Phdr) ||
        ehdr->e_phoff % _Alignof(Elf64_Phdr) != 0 || ehdr->e_phoff > file->size ||
        (uint64_t)ehdr->e_phnum * sizeof(Elf64_Phdr) > file->size - ehdr->e_phoff) {
        lc_heap_free(file->data);
        file->data = NULL;
        return false;
    }
    return true;
}

static inline void plugin_file_free(plugin_file *file) {
    lc_heap_free(file->data);
    file->data = NULL;
}

/* The program header `index` of the file, or NULL past the last one. */
static inline const Elf64_Phdr *plugin_file_phdr(const plugin_file *file, size_t index) {
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *)file->data;
    if (index >= ehdr->e_phnum) return NULL;
    return (const Elf64_Phdr *)(file->data + ehdr->e_phoff) + index;
}

/* The first program header of type `type`, or NULL. */
static inline const Elf64_Phdr *plugin_file_find_phdr(const plugin_file *file, uint32_t type) {
    const Elf64_Phdr *phdr;
    for (size_t i = 0; (phdr = plugin_file_phdr(file, i)) != NULL; i++)
        if (phdr->p_type == type) return phdr;
    return NULL;
}

/* The `size` bytes at the virtual address `vaddr` in the file, or NULL
 * when no PT_LOAD has them all in the file. */
static inline const uint8_t *plugin_file_at(const plugin_file *file, uint64_t vaddr, uint64_t size) {
    const Elf64_Phdr *phdr;
    for (size_t i = 0; (phdr = plugin_file_phdr(file, i)) != NULL; i++) {
        if (phdr->p_type != PT_LOAD || vaddr < phdr->p_vaddr || size > phdr->p_filesz ||
            vaddr - phdr->p_vaddr > phdr->p_filesz - size)
            continue;
        uint64_t offset = phdr->p_offset + (vaddr - phdr->p_vaddr);
        if (offset > file->size || size > file->size - offset) return NULL;
        return file->data + offset;
    }
    return NULL;
}

/* Sets *value to the value of the file's first dynamic entry with `tag`,
 * and returns whether it has one. */
static inline bool plugin_file_dynamic(const plugin_file *file, int64_t tag, uint64_t *value) {
    const Elf64_Phdr *dynamic = plugin_file_find_phdr(file, PT_DYNAMIC);
    if (!dynamic || dynamic->p_offset % _Alignof(Elf64_Dyn) != 0 ||
        dynamic->p_offset > file->size || dynamic->p_filesz > file->size - dynamic->p_offset)
        return false;
    const Elf64_Dyn *entries = (const Elf64_Dyn *)(file->data + dynamic->p_offset);
    for (size_t i = 0; i < dynamic->p_filesz / sizeof(Elf64_Dyn) && entries[i].d_tag != DT_NULL; i++) {
        if (entries[i].d_tag == tag) {
            *value = entries[i].d_un.d_val;
            return true;
        }
    }
    return false;
}

/* Whether an entry of the file's DT_RELA is against a symbol of section
 * index `shndx` and binding `bind`. */
static inline bool plugin_file_has_rela_to(const plugin_file *file, uint16_t shndx, uint8_t bind) {
    uint64_t rela, relasz, symtab;
    if (!plugin_file_dynamic(file, DT_RELA, &rela) || !plugin_file_dynamic(file, DT_RELASZ, &relasz) ||
        !plugin_file_dynamic(file, DT_SYMTAB, &symtab))
        return false;
    for (uint64_t i = 0; i < relasz / sizeof(Elf64_Rela); i++) {
        const uint8_t *at = plugin_file_at(file, rela + i * sizeof(Elf64_Rela), sizeof(Elf64_Rela));
        if (!at) return false;
        Elf64_Rela entry;
        lc_bytes_copy(&entry, at, sizeof entry);
        uint64_t index = ELF64_R_SYM(entry.r_info);
        if (index == 0) continue;
        at = plugin_file_at(file, symtab + index * sizeof(Elf64_Sym), sizeof(Elf64_Sym));
        if (!at) return false;
        Elf64_Sym sym;
        lc_bytes_copy(&sym, at, sizeof sym);
        if (sym.st_shndx == shndx && ELF64_ST_BIND(sym.st_info) == bind) return true;
    }
    return false;
}

/* The address of the symbol `name` of `lib`, or NULL. */
static inline void *plugin_symbol(lc_library *lib, const char *name) {
    lc_result_ptr r = lc_library_find_symbol(lib, name);
    return lc_ptr_is_err(r) ? NULL : r.value;
}

/* Sets the function pointer `fn`, of any function type, to the function
 * `name` of `lib`, and gives whether `lib` has it. C converts no data
 * pointer, which lc_library_find_symbol gives, to a function pointer, so
 * this copies the address. */
#define plugin_function(lib, name, fn) plugin_function_address((lib), (name), &(fn), sizeof(fn))

static inline bool plugin_function_address(lc_library *lib, const char *name, void *fn, size_t size) {
    void *address = plugin_symbol(lib, name);
    if (!address || size != sizeof(address)) return false;
    lc_bytes_copy(fn, &address, size);
    return true;
}

#endif /* LIGHTC_TEST_PLUGINS_H */
