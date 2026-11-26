/*
 * LIB-symvalue — lc_library_find_symbol must give a symbol the address a
 * relocation against it gets, and refuse one it has no address for.
 *
 * The lookups added the load bias to every symbol's value. An absolute
 * symbol (SHN_ABS), whose value is the address itself, came back moved by
 * the bias. An IFUNC symbol, whose value is the address of its resolver,
 * came back as that, so a caller of the function called the resolver,
 * which returns the function's address instead of running it, and a
 * relocation against an IFUNC symbol bound it to the resolver too.
 * Lookups and relocations now share one helper: an absolute symbol keeps
 * its value, and an IFUNC or TLS symbol fails with LC_ERR_UNSUPPORTED, in
 * a lookup and as the symbol of a relocation alike. An IRELATIVE
 * relocation fails the load with it too.
 *
 * build/plugins/abs.so is build/plugins/based.so with plugin_value made
 * absolute, of value 0x12345678 (tests/plugins/corrupt.sh makes it).
 * build/plugins/ifunc_symbol.so (tests/plugins/ifunc_symbol.c) exports an
 * IFUNC, fn, that nothing in it refers to, so it loads.
 * build/plugins/ifunc.so (tests/plugins/ifunc.c) uses its IFUNCs itself.
 * Run this from the source root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"

#if defined(__x86_64__)
#define IRELATIVE R_X86_64_IRELATIVE
#elif defined(__aarch64__)
#define IRELATIVE R_AARCH64_IRELATIVE
#endif

static void test_absolute_symbol_keeps_its_value(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "abs.so"));
    Elf64_Sym sym;
    bool has = plugin_file_symbol(&file, "plugin_value", &sym);
    plugin_file_free(&file);
    TEST_REQUIRE(has && sym.st_shndx == SHN_ABS && sym.st_value == 0x12345678);

    lc_result_ptr r = lc_library_open(PLUGIN_DIR "abs.so");
    TEST_REQUIRE(lc_ptr_is_ok(r));
    lc_result_ptr s = lc_library_find_symbol(r.value, "plugin_value");
    TEST_ASSERT_PTR_OK(s);
    TEST_ASSERT_EQ((uintptr_t)s.value, (uintptr_t)0x12345678);
    lc_library_close(r.value);
}

static void test_ifunc_symbol_has_no_address(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "ifunc_symbol.so"));
    Elf64_Sym sym;
    bool has = plugin_file_symbol(&file, "fn", &sym);
    plugin_file_free(&file);
    TEST_REQUIRE(has && ELF64_ST_TYPE(sym.st_info) == STT_GNU_IFUNC);

    lc_result_ptr r = lc_library_open(PLUGIN_DIR "ifunc_symbol.so");
    TEST_REQUIRE(lc_ptr_is_ok(r));
    lc_result_ptr s = lc_library_find_symbol(r.value, "fn");
    TEST_ASSERT_PTR_ERR(s);   /* the defect: the resolver's address */
    TEST_ASSERT_EQ(s.error, LC_ERR_UNSUPPORTED);

    int (*plain)(int) = NULL;
    TEST_ASSERT(plugin_function(r.value, "plugin_plain", plain));
    if (plain) TEST_ASSERT_EQ(plain(2), 3);
    lc_library_close(r.value);
}

static void test_plugin_that_uses_its_ifuncs_is_refused(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "ifunc.so"));
    bool has = plugin_file_has_rela_type(&file, IRELATIVE);
    plugin_file_free(&file);
    TEST_REQUIRE(has);

    lc_result_ptr r = lc_library_open(PLUGIN_DIR "ifunc.so");
    if (lc_ptr_is_ok(r)) lc_library_close(r.value);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_UNSUPPORTED);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_absolute_symbol_keeps_its_value);
    TEST_RUN(test_ifunc_symbol_has_no_address);
    TEST_RUN(test_plugin_that_uses_its_ifuncs_is_refused);
    return test_main();
}
