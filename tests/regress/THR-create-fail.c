/*
 * THR-create-fail — a thread handle whose create failed must be safe to
 * join.
 *
 * lc_thread_create_with_stack filled in the handle only as it went, so one
 * that failed kept whatever tid and alive it had: garbage, in a handle on
 * the stack. Before an earlier fix it also kept stack_base at the stack it
 * had just unmapped. A join of that handle, as a cleanup loop that joins
 * every handle makes, then waited for ever on alive, or unmapped whatever
 * had since been mapped at stack_base. The handle is now all zeros from
 * the start of the create, and stays so when the create fails.
 *
 * An RLIMIT_NPROC of 0 makes clone fail with EAGAIN. The handle starts
 * with every byte 0xff and stack_base at a live mapping, as a reused or
 * uninitialised one might: after the failed create, a join must return,
 * and the mapping must keep its contents. It runs in a child under a 3 s
 * watchdog, which gives up root first when run as root, as the limit does
 * not bind root.
 */

#include "test.h"
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>

#define RLIMIT_NPROC 6
#define NOBODY       65534
#define ITIMER_REAL  0

static int32_t never_runs(void *arg) {
    (void)arg;
    return 0;
}

static int32_t join_a_failed_handle(void) {
    if (lc_kernel_getuid() == 0 && (lc_kernel_setgid(NOBODY) < 0 || lc_kernel_setuid(NOBODY) < 0))
        return 10;
    uint64_t limit[2] = { 0, 0 };   /* struct rlimit: no new thread or process */
    if (lc_syscall4(SYS_prlimit64, 0, RLIMIT_NPROC, (int64_t)limit, 0) < 0) return 11;

    uint32_t *live = lc_kernel_map_memory(NULL, 4096, PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (live == MAP_FAILED) return 12;
    *live = 0xc0decafe;

    lc_thread t;
    lc_bytes_fill(&t, 0xff, sizeof t);
    t.stack_base = live;
    t.stack_size = 4096;
    lc_result r = lc_thread_create(&t, never_runs, NULL);
    if (lc_is_ok(r)) return 13;   /* the limit did not hold */
    lc_thread_join(&t);           /* the defect: waits for ever on alive */
    return *(volatile uint32_t *)live == 0xc0decafe ? 0 : 14;
}

static void test_join_after_a_failed_create(void) {
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, 3, 0 };   /* struct itimerval: 3 s, once */
        (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)watchdog, 0);
        lc_kernel_exit(join_a_failed_handle());
    }
    TEST_REQUIRE(pid > 0);
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status & 0x7f, 0);          /* 14, SIGALRM: the join never returned */
    TEST_ASSERT_EQ((status >> 8) & 0xff, 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_join_after_a_failed_create);
    return test_main();
}
