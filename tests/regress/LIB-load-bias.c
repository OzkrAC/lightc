/*
 * LIB-load-bias — lc_library_open must translate every address of a plugin
 * by its load bias.
 *
 * The loader maps a plugin's lowest PT_LOAD address, min_vaddr, at the
 * start of its mapping, so the plugin's address v is at mapping + v -
 * min_vaddr. The pointers of the dynamic table were translated so, but
 * relocation offsets, RELATIVE addends and symbol values were taken as
 * mapping + v. For a plugin linked at a nonzero address, every relocation
 * then fell outside the mapping and was skipped, and lc_library_find_symbol
 * gave addresses past it. Every address now goes through the one load
 * bias, mapping - min_vaddr.
 *
 * build/plugins/based.so (tests/plugins/based.c) is linked with
 * -Ttext-segment=0x200000. Its plugin_add(2, 3) must be 5, and its
 * absolute and RELATIVE pointers must reach what they point to. With the
 * defect the address of plugin_value is past the mapping, and this program
 * dies of SIGSEGV. An SHN_ABS symbol's value is no address in the plugin,
 * so a relocation against it takes the value without the bias:
 * build/plugins/abs.so is build/plugins/based.so with plugin_value made
 * absolute, of value 0x12345678 (tests/plugins/corrupt.sh makes it), and
 * its plugin_value_ptr must hold 0x12345678 exactly. Run this from the
 * source root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"

#define PLUGIN PLUGIN_DIR "based.so"

static void test_plugin_is_linked_at_a_nonzero_address(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN));
    const Elf64_Phdr *load = plugin_file_find_phdr(&file, PT_LOAD);
    TEST_REQUIRE_NOT_NULL(load);
    _test_print_text("  first PT_LOAD at ");
    lc_print_hex(TEST_FD, load->p_vaddr);
    _test_print_text("\n");
    TEST_ASSERT(load->p_vaddr != 0);
    plugin_file_free(&file);
}

static void test_symbols_and_relocations(void) {
    lc_result_ptr r = lc_library_open(PLUGIN);
    TEST_REQUIRE_PTR_OK(r);
    lc_library *lib = r.value;

    int *value = plugin_symbol(lib, "plugin_value");
    int **value_ptr = plugin_symbol(lib, "plugin_value_ptr");
    TEST_REQUIRE_NOT_NULL(value);
    TEST_REQUIRE_NOT_NULL(value_ptr);
    TEST_ASSERT_EQ(*value, 7);
    TEST_ASSERT_EQ(*value_ptr, value);          /* R_X86_64_64, R_AARCH64_ABS64 */

    int (*add)(int, int), (*read_value)(void), (*table)(void);
    TEST_REQUIRE(plugin_function(lib, "plugin_add", add));
    TEST_REQUIRE(plugin_function(lib, "plugin_read_value", read_value));
    TEST_REQUIRE(plugin_function(lib, "plugin_table", table));
    TEST_ASSERT_EQ(add(2, 3), 5);
    TEST_ASSERT_EQ(read_value(), 7);
    TEST_ASSERT_EQ(table(), 11);                /* RELATIVE */

    lc_library_close(lib);
}

static void test_absolute_symbol(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "abs.so"));
    TEST_ASSERT(plugin_file_has_rela_to(&file, SHN_ABS, STB_GLOBAL));
    plugin_file_free(&file);

    lc_result_ptr r = lc_library_open(PLUGIN_DIR "abs.so");
    TEST_REQUIRE_PTR_OK(r);
    lc_library *lib = r.value;
    uint64_t *value_ptr = plugin_symbol(lib, "plugin_value_ptr");
    TEST_REQUIRE_NOT_NULL(value_ptr);
    TEST_ASSERT_EQ(*value_ptr, 0x12345678);
    lc_library_close(lib);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_plugin_is_linked_at_a_nonzero_address);
    TEST_RUN(test_symbols_and_relocations);
    TEST_RUN(test_absolute_symbol);
    return test_main();
}
