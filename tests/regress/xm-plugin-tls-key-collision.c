/*
 * xm-plugin-tls-key-collision — lc_library_open must refuse a plugin that
 * has its own copy of lightc.
 *
 * Such a plugin, linked with liblightc.a, keeps lightc's globals of its
 * own but finds its per-thread block through the thread pointer, which is
 * the host's. Its count of TLS keys started at 0, so its first key was the
 * host's key 0, which it overwrote, and its heap linked its segments into
 * the host's per-thread heap state. lc_library_open now refuses a library
 * that defines lc_heap_*, lc_tls_* or lc_abi_* symbols, with
 * LC_ERR_UNSUPPORTED.
 *
 * build/plugins/embedded.so (tests/plugins/embedded.c) is such a plugin.
 * The host makes TLS key 0 and sets it to 0xaaaa: the plugin must be
 * refused, and the key must still read 0xaaaa. With the defect the plugin
 * loads, and this program calls its plugin_tls_init, which sets its own
 * first key, the host's 0, to 0x1234. Run this from the source root, as
 * ninja regress does.
 */

#include "test.h"
#include "plugins.h"
#include <lightc/thread.h>

static void test_plugin_with_its_own_lightc_is_refused(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN_DIR "embedded.so"));
    Elf64_Sym sym;
    bool has = plugin_file_symbol(&file, "lc_tls_key_create", &sym);
    plugin_file_free(&file);
    TEST_REQUIRE(has && sym.st_shndx != SHN_UNDEF);   /* its own copy, not an import */

    lc_tls_key key;
    TEST_REQUIRE(lc_tls_key_create(&key));
    TEST_REQUIRE_EQ(key, (lc_tls_key)0);   /* the plugin's first key is 0 too */
    lc_tls_set(key, (void *)0xaaaa);

    lc_result_ptr r = lc_library_open(PLUGIN_DIR "embedded.so");
    if (lc_ptr_is_ok(r)) {
        int (*tls_init)(void) = NULL;
        if (plugin_function(r.value, "plugin_tls_init", tls_init)) (void)tls_init();
        lc_library_close(r.value);
    }
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_UNSUPPORTED);
    TEST_ASSERT_EQ((uintptr_t)lc_tls_get(key), (uintptr_t)0xaaaa);   /* the defect: 0x1234 */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_plugin_with_its_own_lightc_is_refused);
    return test_main();
}
