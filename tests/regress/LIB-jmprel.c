/*
 * LIB-jmprel — lc_library_open must apply a plugin's PLT relocations.
 *
 * GNU ld puts the R_*_JUMP_SLOT relocations of a shared object's PLT in
 * .rela.plt, which only DT_JMPREL and DT_PLTRELSZ give: DT_RELA covers
 * .rela.dyn. The loader applied DT_RELA alone, so each PLT slot kept its
 * link-time value, and the first call through the PLT, which -fPIC makes
 * of every call to a function of default visibility, even one the plugin
 * defines itself, jumped to that address and faulted. It now applies
 * DT_JMPREL too, at once, as it does no lazy binding.
 *
 * build/plugins/plt.so (tests/plugins/plt.c) has plugin_call(x), which
 * returns helper(x) + 1 through the PLT: plugin_call(5) must be 16. With
 * the defect that call faults, and this program dies of SIGSEGV. The
 * plugin's other relocations, its constructor and its destructor are
 * checked too. Run this from the source root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"

#define PLUGIN PLUGIN_DIR "plt.so"

static void test_plugin_has_plt_relocations(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN));
    uint64_t address = 0, size = 0, type = 0;
    TEST_ASSERT(plugin_file_dynamic(&file, DT_JMPREL, &address));
    TEST_ASSERT(plugin_file_dynamic(&file, DT_PLTRELSZ, &size));
    TEST_ASSERT(size >= sizeof(Elf64_Rela));
    TEST_ASSERT(plugin_file_dynamic(&file, DT_PLTREL, &type));
    TEST_ASSERT_EQ(type, (uint64_t)DT_RELA);
    plugin_file_free(&file);
}

static void test_other_relocations_constructor_and_destructor(void) {
    lc_result_ptr r = lc_library_open(PLUGIN);
    TEST_REQUIRE_PTR_OK(r);
    lc_library *lib = r.value;

    int *ctor_ran = plugin_symbol(lib, "ctor_ran");
    TEST_REQUIRE_NOT_NULL(ctor_ran);
    TEST_ASSERT_EQ(*ctor_ran, 1234);

    int (*read_counter)(void), (*counter_via_ptr)(void), (*table)(int);
    TEST_REQUIRE(plugin_function(lib, "plugin_read_counter", read_counter));
    TEST_REQUIRE(plugin_function(lib, "plugin_counter_via_ptr", counter_via_ptr));
    TEST_REQUIRE(plugin_function(lib, "plugin_table", table));
    TEST_ASSERT_EQ(read_counter(), 7);          /* GLOB_DAT */
    TEST_ASSERT_EQ(counter_via_ptr(), 7);       /* R_X86_64_64, R_AARCH64_ABS64 */
    TEST_ASSERT_EQ(table(0), 11);               /* RELATIVE */
    TEST_ASSERT_EQ(table(1), 22);

    int flag = 0;
    int **dtor_flag = plugin_symbol(lib, "dtor_flag");
    TEST_REQUIRE_NOT_NULL(dtor_flag);
    *dtor_flag = &flag;
    lc_library_close(lib);
    TEST_ASSERT_EQ(flag, 5678);
}

static void test_call_through_the_plt(void) {
    lc_result_ptr r = lc_library_open(PLUGIN);
    TEST_REQUIRE_PTR_OK(r);
    lc_library *lib = r.value;

    int (*helper)(int), (*call)(int);
    TEST_REQUIRE(plugin_function(lib, "helper", helper));
    TEST_REQUIRE(plugin_function(lib, "plugin_call", call));
    TEST_ASSERT_EQ(helper(5), 15);
    _test_print_text("  plugin_call(5), which calls helper through the PLT\n");
    TEST_ASSERT_EQ(call(5), 16);

    lc_library_close(lib);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_plugin_has_plt_relocations);
    TEST_RUN(test_other_relocations_constructor_and_destructor);
    TEST_RUN(test_call_through_the_plt);
    return test_main();
}
