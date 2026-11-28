/*
 * LIB-p-align — lc_library_open must keep the alignment that a plugin's
 * segments ask for, also beyond the page.
 *
 * The loader put a plugin wherever mmap gave it room, at an address
 * aligned to a page, and ignored p_align. A variable aligned to 64 KiB,
 * whose PT_LOAD ld gives a p_align of 0x10000, then sat at an address
 * aligned to 4 KiB only, where the compiler had taken 64 KiB as given. The
 * loader now places its mapping at the largest p_align of the PT_LOADs.
 *
 * build/plugins/align.so (tests/plugins/align.c) has such a variable. It
 * is loaded 16 times, each copy staying open, with a spacer mapping of a
 * different size before each, so that each lands at its own address: the
 * variable must be 64 KiB-aligned in every copy, with its first byte
 * loaded. With the defect, on 4 KiB pages, most copies misalign it; on
 * 64 KiB pages it cannot be misaligned. Run this from the source root, as
 * ninja regress does.
 */

#include "test.h"
#include "plugins.h"
#include <lightc/syscall.h>

#define COPIES 16

static void test_over_aligned_data_stays_aligned(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "align.so"));
    uint64_t max_align = 0;
    const Elf64_Phdr *phdr;
    for (size_t i = 0; (phdr = plugin_file_phdr(&file, i)) != NULL; i++)
        if (phdr->p_type == PT_LOAD && phdr->p_align > max_align) max_align = phdr->p_align;
    plugin_file_free(&file);
    TEST_REQUIRE(max_align >= 0x10000);   /* else the toolchain ignored aligned(65536) */

    lc_library *libs[COPIES] = {0};
    void *spacers[COPIES] = {0};
    int misaligned = 0, unloaded = 0;
    for (int i = 0; i < COPIES; i++) {
        size_t spacer = (size_t)(i + 1) * 4096;
        spacers[i] = lc_kernel_map_memory(NULL, spacer, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        lc_result_ptr r = lc_library_open(PLUGIN_DIR "align.so");
        TEST_REQUIRE(lc_ptr_is_ok(r));
        libs[i] = r.value;

        void *(*address)(void);
        int (*first)(void);
        TEST_REQUIRE(plugin_function(libs[i], "plugin_aligned_addr", address));
        TEST_REQUIRE(plugin_function(libs[i], "plugin_aligned_first", first));
        if (((uintptr_t)address() & 0xffff) != 0) misaligned++;
        if (first() != 1) unloaded++;
    }
    _test_print_text("  copies with the variable not 64 KiB-aligned: ");
    lc_print_signed(TEST_FD, misaligned);
    _test_print_text(" of 16\n");
    TEST_ASSERT_EQ(misaligned, 0);
    TEST_ASSERT_EQ(unloaded, 0);

    for (int i = 0; i < COPIES; i++) {
        if (libs[i]) lc_library_close(libs[i]);
        if (spacers[i] != MAP_FAILED && spacers[i] != NULL)
            lc_kernel_unmap_memory(spacers[i], (size_t)(i + 1) * 4096);
    }
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_over_aligned_data_stays_aligned);
    return test_main();
}
