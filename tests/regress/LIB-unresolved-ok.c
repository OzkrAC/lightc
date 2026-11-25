/*
 * LIB-unresolved-ok — lc_library_open must fail on a relocation it cannot
 * apply.
 *
 * The loader skipped a relocation against an undefined symbol that is not
 * weak, one of a type it does not know and one whose target is outside
 * the mapping, and reported success: LC_ERR_RELOC_FAILED was never
 * returned. The plugin then followed a GOT slot left 0, or a pointer left
 * at its link-time value, far from the cause. Each of those now fails the
 * load with LC_ERR_RELOC_FAILED.
 *
 * build/plugins/imp.so (tests/plugins/imp.c) imports host_value and
 * host_func, and lc_library_open has no other object to find them in.
 * build/plugins/unknown_reloc.so is build/plugins/plt.so with the type of
 * its first relocation made 32767, which neither psABI defines, and
 * build/plugins/wild_reloc.so the same with that relocation's target
 * 1 GiB past the plugin (tests/plugins/corrupt.sh makes both). Each must
 * fail with LC_ERR_RELOC_FAILED. With the defect imp.so loads, and so do
 * the corrupt ones, or, where the relocation they spoil is the one of
 * plt.so's constructor in .init_array, as GNU ld lays it out, this program
 * dies of SIGSEGV in lc_library_open. A NONE relocation has no target,
 * so it is skipped wherever it points: build/plugins/none_reloc.so is
 * build/plugins/based.so, linked at 0x200000, with its first relocation
 * made all zeros, a NONE of target 0, below the plugin, and it must load.
 * A weak import that nothing defines is 0: build/plugins/weak.so, whose one
 * import is weak, must load, and its plugin_has_weak must return 0. Run
 * this from the source root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"

static lc_result_ptr open_and_report(const char *path) {
    lc_result_ptr r = lc_library_open(path);
    _test_print_text("  ");
    _test_print_text(path);
    _test_print_text(": ");
    _test_print_text(lc_error_name(r.error));
    _test_print_text("\n");
    if (!lc_ptr_is_err(r)) lc_library_close(r.value);
    return r;
}

static void test_unresolved_import(void) {
    lc_result_ptr r = open_and_report(PLUGIN_DIR "imp.so");
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_RELOC_FAILED);
}

static void test_unknown_relocation_type(void) {
    lc_result_ptr r = open_and_report(PLUGIN_DIR "unknown_reloc.so");
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_RELOC_FAILED);
}

static void test_target_outside_the_mapping(void) {
    lc_result_ptr r = open_and_report(PLUGIN_DIR "wild_reloc.so");
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_RELOC_FAILED);
}

static void test_none_relocation_outside_the_mapping(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "none_reloc.so"));
    const Elf64_Phdr *load = plugin_file_find_phdr(&file, PT_LOAD);
    TEST_REQUIRE_NOT_NULL(load);
    TEST_ASSERT(load->p_vaddr != 0);
    uint64_t address;
    TEST_REQUIRE(plugin_file_dynamic(&file, DT_RELA, &address));
    const uint8_t *at = plugin_file_at(&file, address, sizeof(Elf64_Rela));
    TEST_REQUIRE_NOT_NULL(at);
    Elf64_Rela first;
    lc_bytes_copy(&first, at, sizeof first);
    TEST_ASSERT_EQ(first.r_offset, 0);
    TEST_ASSERT_EQ(first.r_info, 0);
    plugin_file_free(&file);

    lc_result_ptr r = open_and_report(PLUGIN_DIR "none_reloc.so");
    TEST_ASSERT_PTR_OK(r);
}

static void test_weak_import(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "weak.so"));
    TEST_ASSERT(plugin_file_has_rela_to(&file, SHN_UNDEF, STB_WEAK));
    plugin_file_free(&file);

    lc_result_ptr r = lc_library_open(PLUGIN_DIR "weak.so");
    TEST_REQUIRE_PTR_OK(r);
    int (*has_weak)(void);
    TEST_REQUIRE(plugin_function(r.value, "plugin_has_weak", has_weak));
    TEST_ASSERT_EQ(has_weak(), 0);
    lc_library_close(r.value);
}

/* The plugin they are made from loads: what fails is the corruption. */
static void test_the_uncorrupted_plugin_loads(void) {
    lc_result_ptr r = open_and_report(PLUGIN_DIR "plt.so");
    TEST_ASSERT_PTR_OK(r);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_unresolved_import);
    TEST_RUN(test_unknown_relocation_type);
    TEST_RUN(test_target_outside_the_mapping);
    TEST_RUN(test_none_relocation_outside_the_mapping);
    TEST_RUN(test_weak_import);
    TEST_RUN(test_the_uncorrupted_plugin_loads);
    return test_main();
}
