/*
 * LIB-tls — lc_library_open must refuse a plugin with thread-local storage.
 *
 * The loader ignored PT_TLS and skipped the TLS relocations, and reported
 * success. It makes no TLS block for a plugin, so the plugin's
 * thread-local variables had no storage. With the initial-exec model the
 * plugin finds such a variable at the thread pointer plus an offset that a
 * relocation puts in its GOT, and left at 0 that is the word at the thread
 * pointer: lightc's own per-thread heap data, which one increment of the
 * plugin's counter corrupted, so that the next heap allocation crashed.
 * With the general-dynamic model the first access called an unresolved
 * __tls_get_addr or TLS descriptor. A plugin with PT_TLS now fails to load
 * with LC_ERR_MALFORMED_ELF.
 *
 * build/plugins/tls_ie.so (tests/plugins/tls_ie.c) uses the initial-exec
 * model, and build/plugins/tls_gd.so (tests/plugins/tls_gd.c) the
 * general-dynamic one. Both must be refused, and the word at the thread
 * pointer must be unchanged after the attempt to load tls_ie.so. With the
 * defect tls_ie.so loads, and this program calls its plugin_tls_bump, which
 * changes that word, and then puts the word back, so that the heap still
 * works. Run this from the source root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"

/* The word at the thread pointer: on x86_64 at %fs:0, on aarch64 at the
 * address in TPIDR_EL0. */
static uint64_t thread_pointer_word(void) {
    uint64_t word;
#if defined(__x86_64__)
    __asm__ volatile("movq %%fs:0, %0" : "=r"(word));
#elif defined(__aarch64__)
    uint64_t *tp;
    __asm__ volatile("mrs %0, tpidr_el0" : "=r"(tp));
    word = *(volatile uint64_t *)tp;
#endif
    return word;
}

static void set_thread_pointer_word(uint64_t word) {
#if defined(__x86_64__)
    __asm__ volatile("movq %0, %%fs:0" : : "r"(word) : "memory");
#elif defined(__aarch64__)
    uint64_t *tp;
    __asm__ volatile("mrs %0, tpidr_el0" : "=r"(tp));
    *(volatile uint64_t *)tp = word;
#endif
}

static void require_pt_tls(const char *path) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, path));
    TEST_ASSERT_NOT_NULL(plugin_file_find_phdr(&file, PT_TLS));
    plugin_file_free(&file);
}

static void test_initial_exec_is_refused(void) {
    require_pt_tls(PLUGIN_DIR "tls_ie.so");

    /* The heap's per-thread data must be set up before the word is read:
     * lc_library_open allocates. */
    lc_result_ptr block = lc_heap_allocate(32);
    TEST_REQUIRE_PTR_OK(block);

    uint64_t before = thread_pointer_word();
    lc_result_ptr r = lc_library_open(PLUGIN_DIR "tls_ie.so");
    int (*bump)(void);
    if (!lc_ptr_is_err(r) && plugin_function(r.value, "plugin_tls_bump", bump)) {
        _test_print_text("  tls_ie.so loaded; calling its plugin_tls_bump\n");
        (void)bump();
    }
    uint64_t after = thread_pointer_word();
    set_thread_pointer_word(before);

    _test_print_text("  word at the thread pointer: before ");
    lc_print_hex(TEST_FD, before);
    _test_print_text(", after ");
    lc_print_hex(TEST_FD, after);
    _test_print_text("\n");
    if (!lc_ptr_is_err(r)) lc_library_close(r.value);
    lc_heap_free(block.value);

    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_MALFORMED_ELF);
    TEST_ASSERT_EQ(after, before);
}

static void test_general_dynamic_is_refused(void) {
    require_pt_tls(PLUGIN_DIR "tls_gd.so");

    lc_result_ptr r = lc_library_open(PLUGIN_DIR "tls_gd.so");
    if (!lc_ptr_is_err(r)) lc_library_close(r.value);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_MALFORMED_ELF);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_initial_exec_is_refused);
    TEST_RUN(test_general_dynamic_is_refused);
    return test_main();
}
