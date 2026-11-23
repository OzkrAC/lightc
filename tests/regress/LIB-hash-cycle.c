/*
 * LIB-hash-cycle — a symbol lookup must end on a DT_HASH chain that loops.
 *
 * lc_library_find_symbol walked a DT_HASH chain for as long as each index
 * was below nchain and its slot readable. A chain that leads back to
 * itself meets both for ever, so the lookup of a name the object lacks
 * never returned. The walk now stops after nchain steps: a valid chain
 * visits each symbol at most once.
 *
 * build/plugins/hash_cycle.so is examples/test_plugin.c linked with
 * DT_HASH alone, every bucket of which then goes to symbol 1, and the
 * chain from there through every symbol and back to 1
 * (tests/plugins/corrupt.sh makes it). A child process looks up a name the
 * plugin does not define: it must get LC_ERR_NOENT within 5 s. With the
 * defect it never returns, and the parent kills it. The plugin's own names
 * are on the loop, so they must still be found. Run this from the source
 * root, as ninja regress does.
 */

#include "test.h"
#include "plugins.h"
#include <lightc/syscall.h>
#include <lightc/time.h>

#define PLUGIN  PLUGIN_DIR "hash_cycle.so"
#define WAIT_MS 5000

/* The child's exit statuses. */
#define CHILD_NOENT     0       /* the lookup gave LC_ERR_NOENT */
#define CHILD_FOUND     1       /* it found the name */
#define CHILD_NO_OPEN   2       /* lc_library_open failed */
#define CHILD_OTHER     3       /* the lookup gave another error */

static void test_plugin_has_dt_hash_alone(void) {
    plugin_file file;
    TEST_REQUIRE(plugin_file_read(&file, PLUGIN));
    uint64_t address = 0;
    TEST_ASSERT(plugin_file_dynamic(&file, DT_HASH, &address));
    TEST_ASSERT(!plugin_file_dynamic(&file, DT_GNU_HASH, &address));
    plugin_file_free(&file);
}

static void test_names_on_the_loop_are_found(void) {
    lc_result_ptr r = lc_library_open(PLUGIN);
    TEST_REQUIRE_PTR_OK(r);
    int (*add)(int, int);
    TEST_ASSERT(plugin_function(r.value, "plugin_add", add));
    TEST_ASSERT_NOT_NULL(plugin_symbol(r.value, "plugin_global"));
    lc_library_close(r.value);
}

static int32_t child_lookup(void) {
    lc_result_ptr r = lc_library_open(PLUGIN);
    if (lc_ptr_is_err(r)) return CHILD_NO_OPEN;
    lc_result_ptr symbol = lc_library_find_symbol(r.value, "no_such_symbol");
    if (!lc_ptr_is_err(symbol)) return CHILD_FOUND;
    return symbol.error == LC_ERR_NOENT ? CHILD_NOENT : CHILD_OTHER;
}

static void test_missing_name_lookup_ends(void) {
    lc_sysret pid = lc_kernel_fork();
    TEST_REQUIRE(pid >= 0);
    if (pid == 0) lc_kernel_exit(child_lookup());

    int32_t status = 0;
    lc_sysret reaped;
    int64_t start = lc_time_start_timer();
    while ((reaped = lc_kernel_wait_for_child((int32_t)pid, &status, WNOHANG)) == 0
           && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(5);

    if (reaped == 0) {
        _test_print_text("  the lookup still runs after 5 s; killing it\n");
        lc_kernel_send_signal((int32_t)pid, SIGKILL);
        lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    }
    TEST_ASSERT_EQ(reaped, pid);                    /* the lookup ended within 5 s */
    TEST_ASSERT_EQ(status, CHILD_NOENT << 8);       /* with LC_ERR_NOENT */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_plugin_has_dt_hash_alone);
    TEST_RUN(test_names_on_the_loop_are_found);
    TEST_RUN(test_missing_name_lookup_ends);
    return test_main();
}
