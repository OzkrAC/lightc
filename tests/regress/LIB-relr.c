/*
 * LIB-relr — lc_library_open must apply DT_RELR.
 *
 * ld -z pack-relative-relocs (binutils 2.38 and later, and lld) moves the
 * relative relocations of a shared object from DT_RELA to DT_RELR, a
 * packed list of the words to relocate. The loader did not read DT_RELR,
 * so those words kept their link-time values: pointer tables pointed
 * outside the mapping, and lc_library_open called the constructor in
 * .init_array at its link-time address and faulted. It now applies
 * DT_RELR, before DT_RELA.
 *
 * build/plugins/relr.so (tests/plugins/relr.c) is linked so, and its
 * DT_RELR has address entries and several bitmaps. Opening it must run
 * its constructor, which sets ctor_ran to 1234, and plugin_table must call
 * through its two relocated pointers. plugin_word(i) must be i for words
 * in each of the three bitmaps that cover its 136 pointers. With the
 * defect this program dies of SIGSEGV in lc_library_open. Run this from
 * the source root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"

#define PLUGIN PLUGIN_DIR "relr.so"

static void test_plugin_has_relr(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN));
    uint64_t address = 0, size = 0;
    TEST_ASSERT(plugin_file_dynamic(&file, DT_RELR, &address));
    TEST_ASSERT(plugin_file_dynamic(&file, DT_RELRSZ, &size));
    /* At least one address entry and the three bitmaps of words. */
    TEST_ASSERT(size >= 4 * sizeof(uint64_t));
    plugin_file_free(&file);
}

static void test_constructor_and_table(void) {
    _test_print_text("  opening " PLUGIN ", which runs its constructor\n");
    lc_result_ptr r = lc_library_open(PLUGIN);
    TEST_REQUIRE_PTR_OK(r);
    lc_library *lib = r.value;

    int *ctor_ran = plugin_symbol(lib, "ctor_ran");
    TEST_REQUIRE_NOT_NULL(ctor_ran);
    TEST_ASSERT_EQ(*ctor_ran, 1234);

    int (*table)(int);
    TEST_REQUIRE(plugin_function(lib, "plugin_table", table));
    TEST_ASSERT_EQ(table(0), 11);
    TEST_ASSERT_EQ(table(1), 22);

    lc_library_close(lib);
}

/* Words in the first bitmap, at the step to the second and the third, and
 * at the end. */
static void test_words_of_every_bitmap(void) {
    lc_result_ptr r = lc_library_open(PLUGIN);
    TEST_REQUIRE_PTR_OK(r);
    lc_library *lib = r.value;

    int (*count)(void), (*word)(int);
    TEST_REQUIRE(plugin_function(lib, "plugin_word_count", count));
    TEST_REQUIRE(plugin_function(lib, "plugin_word", word));
    TEST_REQUIRE_EQ(count(), 136);
    static const int checked[] = { 0, 1, 62, 63, 64, 65, 126, 127, 128, 135 };
    for (size_t i = 0; i < sizeof(checked) / sizeof(checked[0]); i++)
        TEST_ASSERT_EQ(word(checked[i]), checked[i]);

    lc_library_close(lib);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_plugin_has_relr);
    TEST_RUN(test_constructor_and_table);
    TEST_RUN(test_words_of_every_bitmap);
    return test_main();
}
