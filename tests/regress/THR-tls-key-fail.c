/*
 * THR-tls-key-fail — the coroutine scheduler must not share a TLS key with
 * the program.
 *
 * The scheduler kept its current scheduler in a TLS key that it made on
 * first use, and lightio's loop did the same, and both ignored a failure
 * of lc_tls_key_create. A program that had taken all 64 keys first left
 * the key at 0, its static initial value, so the scheduler kept its
 * pointer in the program's key 0: in a coroutine, the program read the
 * scheduler there. Both now keep their pointer in a slot of the per-thread
 * block reserved for the library, which no key reaches.
 *
 * This program takes every key, sets key 0, and runs one coroutine, which
 * reads key 0: it must read the program's value, in the coroutine and
 * after the run.
 */

#include "test.h"
#include <lightc/coroutine.h>
#include <lightc/thread.h>

static lc_tls_key key0;
static void *seen_in_coroutine;
static int ran;

static void read_key0(void *arg) {
    (void)arg;
    seen_in_coroutine = lc_tls_get(key0);
    ran = 1;
}

static void test_scheduler_keeps_off_key_0(void) {
    lc_tls_key key;
    int made = 0;
    while (lc_tls_key_create(&key)) {
        if (made == 0) key0 = key;
        made++;
    }
    TEST_REQUIRE_EQ(made, LC_TLS_MAX_KEYS);   /* every key is the program's */
    TEST_REQUIRE_EQ(key0, (lc_tls_key)0);
    lc_tls_set(key0, (void *)0xaaaa);

    lc_scheduler sched = lc_scheduler_create();
    TEST_REQUIRE_NOT_NULL(lc_coroutine_create(&sched, read_key0, NULL));
    lc_scheduler_run(&sched);
    lc_scheduler_destroy(&sched);

    TEST_ASSERT_EQ(ran, 1);
    TEST_ASSERT_EQ((uintptr_t)seen_in_coroutine, (uintptr_t)0xaaaa);   /* the defect: the scheduler */
    TEST_ASSERT_EQ((uintptr_t)lc_tls_get(key0), (uintptr_t)0xaaaa);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_scheduler_keeps_off_key_0);
    return test_main();
}
