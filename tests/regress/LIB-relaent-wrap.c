/*
 * LIB-relaent-wrap — lc_library_open must check DT_RELAENT.
 *
 * The loader counted the entries of DT_RELA as DT_RELASZ / DT_RELAENT,
 * both from the file, read them at the 24-byte stride of an Elf64_Rela,
 * and checked the table against its mapping as count * 24 bytes, a product
 * that could wrap. With DT_RELAENT 1 and DT_RELASZ 768614336404564651,
 * count * 24 is 2^64 + 8, so the table passed as 8 bytes long, and the
 * loader read entries past its mapping until it faulted. It now requires
 * DT_RELAENT, DT_RELRENT and DT_SYMENT to be the sizes it reads at, and
 * counts by those.
 *
 * build/plugins/relaent1.so is build/plugins/plt.so with those two values
 * (tests/plugins/corrupt.sh makes it). It must fail with
 * LC_ERR_MALFORMED_ELF; with the defect this program dies of SIGSEGV in
 * lc_library_open. Run this from the source root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"

#define PLUGIN PLUGIN_DIR "relaent1.so"

static void test_plugin_has_the_wrapping_sizes(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN));
    uint64_t entry_size = 0, size = 0;
    TEST_ASSERT(plugin_file_dynamic(&file, DT_RELAENT, &entry_size));
    TEST_ASSERT(plugin_file_dynamic(&file, DT_RELASZ, &size));
    TEST_ASSERT_EQ(entry_size, (uint64_t)1);
    TEST_ASSERT_EQ(size * sizeof(Elf64_Rela), (uint64_t)8);
    plugin_file_free(&file);
}

static void test_bad_entry_size_is_refused(void) {
    lc_result_ptr r = lc_library_open(PLUGIN);
    if (!lc_ptr_is_err(r)) lc_library_close(r.value);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_MALFORMED_ELF);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_plugin_has_the_wrapping_sizes);
    TEST_RUN(test_bad_entry_size_is_refused);
    return test_main();
}
