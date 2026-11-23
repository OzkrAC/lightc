#include <lightc/library.h>
#include <lightc/syscall.h>
#include <lightc/heap.h>
#include <lightc/string.h>
#include <lightc/memory.h>   /* lc_runtime_page_size */
#include "kernel.h"          /* Elf64_*, and the ELF constants PT_*, DT_*, R_* ... */

/* ========================================================================
 * Library struct
 * ======================================================================== */

/* The kernel's page size, from AT_PAGESZ: 4 KiB on x86_64, and 4, 16 or
 * 64 KiB on aarch64. mprotect takes only addresses aligned to it. */
#define PAGE_SIZE ((uint64_t)lc_runtime_page_size())
#define PAGE_ALIGN_DOWN(x) ((x) & ~((uint64_t)(PAGE_SIZE - 1)))
#define PAGE_ALIGN_UP(x)   (((x) + PAGE_SIZE - 1) & ~((uint64_t)(PAGE_SIZE - 1)))

struct lc_library {
    uint8_t *base;           /* mmap'd base address */
    size_t   total_size;     /* total mmap'd size */
    uintptr_t bias;          /* load bias: base - min_vaddr, added to every vaddr */

    /* Parsed from DYNAMIC segment */
    Elf64_Sym  *symtab;      /* symbol table */
    char       *strtab;      /* string table */
    size_t      strtab_size;
    Elf64_Rela *rela;        /* relocation table (DT_RELA) */
    size_t      rela_count;
    Elf64_Rela *jmprel;      /* PLT relocations (DT_JMPREL) */
    size_t      jmprel_count;
    uint64_t   *relr;        /* packed relative relocations (DT_RELR) */
    size_t      relr_count;

    /* ELF hash table (for symbol lookup) */
    uint32_t   *elf_hash;

    /* GNU hash table (for fast symbol lookup) */
    uint32_t   *gnu_hash;

    /* Init/fini */
    void      **init_array;
    size_t      init_array_count;
    void      **fini_array;
    size_t      fini_array_count;
    void       (*init_func)(void);
    void       (*fini_func)(void);
};

/* ========================================================================
 * Internal helpers
 * ======================================================================== */

/* Read exactly `count` bytes from `fd`, retrying on short reads. */
static bool read_all(int32_t fd, void *buf, size_t count) {
    uint8_t *p = (uint8_t *)buf;
    while (count > 0) {
        lc_sysret n = lc_kernel_read_bytes(fd, p, count);
        if (n <= 0) return false;
        p += n;
        count -= (size_t)n;
    }
    return true;
}

/* ========================================================================
 * Bounds checking against the mapped region
 *
 * The ELF file is fully untrusted. Every pointer we form from a file-supplied
 * offset/vaddr must be proven to lie inside [base, base + total_size) before we
 * dereference it. These helpers centralize that check so a malformed or hostile
 * .so can only ever fail the load — never read or write outside the mapping.
 * ======================================================================== */

/* True iff [p, p+len) lies fully within [base, base+total_size).
 * Overflow-safe: never forms a pointer past `end` to compare. */
static bool region_in_bounds(const lc_library *lib, const void *p, uint64_t len) {
    uintptr_t start = (uintptr_t)lib->base;
    uintptr_t end   = start + lib->total_size;
    uintptr_t q     = (uintptr_t)p;
    if (q < start || q > end) return false;
    return len <= (uint64_t)(end - q);
}

/* The address where the object's virtual address `vaddr` is mapped. It is
 * only a number until region_in_bounds has checked it: a hostile vaddr
 * gives an address outside the mapping, never a pointer overflow. */
static void *mapped_address(const lc_library *lib, uint64_t vaddr) {
    return (void *)(lib->bias + vaddr);
}

/* Compare `a` (bounded by `a_end`) against NUL-terminated `b`. Never reads at
 * or past `a_end`, so an unterminated string table cannot walk out of bounds. */
static bool str_equal_bounded(const char *a, const char *a_end, const char *b) {
    while (a < a_end && *a && *b) {
        if (*a != *b) return false;
        a++;
        b++;
    }
    /* Match iff both ended together: `a` on its NUL (still in range) and `b` on
     * its NUL. If we hit a_end first without a NUL, it is not a valid match. */
    return a < a_end && *a == 0 && *b == 0;
}

/* Convert ELF program header flags to mmap prot flags. */
static int32_t prot_from_elf_flags(uint32_t p_flags) {
    int32_t prot = 0;
    if (p_flags & PF_R) prot |= PROT_READ;
    if (p_flags & PF_W) prot |= PROT_WRITE;
    if (p_flags & PF_X) prot |= PROT_EXEC;
    return prot;
}

/* ELF hash function (for DT_HASH lookup). */
static uint32_t elf_hash_function(const char *name) {
    uint32_t h = 0, g;
    while (*name) {
        h = (h << 4) + (uint8_t)*name++;
        g = h & 0xf0000000;
        if (g) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

/* ========================================================================
 * ELF validation
 * ======================================================================== */

static int32_t validate_elf_header(const Elf64_Ehdr *ehdr) {
    /* Check magic bytes */
    if (ehdr->e_ident[0] != ELFMAG0 ||
        ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 ||
        ehdr->e_ident[3] != ELFMAG3) {
        return LC_ERR_BAD_ELF_MAGIC;
    }

    /* Must be 64-bit */
    if (ehdr->e_ident[4] != ELFCLASS64) {
        return LC_ERR_NOT_64BIT;
    }

    /* Must be little-endian */
    if (ehdr->e_ident[5] != ELFDATA2LSB) {
        return LC_ERR_BAD_ELF_MAGIC;
    }

    /* Must be a shared object (ET_DYN) */
    if (ehdr->e_type != ET_DYN) {
        return LC_ERR_BAD_ELF_MAGIC;
    }

    /* Check machine type matches current arch */
#if defined(__x86_64__)
    if (ehdr->e_machine != EM_X86_64) {
        return LC_ERR_BAD_MACHINE;
    }
#elif defined(__aarch64__)
    if (ehdr->e_machine != EM_AARCH64) {
        return LC_ERR_BAD_MACHINE;
    }
#endif

    /* We index the program-header table at a fixed sizeof(Elf64_Phdr) stride,
     * so a file-supplied e_phentsize that disagrees would desync our indexing
     * from the size we allocate and read — reject it outright. */
    if (ehdr->e_phentsize != sizeof(Elf64_Phdr)) {
        return LC_ERR_MALFORMED_ELF;
    }
    if (ehdr->e_phnum == 0) {
        return LC_ERR_NO_LOAD_SEG;
    }

    return LC_OK;
}

/* ========================================================================
 * Relocation processing
 * ======================================================================== */

/* Forward declaration — defined in the symbol lookup section below. */
static size_t get_symbol_count(lc_library *lib);

/* Resolve a relocation's symbol index to an in-bounds symbol pointer, or NULL.
 * Index 0 is STN_UNDEF (no symbol). Any index that falls outside the symbol
 * table — including the case where the table size is unknown (sym_count == 0)
 * — is rejected by the mapping bounds check, so symtab[sym_idx] can never be an
 * out-of-bounds read even for a hostile r_info. */
static Elf64_Sym *reloc_symbol(lc_library *lib, uint32_t sym_idx, size_t sym_count) {
    if (sym_idx == 0) return NULL;
    if (sym_count > 0 && sym_idx >= sym_count) return NULL;
    Elf64_Sym *sym = &lib->symtab[sym_idx];
    if (!region_in_bounds(lib, sym, sizeof(*sym))) return NULL;
    return sym;
}

/* The value of `sym` for a relocation: S. An undefined weak symbol is 0. An
 * undefined strong one has no definition to bind to, since the loader has
 * no other object to search, and a TLS symbol's value is an offset in a TLS
 * block the loader does not make: both fail the load. */
static bool reloc_symbol_value(const lc_library *lib, const Elf64_Sym *sym, uint64_t *out) {
    if (sym->st_shndx == SHN_UNDEF) {
        if (ELF64_ST_BIND(sym->st_info) != STB_WEAK) return false;
        *out = 0;
        return true;
    }
    if (ELF64_ST_TYPE(sym->st_info) == STT_TLS) return false;
    *out = (sym->st_shndx == SHN_ABS) ? sym->st_value : lib->bias + sym->st_value;
    return true;
}

/* Apply `count` RELA entries of `table` (DT_RELA or DT_JMPREL). Every
 * entry is applied now, PLT slots too: there is no lazy binding. A target
 * outside the mapping, an unresolved symbol or a relocation type this
 * loader does not know fails the load, as anything it skipped would leave
 * a pointer that the plugin later follows to a wrong address. A NONE entry
 * has no target, so its offset is not looked at: GNU ld leaves all-zero
 * ones in a table it sized larger than it filled. */
static bool process_rela_table(lc_library *lib, const Elf64_Rela *table, size_t count,
                               size_t sym_count) {
    for (size_t i = 0; i < count; i++) {
        const Elf64_Rela *rela = &table[i];
        uint32_t type = ELF64_R_TYPE(rela->r_info);
        uint32_t sym_idx = ELF64_R_SYM(rela->r_info);
        uint64_t value;

#if defined(__x86_64__)
        switch (type) {
            case R_X86_64_NONE:
                continue;

            case R_X86_64_RELATIVE:
                value = lib->bias + rela->r_addend;
                break;

            case R_X86_64_GLOB_DAT:
            case R_X86_64_JUMP_SLOT:
            case R_X86_64_64: {
#elif defined(__aarch64__)
        switch (type) {
            case R_AARCH64_NONE:
                continue;

            case R_AARCH64_RELATIVE:
                value = lib->bias + rela->r_addend;
                break;

            case R_AARCH64_GLOB_DAT:
            case R_AARCH64_JUMP_SLOT:
            case R_AARCH64_ABS64: {
#endif
                Elf64_Sym *sym = reloc_symbol(lib, sym_idx, sym_count);
                if (!sym || !reloc_symbol_value(lib, sym, &value)) return false;
                value += rela->r_addend;
                break;
            }

            default:
                /* TLS, IFUNC, COPY and every other type: not supported */
                return false;
        }

        /* The write is 8 bytes at `target`: it must lie wholly within the
         * mapping, or it is an arbitrary write. */
        uint64_t *target = mapped_address(lib, rela->r_offset);
        if (!region_in_bounds(lib, target, sizeof(*target))) return false;
        *target = value;
    }
    return true;
}

/* Apply DT_RELR, the packed relative relocations of ld -z
 * pack-relative-relocs: each word they name gets the load bias added. An
 * even entry is the address of one such word. An odd entry is a bitmap of
 * the 63 words that follow the last one relocated so far: bit n, from 1 to
 * 63, names word n - 1 of them, and a further bitmap goes on 63 words
 * later. */
static bool process_relr_table(lc_library *lib) {
    uint64_t *where = NULL;
    for (size_t i = 0; i < lib->relr_count; i++) {
        uint64_t entry = lib->relr[i];
        if ((entry & 1) == 0) {
            where = mapped_address(lib, entry);
            if (!region_in_bounds(lib, where, sizeof(*where))) return false;
            *where++ += lib->bias;
        } else {
            if (!where) return false;   /* a bitmap before any address */
            uint64_t *p = where;
            for (entry >>= 1; entry != 0; entry >>= 1, p++) {
                if (entry & 1) {
                    if (!region_in_bounds(lib, p, sizeof(*p))) return false;
                    *p += lib->bias;
                }
            }
            where += 8 * sizeof(entry) - 1;   /* 63: the entry's bits less the tag */
        }
    }
    return true;
}

/* RELR first: it holds only relative relocations, which the others do not
 * read. */
static bool process_relocations(lc_library *lib) {
    size_t sym_count = get_symbol_count(lib);
    if (lib->relr && !process_relr_table(lib)) return false;
    if (lib->rela && !process_rela_table(lib, lib->rela, lib->rela_count, sym_count)) return false;
    if (lib->jmprel && !process_rela_table(lib, lib->jmprel, lib->jmprel_count, sym_count)) return false;
    return true;
}

/* ========================================================================
 * Symbol lookup
 * ======================================================================== */

/* GNU hash function (djb2-style, used by DT_GNU_HASH). */
static uint32_t gnu_hash_function(const char *name) {
    uint32_t h = 5381;
    while (*name) {
        h = (h << 5) + h + (uint8_t)*name++;
    }
    return h;
}

/*
 * GNU hash table layout:
 *   uint32_t nbuckets
 *   uint32_t symndx        — first symbol index in chains
 *   uint32_t maskwords     — number of Bloom filter words
 *   uint32_t shift2        — Bloom filter shift
 *   uint64_t bloom[maskwords]  (Elf64: 64-bit words)
 *   uint32_t buckets[nbuckets]
 *   uint32_t chains[...]   — one per symbol starting at symndx
 *
 * Chain values have bit 0 set on the last entry in the chain.
 */
static void *find_symbol_with_gnu_hash(lc_library *lib, const char *name) {
    uint32_t *hash_table = lib->gnu_hash;

    /* The 4-word header must itself be readable before we trust its fields. */
    if (!region_in_bounds(lib, hash_table, 4 * sizeof(uint32_t))) return NULL;

    uint32_t nbuckets  = hash_table[0];
    uint32_t symndx    = hash_table[1];
    uint32_t maskwords = hash_table[2];
    /* hash_table[3] is shift2, used for Bloom filter — we skip Bloom check */

    /* nbuckets == 0 would divide by zero; oversized counts would overflow the
     * pointer math below. Cap both against the mapping before using them. */
    if (nbuckets == 0) return NULL;
    if ((uint64_t)maskwords * sizeof(uint64_t) > lib->total_size) return NULL;
    if ((uint64_t)nbuckets * sizeof(uint32_t) > lib->total_size) return NULL;

    /* Bloom filter: 64-bit words start at &hash_table[4] */
    uint64_t *bloom = (uint64_t *)&hash_table[4];

    /* Buckets follow the Bloom filter */
    uint32_t *buckets = (uint32_t *)(bloom + maskwords);
    if (!region_in_bounds(lib, buckets, (uint64_t)nbuckets * sizeof(uint32_t))) return NULL;

    /* Chains follow the buckets; length is unknown, so each access is bounded. */
    uint32_t *chains = buckets + nbuckets;

    uint32_t hash = gnu_hash_function(name);

    /* Bloom filter check (optional, skip for simplicity — go straight to bucket) */

    uint32_t bucket_idx = hash % nbuckets;
    uint32_t sym_idx = buckets[bucket_idx];

    if (sym_idx == 0) return NULL; /* empty bucket */
    if (sym_idx < symndx) return NULL; /* shouldn't happen */

    const char *str_end = lib->strtab + lib->strtab_size;

    /* Walk the chain. It only goes forward, and the index is 64-bit so it
     * cannot wrap back to the start of the chains: a walk that never meets
     * an end marker leaves the mapping and stops. */
    uint64_t chain_idx = sym_idx - symndx;
    uint32_t hash1 = hash | 1; /* set bit 0 for comparison (chains store hash with bit 0 as end marker) */

    for (;;) {
        /* Stop if the chain runs past the mapping (missing terminator). */
        if (!region_in_bounds(lib, &chains[chain_idx], sizeof(uint32_t))) break;
        uint32_t chain_val = chains[chain_idx];

        /* Compare hashes (ignoring bit 0 which is the end-of-chain marker) */
        if ((chain_val | 1) == hash1) {
            /* Hash matches — compare the actual name */
            Elf64_Sym *sym = &lib->symtab[symndx + chain_idx];
            if (region_in_bounds(lib, sym, sizeof(*sym)) &&
                sym->st_name < lib->strtab_size &&
                str_equal_bounded(lib->strtab + sym->st_name, str_end, name)) {
                if (sym->st_shndx != SHN_UNDEF && ELF64_ST_TYPE(sym->st_info) != STT_TLS) {
                    return mapped_address(lib, sym->st_value);
                }
            }
        }

        /* Bit 0 set means end of chain */
        if (chain_val & 1) break;
        chain_idx++;
    }

    return NULL;
}

/* Look up a symbol using the ELF hash table (DT_HASH). */
static void *find_symbol_with_elf_hash(lc_library *lib, const char *name) {
    uint32_t *hash_table = lib->elf_hash;
    /* Two-word header (nbucket, nchain) must be readable first. */
    if (!region_in_bounds(lib, hash_table, 2 * sizeof(uint32_t))) return NULL;

    uint32_t nbucket = hash_table[0];
    uint32_t nchain  = hash_table[1]; /* also the number of symbols */
    if (nbucket == 0) return NULL;    /* would divide by zero */
    if ((uint64_t)nbucket * sizeof(uint32_t) > lib->total_size) return NULL;

    uint32_t *buckets = &hash_table[2];
    if (!region_in_bounds(lib, buckets, (uint64_t)nbucket * sizeof(uint32_t))) return NULL;
    uint32_t *chains  = &hash_table[2 + nbucket];

    const char *str_end = lib->strtab + lib->strtab_size;

    uint32_t hash = elf_hash_function(name);
    uint32_t idx = buckets[hash % nbucket];

    /* Bound the walk: a valid chain index is < nchain, each chain slot must
     * be readable, and a valid chain visits each of the nchain symbols at
     * most once, so a longer walk is a chain that loops. Any guard failing
     * means a malformed table — stop. */
    for (uint32_t steps = 0; idx != 0 && idx < nchain && steps < nchain; steps++) {
        Elf64_Sym *sym = &lib->symtab[idx];
        if (!region_in_bounds(lib, sym, sizeof(*sym))) break;
        if (sym->st_name < lib->strtab_size &&
            str_equal_bounded(lib->strtab + sym->st_name, str_end, name)) {
            if (sym->st_shndx != SHN_UNDEF && ELF64_ST_TYPE(sym->st_info) != STT_TLS) {
                return mapped_address(lib, sym->st_value);
            }
        }
        if (!region_in_bounds(lib, &chains[idx], sizeof(uint32_t))) break;
        idx = chains[idx];
    }
    return NULL;
}

/* Determine the total number of symbols using available hash tables.
 * With DT_HASH: nchain gives exact count.
 * With DT_GNU_HASH: walk buckets/chains to find the highest symbol index. */
static size_t get_symbol_count(lc_library *lib) {
    if (lib->elf_hash) {
        if (!region_in_bounds(lib, lib->elf_hash, 2 * sizeof(uint32_t))) return 0;
        return lib->elf_hash[1]; /* nchain */
    }

    if (lib->gnu_hash) {
        uint32_t *hash_table = lib->gnu_hash;
        if (!region_in_bounds(lib, hash_table, 4 * sizeof(uint32_t))) return 0;

        uint32_t nbuckets  = hash_table[0];
        uint32_t symndx    = hash_table[1];
        uint32_t maskwords = hash_table[2];

        if (nbuckets == 0) return 0;
        if ((uint64_t)maskwords * sizeof(uint64_t) > lib->total_size) return 0;
        if ((uint64_t)nbuckets * sizeof(uint32_t) > lib->total_size) return 0;

        uint64_t *bloom = (uint64_t *)&hash_table[4];
        uint32_t *buckets = (uint32_t *)(bloom + maskwords);
        if (!region_in_bounds(lib, buckets, (uint64_t)nbuckets * sizeof(uint32_t))) return 0;
        uint32_t *chains  = buckets + nbuckets;

        /* Find the highest occupied bucket */
        uint32_t max_sym = symndx;
        for (uint32_t i = 0; i < nbuckets; i++) {
            if (buckets[i] > max_sym) {
                max_sym = buckets[i];
            }
        }

        if (max_sym < symndx) return symndx; /* no symbols in hash */

        /* Walk the chain from max_sym to find the terminator, bounded so a
         * chain with no terminator cannot run off the end of the mapping. */
        uint32_t chain_idx = max_sym - symndx;
        while (region_in_bounds(lib, &chains[chain_idx], sizeof(uint32_t)) &&
               !(chains[chain_idx] & 1)) {
            chain_idx++;
        }
        /* Total symbols = symndx + chain_idx + 1 */
        return symndx + chain_idx + 1;
    }

    return 0;
}

/* Look up a symbol by linear scan of the symbol table. */
static void *find_symbol_linear_scan(lc_library *lib, const char *name) {
    size_t sym_count = get_symbol_count(lib);
    const char *str_end = lib->strtab + lib->strtab_size;

    for (size_t i = 1; i < sym_count; i++) { /* skip index 0 (STN_UNDEF) */
        Elf64_Sym *sym = &lib->symtab[i];
        if (!region_in_bounds(lib, sym, sizeof(*sym))) break; /* bogus count */
        uint8_t bind = ELF64_ST_BIND(sym->st_info);

        if (bind != STB_GLOBAL && bind != STB_WEAK) continue;
        if (sym->st_shndx == SHN_UNDEF) continue;
        if (sym->st_name >= lib->strtab_size) continue;

        if (str_equal_bounded(lib->strtab + sym->st_name, str_end, name)) {
            return mapped_address(lib, sym->st_value);
        }
    }
    return NULL;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

lc_result_ptr lc_library_open(const char *path) {
    /* --- Step 1: Open the file --- */
    lc_sysret fd_ret = lc_kernel_open_file(path, O_RDONLY | O_CLOEXEC, 0);
    if (fd_ret < 0) {
        return lc_err_ptr(LC_ERR_NOENT);
    }
    int32_t fd = (int32_t)fd_ret;

    /* --- Step 2: Read and validate ELF header --- */
    Elf64_Ehdr ehdr;
    if (!read_all(fd, &ehdr, sizeof(ehdr))) {
        lc_kernel_close_file(fd);
        return lc_err_ptr(LC_ERR_IO);
    }
    int32_t hdr_err = validate_elf_header(&ehdr);
    if (hdr_err != LC_OK) {
        lc_kernel_close_file(fd);
        return lc_err_ptr(hdr_err);
    }

    /* --- Step 3: Read program headers --- */
    size_t phdr_table_size = (size_t)ehdr.e_phnum * ehdr.e_phentsize;
    lc_result_ptr phdr_alloc = lc_heap_allocate(phdr_table_size);
    if (lc_ptr_is_err(phdr_alloc)) {
        lc_kernel_close_file(fd);
        return lc_err_ptr(LC_ERR_NOMEM);
    }
    Elf64_Phdr *phdrs = (Elf64_Phdr *)phdr_alloc.value;

    if (lc_kernel_seek_position(fd, (int64_t)ehdr.e_phoff, SEEK_SET) < 0) {
        lc_heap_free(phdrs);
        lc_kernel_close_file(fd);
        return lc_err_ptr(LC_ERR_IO);
    }
    if (!read_all(fd, phdrs, phdr_table_size)) {
        lc_heap_free(phdrs);
        lc_kernel_close_file(fd);
        return lc_err_ptr(LC_ERR_IO);
    }

    /* --- Step 4: Compute total address span of LOAD segments --- */
    uint64_t min_vaddr = UINT64_MAX;
    uint64_t max_vaddr = 0;
    bool found_load = false;
    uint64_t prev_page_end = 0;     /* the end of the last PT_LOAD's pages */
    int32_t  prev_prot = 0;         /* and its protection */

    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        /* The loader makes no TLS block for the object, so its thread-local
         * variables would have no storage: code that reaches for one through
         * the thread pointer would use lightc's own per-thread data. */
        if (phdrs[i].p_type == PT_TLS) {
            lc_heap_free(phdrs);
            lc_kernel_close_file(fd);
            return lc_err_ptr(LC_ERR_MALFORMED_ELF);
        }
        if (phdrs[i].p_type != PT_LOAD) continue;

        /* Reject nonsensical/hostile segment sizes: more bytes in the file than
         * in memory would overflow the copy, and a vaddr+memsz that wraps would
         * corrupt the span math below. */
        if (phdrs[i].p_filesz > phdrs[i].p_memsz) {
            lc_heap_free(phdrs);
            lc_kernel_close_file(fd);
            return lc_err_ptr(LC_ERR_MALFORMED_ELF);
        }
        uint64_t end;
        if (__builtin_add_overflow(phdrs[i].p_vaddr, phdrs[i].p_memsz, &end)) {
            lc_heap_free(phdrs);
            lc_kernel_close_file(fd);
            return lc_err_ptr(LC_ERR_MALFORMED_ELF);
        }

        /* Step 10 gives each PT_LOAD its protection over the whole pages it
         * touches. Two PT_LOADs with different protections on one page would
         * leave it with the later one's, and the code or data of the other
         * would fault: an object linked for a smaller page size than the
         * kernel's, as ld -z max-page-size=4096 makes it, on a 16 KiB-page
         * aarch64 kernel. glibc refuses such objects too. PT_LOADs come in
         * vaddr order. */
        int32_t prot = prot_from_elf_flags(phdrs[i].p_flags);
        if (found_load && PAGE_ALIGN_DOWN(phdrs[i].p_vaddr) < prev_page_end && prot != prev_prot) {
            lc_heap_free(phdrs);
            lc_kernel_close_file(fd);
            return lc_err_ptr(LC_ERR_MALFORMED_ELF);
        }
        prev_page_end = PAGE_ALIGN_UP(end);
        prev_prot = prot;

        found_load = true;
        if (phdrs[i].p_vaddr < min_vaddr) {
            min_vaddr = phdrs[i].p_vaddr;
        }
        if (end > max_vaddr) {
            max_vaddr = end;
        }
    }

    if (!found_load) {
        lc_heap_free(phdrs);
        lc_kernel_close_file(fd);
        return lc_err_ptr(LC_ERR_NO_LOAD_SEG);
    }

    /* Align min_vaddr down and max_vaddr up to page boundaries */
    min_vaddr = PAGE_ALIGN_DOWN(min_vaddr);
    size_t total_size = (size_t)PAGE_ALIGN_UP(max_vaddr - min_vaddr);

    /* --- Step 5: Map a single contiguous anonymous region --- */
    void *base_ptr = lc_kernel_map_memory(
        NULL, total_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
    if (base_ptr == MAP_FAILED) {
        lc_heap_free(phdrs);
        lc_kernel_close_file(fd);
        return lc_err_ptr(LC_ERR_NOMEM);
    }
    uint8_t *base = (uint8_t *)base_ptr;

    /* Zero the entire region (MAP_ANONYMOUS should do this, but be safe for .bss) */

    /* --- Step 6: Read each LOAD segment into the mapped region --- */
    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        if (phdrs[i].p_filesz == 0) continue;

        uint64_t offset_in_map = phdrs[i].p_vaddr - min_vaddr;

        /* Prove the destination range [offset_in_map, +p_filesz) lies wholly
         * within the mapping before reading file bytes into it — otherwise a
         * crafted p_vaddr/p_filesz is an out-of-bounds write. */
        if (offset_in_map > total_size || phdrs[i].p_filesz > total_size - offset_in_map) {
            lc_kernel_unmap_memory(base, total_size);
            lc_heap_free(phdrs);
            lc_kernel_close_file(fd);
            return lc_err_ptr(LC_ERR_MALFORMED_ELF);
        }

        if (lc_kernel_seek_position(fd, (int64_t)phdrs[i].p_offset, SEEK_SET) < 0) {
            lc_kernel_unmap_memory(base, total_size);
            lc_heap_free(phdrs);
            lc_kernel_close_file(fd);
            return lc_err_ptr(LC_ERR_IO);
        }
        if (!read_all(fd, base + offset_in_map, (size_t)phdrs[i].p_filesz)) {
            lc_kernel_unmap_memory(base, total_size);
            lc_heap_free(phdrs);
            lc_kernel_close_file(fd);
            return lc_err_ptr(LC_ERR_IO);
        }
        /* .bss (memsz > filesz) is already zero from MAP_ANONYMOUS */
    }

    /* Done with the file */
    lc_kernel_close_file(fd);

    /* --- Step 7: Allocate and initialize the library struct ---
     * Note: page permissions are intentionally left RW (as mapped) until AFTER
     * relocations are applied (Step 9). Relocations frequently write into
     * segments whose final permission is read-only (e.g. GLOB_DAT into a
     * relro region); tightening first would fault. */
    lc_result_ptr lib_alloc = lc_heap_allocate_zeroed(sizeof(lc_library));
    if (lc_ptr_is_err(lib_alloc)) {
        lc_kernel_unmap_memory(base, total_size);
        lc_heap_free(phdrs);
        return lc_err_ptr(LC_ERR_NOMEM);
    }
    lc_library *lib = (lc_library *)lib_alloc.value;
    lib->base = base;
    lib->total_size = total_size;
    lib->bias = (uintptr_t)base - min_vaddr;

    /* --- Step 8: Find and parse the DYNAMIC segment --- */
    Elf64_Dyn *dynamic = NULL;
    uint64_t   dynamic_bytes = 0;

    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dynamic = mapped_address(lib, phdrs[i].p_vaddr);
            dynamic_bytes = phdrs[i].p_filesz;
            break;
        }
    }

    if (!dynamic) {
        lc_kernel_unmap_memory(base, total_size);
        lc_heap_free(phdrs);
        lc_heap_free(lib);
        return lc_err_ptr(LC_ERR_NO_DYNAMIC);
    }
    /* The dynamic array itself must lie within the mapping before we walk it. */
    if (!region_in_bounds(lib, dynamic, dynamic_bytes)) {
        lc_kernel_unmap_memory(base, total_size);
        lc_heap_free(phdrs);
        lc_heap_free(lib);
        return lc_err_ptr(LC_ERR_MALFORMED_ELF);
    }

    /* Walk the dynamic table. All d_ptr values are virtual addresses, which
     * the load bias translates. The walk is bounded by the segment size so a
     * table with no DT_NULL terminator cannot run past the mapping. */
    size_t rela_total_size = 0;
    size_t rela_entry_size = 0;
    size_t jmprel_total_size = 0;
    int64_t jmprel_type = -1;       /* DT_PLTREL: DT_RELA or DT_REL */
    size_t relr_total_size = 0;
    size_t relr_entry_size = 0;
    size_t sym_entry_size = 0;
    size_t dyn_max = (size_t)(dynamic_bytes / sizeof(Elf64_Dyn));

    for (size_t di = 0; di < dyn_max && dynamic[di].d_tag != DT_NULL; di++) {
        Elf64_Dyn *dyn = &dynamic[di];
        switch (dyn->d_tag) {
            case DT_SYMTAB:
                lib->symtab = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_STRTAB:
                lib->strtab = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_STRSZ:
                lib->strtab_size = (size_t)dyn->d_un.d_val;
                break;
            case DT_SYMENT:
                sym_entry_size = (size_t)dyn->d_un.d_val;
                break;
            case DT_RELA:
                lib->rela = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_RELASZ:
                rela_total_size = (size_t)dyn->d_un.d_val;
                break;
            case DT_RELAENT:
                rela_entry_size = (size_t)dyn->d_un.d_val;
                break;
            case DT_JMPREL:
                lib->jmprel = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_PLTRELSZ:
                jmprel_total_size = (size_t)dyn->d_un.d_val;
                break;
            case DT_PLTREL:
                jmprel_type = (int64_t)dyn->d_un.d_val;
                break;
            case DT_RELR:
                lib->relr = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_RELRSZ:
                relr_total_size = (size_t)dyn->d_un.d_val;
                break;
            case DT_RELRENT:
                relr_entry_size = (size_t)dyn->d_un.d_val;
                break;
            case DT_HASH:
                lib->elf_hash = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_GNU_HASH:
                lib->gnu_hash = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_INIT:
                lib->init_func = (void (*)(void))(lib->bias + dyn->d_un.d_ptr);
                break;
            case DT_FINI:
                lib->fini_func = (void (*)(void))(lib->bias + dyn->d_un.d_ptr);
                break;
            case DT_INIT_ARRAY:
                lib->init_array = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_INIT_ARRAYSZ:
                lib->init_array_count = (size_t)dyn->d_un.d_val / sizeof(void *);
                break;
            case DT_FINI_ARRAY:
                lib->fini_array = mapped_address(lib, dyn->d_un.d_ptr);
                break;
            case DT_FINI_ARRAYSZ:
                lib->fini_array_count = (size_t)dyn->d_un.d_val / sizeof(void *);
                break;
            default:
                break;
        }
    }

    /* The tables are read at the fixed strides of Elf64_Rela, uint64_t and
     * Elf64_Sym. An entry size that disagrees would desync the reads from
     * the file's layout, so it fails the load, as does a DT_RELA or DT_RELR
     * with none. DT_JMPREL must be RELA, as the 64-bit psABIs have it. Each
     * count is a quotient, so it cannot wrap the bounds checks below. */
    bool sizes_ok = true;
    if (lib->rela && rela_entry_size != sizeof(Elf64_Rela))         sizes_ok = false;
    if (lib->jmprel && jmprel_type != DT_RELA)                      sizes_ok = false;
    if (lib->relr && relr_entry_size != sizeof(uint64_t))           sizes_ok = false;
    if (sym_entry_size != 0 && sym_entry_size != sizeof(Elf64_Sym)) sizes_ok = false;
    lib->rela_count   = rela_total_size / sizeof(Elf64_Rela);
    lib->jmprel_count = jmprel_total_size / sizeof(Elf64_Rela);
    lib->relr_count   = relr_total_size / sizeof(uint64_t);

    /* Every table the dynamic section pointed us at must lie inside the mapping.
     * A table that is present but out of range makes the object unsafe to touch,
     * so reject the whole load rather than risk an OOB access at lookup time.
     * (The hash-table interiors and per-symbol accesses are additionally bounds
     * checked at use.) */
    bool tables_ok = sizes_ok;
    if (lib->symtab   && !region_in_bounds(lib, lib->symtab, sizeof(Elf64_Sym)))      tables_ok = false;
    if (lib->strtab   && !region_in_bounds(lib, lib->strtab, lib->strtab_size))       tables_ok = false;
    if (lib->elf_hash && !region_in_bounds(lib, lib->elf_hash, 2 * sizeof(uint32_t))) tables_ok = false;
    if (lib->gnu_hash && !region_in_bounds(lib, lib->gnu_hash, 4 * sizeof(uint32_t))) tables_ok = false;
    if (lib->init_func && !region_in_bounds(lib, (void *)(uintptr_t)lib->init_func, 1)) tables_ok = false;
    if (lib->fini_func && !region_in_bounds(lib, (void *)(uintptr_t)lib->fini_func, 1)) tables_ok = false;
    if (lib->rela && !region_in_bounds(lib, lib->rela,
                                       (uint64_t)lib->rela_count * sizeof(Elf64_Rela))) tables_ok = false;
    if (lib->jmprel && !region_in_bounds(lib, lib->jmprel,
                                       (uint64_t)lib->jmprel_count * sizeof(Elf64_Rela))) tables_ok = false;
    if (lib->relr && !region_in_bounds(lib, lib->relr,
                                       (uint64_t)lib->relr_count * sizeof(uint64_t))) tables_ok = false;
    if (lib->init_array && !region_in_bounds(lib, lib->init_array,
                                       (uint64_t)lib->init_array_count * sizeof(void *))) tables_ok = false;
    if (lib->fini_array && !region_in_bounds(lib, lib->fini_array,
                                       (uint64_t)lib->fini_array_count * sizeof(void *))) tables_ok = false;
    if (!tables_ok) {
        lc_kernel_unmap_memory(base, total_size);
        lc_heap_free(phdrs);
        lc_heap_free(lib);
        return lc_err_ptr(LC_ERR_MALFORMED_ELF);
    }

    /* --- Step 9: Process relocations (mapping still fully RW) --- */
    if (!process_relocations(lib)) {
        lc_kernel_unmap_memory(base, total_size);
        lc_heap_free(phdrs);
        lc_heap_free(lib);
        return lc_err_ptr(LC_ERR_RELOC_FAILED);
    }

    /* --- Step 10: Tighten page permissions now that relocations are done --- */
    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;

        uint64_t seg_start = phdrs[i].p_vaddr - min_vaddr;
        uint64_t seg_end   = seg_start + phdrs[i].p_memsz;

        uint64_t page_start = PAGE_ALIGN_DOWN(seg_start);
        uint64_t page_end   = PAGE_ALIGN_UP(seg_end);

        int32_t prot = prot_from_elf_flags(phdrs[i].p_flags);

        lc_sysret ret = lc_kernel_protect_memory(
            base + page_start,
            (size_t)(page_end - page_start),
            prot
        );
        if (ret < 0) {
            lc_kernel_unmap_memory(base, total_size);
            lc_heap_free(phdrs);
            lc_heap_free(lib);
            return lc_err_ptr(LC_ERR_IO);
        }
    }

    /* PT_GNU_RELRO, which holds the GOT and the other data that only
     * relocation writes, is read-only from here on. As glibc's ld.so does,
     * this protects the whole pages it covers: a partial page at its end
     * keeps the protection of its PT_LOAD. */
    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_GNU_RELRO) continue;

        uint64_t relro_start = phdrs[i].p_vaddr - min_vaddr;
        uint64_t page_start  = PAGE_ALIGN_DOWN(relro_start);
        uint64_t page_end    = PAGE_ALIGN_DOWN(relro_start + phdrs[i].p_memsz);
        if (page_end <= page_start || page_end > total_size) continue;

        if (lc_kernel_protect_memory(base + page_start, (size_t)(page_end - page_start),
                                     PROT_READ) < 0) {
            lc_kernel_unmap_memory(base, total_size);
            lc_heap_free(phdrs);
            lc_heap_free(lib);
            return lc_err_ptr(LC_ERR_IO);
        }
    }

    lc_heap_free(phdrs);

    /* --- Step 11: Run constructors --- */
    if (lib->init_func) {
        lib->init_func();
    }
    if (lib->init_array) {
        for (size_t i = 0; i < lib->init_array_count; i++) {
            if (lib->init_array[i]) {
                typedef void (*init_fn)(void);
                init_fn fn = (init_fn)(uintptr_t)lib->init_array[i];
                fn();
            }
        }
    }

    return lc_ok_ptr(lib);
}

lc_result_ptr lc_library_find_symbol(lc_library *lib, const char *name) {
    if (!lib || !name) return lc_err_ptr(LC_ERR_NOENT);
    if (!lib->symtab || !lib->strtab) {
        return lc_err_ptr(LC_ERR_NOENT);
    }

    void *sym = NULL;

    /* Try GNU hash table first (fast O(1) lookup, most common) */
    if (lib->gnu_hash) {
        sym = find_symbol_with_gnu_hash(lib, name);
    }
    /* Try ELF hash table (older format, still fast) */
    else if (lib->elf_hash) {
        sym = find_symbol_with_elf_hash(lib, name);
    }
    /* Fall back to linear scan */
    else {
        sym = find_symbol_linear_scan(lib, name);
    }

    if (!sym) return lc_err_ptr(LC_ERR_NOENT);
    return lc_ok_ptr(sym);
}

void lc_library_close(lc_library *lib) {
    if (!lib) return;

    /* Run destructors in reverse order */
    if (lib->fini_array) {
        for (size_t i = lib->fini_array_count; i > 0; i--) {
            if (lib->fini_array[i - 1]) {
                typedef void (*fini_fn)(void);
                fini_fn fn = (fini_fn)(uintptr_t)lib->fini_array[i - 1];
                fn();
            }
        }
    }
    if (lib->fini_func) {
        lib->fini_func();
    }

    /* Unmap the loaded library */
    if (lib->base && lib->total_size > 0) {
        lc_kernel_unmap_memory(lib->base, lib->total_size);
    }

    /* Free the library struct */
    lc_heap_free(lib);
}
