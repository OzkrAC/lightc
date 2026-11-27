/*
 * test_signal.c — tests for lightc signal handling.
 *
 * Signal tests are limited because signal delivery depends on
 * the runtime environment (containers may restrict signals).
 * We test registration succeeds without triggering signals.
 */

#include "test.h"
#include <lightc/signal.h>
#include <lightc/socket.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

static void dummy_handler(int signo) {
    (void)signo;
}

/* Test that signal handler registration succeeds */
static void test_signal_handle_register(void) {
    TEST_ASSERT_OK(lc_signal_handle(SIGUSR1, dummy_handler));
    (void)lc_signal_reset(SIGUSR1);
}

/* Test that block/unblock succeed */
static void test_signal_block_unblock(void) {
    TEST_ASSERT_OK(lc_signal_block(SIGUSR1));
    TEST_ASSERT_OK(lc_signal_unblock(SIGUSR1));
}

/* Test that ignore succeeds */
static void test_signal_ignore(void) {
    TEST_ASSERT_OK(lc_signal_ignore(SIGUSR1));
    (void)lc_signal_reset(SIGUSR1);
}

/* Test that reset succeeds */
static void test_signal_reset_call(void) {
    TEST_ASSERT_OK(lc_signal_handle(SIGUSR1, dummy_handler));
    TEST_ASSERT_OK(lc_signal_reset(SIGUSR1));
}

/*
 * Deliver a signal to ourselves and return from the handler. This exercises
 * the x86_64 SA_RESTORER trampoline (signal_restorer -> rt_sigreturn): the
 * kernel enters the restorer with %rsp pointing at the signal frame, so the
 * restorer must be prologue-free. If it isn't (e.g. a plain C function at -O0
 * emits `push %rbp`), rt_sigreturn reads the frame at the wrong address and
 * the resumed context is corrupt — the process crashes instead of continuing
 * past the kill() below. Reaching the asserts with both flags set proves the
 * handler ran and control returned cleanly through the restorer.
 */
static volatile int32_t roundtrip_ran;

static void roundtrip_handler(int signo) {
    (void)signo;
    roundtrip_ran = 1;
}

static void test_signal_delivery_roundtrip(void) {
    roundtrip_ran = 0;

    TEST_ASSERT_OK(lc_signal_handle(SIGUSR1, roundtrip_handler));

    /* kill() to self delivers the (unblocked) signal synchronously: the
     * handler runs and returns via the restorer before the syscall returns. */
    int32_t pid = lc_kernel_get_process_id();
    lc_kernel_send_signal(pid, SIGUSR1);

    int32_t resumed_ok = 1;  /* set only if we get here — i.e. resume worked */

    TEST_ASSERT_EQ(roundtrip_ran, 1);
    TEST_ASSERT_EQ(resumed_ok, 1);

    (void)lc_signal_reset(SIGUSR1);
}

/* ===== No SA_RESTART: a handled signal ends a blocking call with EINTR ===== */

#define SA_RESTART  0x10000000
#define ITIMER_REAL 0
#define SIGKILL     9

static volatile int32_t alarms;
static void count_alarm(int signo) { (void)signo; alarms++; }

/* SIGALRM once, `ms` milliseconds from now. */
static void alarm_in(int64_t ms) {
    int64_t timer[4] = { 0, 0, ms / 1000, (ms % 1000) * 1000 };   /* struct itimerval */
    (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)timer, 0);
}

/* lc_signal_handle sets no SA_RESTART, so the kernel does not restart a
 * call the handler interrupted: a raw lc_kernel_* call returns -EINTR, and
 * a socket call LC_ERR_INTR, to the caller, which decides. A read of a pipe
 * and an accept each wait, a SIGALRM comes at 20 ms, and a child ends both
 * waits at 300 ms, so that a restarted call fails the test, not hangs it. */
static void test_signal_handle_no_restart(void) {
    TEST_REQUIRE_OK(lc_signal_handle(LC_SIGALRM, count_alarm));
    lc_kernel_sigaction installed;
    TEST_REQUIRE_EQ(lc_syscall4(SYS_rt_sigaction, LC_SIGALRM, 0, (int64_t)&installed, 8), 0);
    TEST_ASSERT_EQ(installed.flags & SA_RESTART, (uint64_t)0);

    int32_t pipe_fds[2];
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(pipe_fds, 0), 0);
    lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, 0);
    lc_result listener = lc_socket_listen_on(&addr, 1);
    TEST_REQUIRE_OK(listener);
    lc_socket_address bound;
    uint32_t size = sizeof bound;
    TEST_REQUIRE_EQ(lc_kernel_getsockname((int32_t)listener.value, &bound, &size), 0);

    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        lc_time_sleep_milliseconds(300);
        (void)lc_kernel_write_bytes(pipe_fds[1], "x", 1);
        lc_result c = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
        if (lc_is_ok(c)) (void)lc_socket_connect((int32_t)c.value, &bound);
        lc_kernel_exit(0);
    }
    TEST_REQUIRE(pid > 0);

    char byte;
    alarms = 0;
    alarm_in(20);
    TEST_ASSERT_EQ(lc_kernel_read_bytes(pipe_fds[0], &byte, 1), -(lc_sysret)LC_ERR_INTR);
    TEST_ASSERT_EQ(alarms, 1);

    alarm_in(20);
    lc_result accepted = lc_socket_accept_flags((int32_t)listener.value, NULL, NULL, 0);
    TEST_ASSERT_EQ(accepted.error, LC_ERR_INTR);
    TEST_ASSERT_EQ(alarms, 2);

    (void)lc_kernel_send_signal((int32_t)pid, SIGKILL);
    (void)lc_kernel_wait_for_child((int32_t)pid, NULL, 0);
    if (lc_is_ok(accepted)) lc_socket_close((int32_t)accepted.value);
    lc_socket_close((int32_t)listener.value);
    lc_kernel_close_file(pipe_fds[0]);
    lc_kernel_close_file(pipe_fds[1]);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGALRM));
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    TEST_RUN(test_signal_handle_register);
    TEST_RUN(test_signal_block_unblock);
    TEST_RUN(test_signal_ignore);
    TEST_RUN(test_signal_reset_call);
    TEST_RUN(test_signal_delivery_roundtrip);
    TEST_RUN(test_signal_handle_no_restart);

    return test_main();
}
