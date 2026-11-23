/*
 * LIB-pagesize — lc_library_open must align to the kernel's page size.
 *
 * The loader aligned its mapping and every mprotect to 4096 bytes. An
 * aarch64 kernel may have 16 or 64 KiB pages, and its mprotect fails with
 * EINVAL on an address that is not aligned to one. ld lays out an aarch64
 * plugin for 64 KiB pages, with its RW segment inside one, so on such a
 * kernel every plugin failed to load, with LC_ERR_IO. The loader now
 * aligns to lc_runtime_page_size(), the kernel's AT_PAGESZ. Two PT_LOADs
 * with different protections on one of those pages, which no mprotect can
 * give both, fail the load with LC_ERR_MALFORMED_ELF.
 *
 * This kernel's pages may be 4 KiB, so this program compiles its own copy
 * of src/library.c in which mmap and mprotect act as on a kernel with
 * larger pages: mmap gives an address aligned to the emulated page, and
 * mprotect fails with EINVAL on one that is not, as mm/mprotect.c does.
 * lc_runtime_init gets an auxiliary vector with that page size as
 * AT_PAGESZ, as _start passes it the kernel's. Each of 4, 16 and 64 KiB
 * that is not smaller than this kernel's page is emulated in turn.
 * build/plugins/page64k.so (tests/plugins/page.c), linked for 64 KiB
 * pages, must then load and work. build/plugins/page4k.so, the same linked
 * for 4 KiB pages, has its code and its data on one page larger than that:
 * it must load where pages are 4 KiB and fail with LC_ERR_MALFORMED_ELF
 * elsewhere. With the defect page64k.so fails with LC_ERR_IO on 16 and
 * 64 KiB pages. Run this from the source root, as ninja regress does.
 */

#include <lightc/heap.h>
#include <lightc/library.h>
#include <lightc/memory.h>
#include <lightc/string.h>
#include <lightc/syscall.h>

static uint64_t emulated_page = 4096;

/* mmap on a kernel with emulated_page pages: the length is a whole number
 * of pages, and the address is aligned to one. */
static void *emulated_map_memory(void *addr, size_t len, int32_t prot, int32_t flags,
                                 int32_t fd, int64_t offset) {
    size_t length = (len + emulated_page - 1) & ~(size_t)(emulated_page - 1);
    size_t span = length + emulated_page;
    uint8_t *p = lc_kernel_map_memory(addr, span, prot, flags, fd, offset);
    if (p == MAP_FAILED) return p;
    uint8_t *aligned = (uint8_t *)(((uintptr_t)p + emulated_page - 1) & ~(uintptr_t)(emulated_page - 1));
    if (aligned > p) lc_kernel_unmap_memory(p, (size_t)(aligned - p));
    if (p + span > aligned + length) lc_kernel_unmap_memory(aligned + length, (size_t)(p + span - (aligned + length)));
    return aligned;
}

/* mprotect on such a kernel: EINVAL for an address that is not aligned to
 * a page. */
static lc_sysret emulated_protect_memory(void *addr, size_t len, int32_t prot) {
    if ((uintptr_t)addr & (emulated_page - 1)) return -(lc_sysret)LC_ERR_INVAL;
    return lc_kernel_protect_memory(addr, len, prot);
}

#define lc_kernel_map_memory     emulated_map_memory
#define lc_kernel_protect_memory emulated_protect_memory
#include "../../src/library.c"
#undef lc_kernel_map_memory
#undef lc_kernel_protect_memory

#include "test.h"
#include "plugins.h"

/* As _start does with the kernel's auxiliary vector: an entry stack with no
 * argument and no environment, then AT_PAGESZ. */
static void set_runtime_page_size(uint64_t size) {
    uint64_t stack[] = { 0, 0, 0, LC_AT_PAGESZ, size, LC_AT_NULL, 0 };
    lc_runtime_init(stack);
}

static size_t kernel_page;

static lc_result_ptr open_with_page_size(const char *path, uint64_t page) {
    emulated_page = page;
    set_runtime_page_size(page);
    lc_result_ptr r = lc_library_open(path);
    set_runtime_page_size(kernel_page);
    _test_print_text("  ");
    _test_print_text(path);
    _test_print_text(" with ");
    lc_print_signed(TEST_FD, (int64_t)(page / 1024));
    _test_print_text(" KiB pages: ");
    _test_print_text(lc_error_name(r.error));
    _test_print_text("\n");
    return r;
}

/* The plugin's code, its data and its RELRO work: the table in its RELRO
 * holds the address of one(), which plugin_one_address gives. */
static void check_plugin_works(lc_library *lib) {
    typedef int (*one_fn)(void);
    int (*add)(int, int);
    one_fn (*one_address)(void);
    TEST_REQUIRE(plugin_function(lib, "plugin_add", add));
    TEST_REQUIRE(plugin_function(lib, "plugin_one_address", one_address));
    const one_fn *table = plugin_symbol(lib, "plugin_const_table");
    int *data = plugin_symbol(lib, "plugin_data");
    TEST_REQUIRE_NOT_NULL(table);
    TEST_REQUIRE_NOT_NULL(data);
    TEST_ASSERT_EQ(add(2, 3), 5);
    TEST_ASSERT(table[0] == one_address());
    TEST_ASSERT_EQ(*data, 7);
    *data = 8;
    TEST_ASSERT_EQ(*data, 8);
}

/* page64k.so must have a PT_LOAD whose 4 KiB page is not aligned to
 * 16 KiB, or the 4096-byte alignment of the defect would pass too. */
static void test_page64k_has_a_segment_inside_a_large_page(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "page64k.so"));
    bool inside = false;
    const Elf64_Phdr *phdr;
    for (size_t i = 0; (phdr = plugin_file_phdr(&file, i)) != NULL; i++)
        if (phdr->p_type == PT_LOAD && phdr->p_vaddr % 16384 >= 4096) inside = true;
    TEST_ASSERT(inside);
    plugin_file_free(&file);
}

static void test_page64k_loads_with_every_page_size(void) {
    for (uint64_t page = 4096; page <= 65536; page *= 4) {
        if (page < kernel_page) continue;
        lc_result_ptr r = open_with_page_size(PLUGIN_DIR "page64k.so", page);
        TEST_ASSERT_PTR_OK(r);
        if (lc_ptr_is_err(r)) continue;
        check_plugin_works(r.value);
        lc_library_close(r.value);
    }
}

/* page4k.so must have PT_LOADs with different protections that start in
 * its first 16 KiB, so that they share any larger page. */
static void test_page4k_shares_larger_pages(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "page4k.so"));
    const Elf64_Phdr *first = plugin_file_find_phdr(&file, PT_LOAD), *phdr;
    TEST_REQUIRE_NOT_NULL(first);
    bool other_flags = false;
    for (size_t i = 0; (phdr = plugin_file_phdr(&file, i)) != NULL; i++) {
        if (phdr->p_type != PT_LOAD) continue;
        TEST_ASSERT(phdr->p_vaddr < 16384);
        if (phdr->p_flags != first->p_flags) other_flags = true;
    }
    TEST_ASSERT(other_flags);
    plugin_file_free(&file);
}

static void test_page4k_loads_only_with_4k_pages(void) {
    for (uint64_t page = 4096; page <= 65536; page *= 4) {
        if (page < kernel_page) continue;
        lc_result_ptr r = open_with_page_size(PLUGIN_DIR "page4k.so", page);
        if (page == 4096) {
            TEST_ASSERT_PTR_OK(r);
            if (!lc_ptr_is_err(r)) check_plugin_works(r.value);
        } else {
            TEST_ASSERT_PTR_ERR(r);
            TEST_ASSERT_EQ(r.error, LC_ERR_MALFORMED_ELF);
        }
        if (!lc_ptr_is_err(r)) lc_library_close(r.value);
    }
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    kernel_page = lc_runtime_page_size();
    _test_print_text("  this kernel's pages: ");
    lc_print_signed(TEST_FD, (int64_t)(kernel_page / 1024));
    _test_print_text(" KiB\n");
    TEST_RUN(test_page64k_has_a_segment_inside_a_large_page);
    TEST_RUN(test_page64k_loads_with_every_page_size);
    TEST_RUN(test_page4k_shares_larger_pages);
    TEST_RUN(test_page4k_loads_only_with_4k_pages);
    return test_main();
}
