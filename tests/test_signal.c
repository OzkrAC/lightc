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
#include "../src/kernel.h"   /* siginfo_t, SA_SIGINFO, SI_*, NSIG*, SIGPOLL */

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

/* ===== Signal numbers: 1 to LC_SIGNAL_MAX, and no other ===== */

/* The calling thread's blocked signals, as rt_sigprocmask gives them. */
static uint64_t blocked_signals(void) {
    uint64_t none = 0, old = 0;
    (void)lc_kernel_set_signal_mask(LC_SIG_BLOCK, &none, &old);
    return old;
}

/* The SigBlk line of /proc/thread-self/status, in *mask; false when it
 * cannot be read. On a kernel it is the same mask as blocked_signals';
 * under qemu-user it can be the emulator's own thread's, so the tests only
 * check that it does not change. */
static bool read_sigblk(uint64_t *mask) {
    static char buf[8192];
    static const char key[] = "\nSigBlk:";
    const lc_sysret key_length = (lc_sysret)sizeof key - 1;
    lc_sysret fd = lc_kernel_open_file("/proc/thread-self/status", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    lc_sysret n = lc_kernel_read_all((int32_t)fd, buf, sizeof buf);
    lc_kernel_close_file((int32_t)fd);
    for (lc_sysret i = 0; i + key_length <= n; i++) {
        if (lc_bytes_compare(buf + i, key, (size_t)key_length) != 0) continue;
        lc_sysret j = i + key_length;
        while (j < n && (buf[j] == ' ' || buf[j] == '\t')) j++;
        uint64_t value = 0;
        int32_t digits = 0;
        for (; j < n; j++, digits++) {
            char c = buf[j];
            if (c >= '0' && c <= '9')      value = value << 4 | (uint64_t)(c - '0');
            else if (c >= 'a' && c <= 'f') value = value << 4 | (uint64_t)(c - 'a' + 10);
            else break;
        }
        if (digits != 16) return false;
        *mask = value;
        return true;
    }
    return false;
}

/* Numbers that are no signal. 65 used to block or unblock SIGHUP, as its
 * bit wrapped to bit 0, and 0 and -1 signal 64 and 63. */
static const int32_t bad_signal_numbers[] = { 0, -1, 65, 129, -64, INT32_MIN };
#define BAD_SIGNAL_COUNT (sizeof bad_signal_numbers / sizeof bad_signal_numbers[0])

/* With the thread's mask set to `mask`, lc_signal_block and
 * lc_signal_unblock of each bad number return LC_ERR_INVAL and change
 * neither the mask nor SigBlk. With no signal blocked a wrong block shows,
 * and with every one blocked a wrong unblock. */
static void check_bad_numbers_keep_mask(uint64_t mask) {
    uint64_t saved = 0;
    TEST_REQUIRE_EQ(lc_kernel_set_signal_mask(LC_SIG_SETMASK, &mask, &saved), 0);
    uint64_t before = blocked_signals(), sigblk_before = 0, sigblk_after = 0;
    bool sigblk_read = read_sigblk(&sigblk_before);

    for (size_t i = 0; i < BAD_SIGNAL_COUNT; i++) {
        TEST_ASSERT_EQ(lc_signal_block(bad_signal_numbers[i]).error, LC_ERR_INVAL);
        TEST_ASSERT_EQ(lc_signal_unblock(bad_signal_numbers[i]).error, LC_ERR_INVAL);
    }
    sigblk_read = read_sigblk(&sigblk_after) && sigblk_read;
    uint64_t after = blocked_signals();

    (void)lc_kernel_set_signal_mask(LC_SIG_SETMASK, &saved, NULL);
    TEST_ASSERT_EQ(after, before);
    TEST_ASSERT(sigblk_read);
    TEST_ASSERT_EQ(sigblk_after, sigblk_before);
}

static void nothing_action(int signo, const lc_signal_info *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
}

/* Every function that takes a signal number refuses one outside 1 to 64
 * with LC_ERR_INVAL, and leaves the mask and a set as they were. */
static void test_signal_numbers_out_of_range(void) {
    check_bad_numbers_keep_mask(0);
    check_bad_numbers_keep_mask(UINT64_MAX);

    for (size_t i = 0; i < BAD_SIGNAL_COUNT; i++) {
        int32_t signo = bad_signal_numbers[i];
        TEST_ASSERT_EQ(lc_signal_handle(signo, dummy_handler).error, LC_ERR_INVAL);
        TEST_ASSERT_EQ(lc_signal_ignore(signo).error, LC_ERR_INVAL);
        TEST_ASSERT_EQ(lc_signal_reset(signo).error, LC_ERR_INVAL);
        TEST_ASSERT_EQ(lc_signal_action(signo, nothing_action, lc_signal_set_empty(), 0).error,
                       LC_ERR_INVAL);
        lc_signal_set set = lc_signal_set_empty();
        TEST_ASSERT_EQ(lc_signal_set_add(&set, signo).error, LC_ERR_INVAL);
        TEST_ASSERT_EQ(set.bits, (uint64_t)0);
        set = lc_signal_set_full();
        TEST_ASSERT_EQ(lc_signal_set_remove(&set, signo).error, LC_ERR_INVAL);
        TEST_ASSERT_EQ(set.bits, UINT64_MAX);
        TEST_ASSERT(!lc_signal_set_has(set, signo));
    }
}

/* 1 and LC_SIGNAL_MAX are signals: SIGHUP and SIGRTMAX, the mask's bits
 * 0 and 63. qemu-user has no host signal for the guest's highest
 * real-time signals, and its mask drops them, so of signal 64 this checks
 * only that its calls succeed and touch no other bit; test_signal_set
 * checks its bit. */
static void test_signal_numbers_at_the_ends(void) {
    const uint64_t top = (uint64_t)1 << 63;
    uint64_t saved = blocked_signals();
    uint64_t none = 0;
    TEST_REQUIRE_EQ(lc_kernel_set_signal_mask(LC_SIG_SETMASK, &none, NULL), 0);

    lc_result block_first = lc_signal_block(LC_SIGHUP);
    uint64_t first_blocked = blocked_signals();
    lc_result block_last = lc_signal_block(LC_SIGNAL_MAX);
    uint64_t both_blocked = blocked_signals();
    lc_result unblock_first = lc_signal_unblock(LC_SIGHUP);
    uint64_t first_unblocked = blocked_signals();
    lc_result unblock_last = lc_signal_unblock(LC_SIGNAL_MAX);
    uint64_t none_blocked = blocked_signals();
    (void)lc_kernel_set_signal_mask(LC_SIG_SETMASK, &saved, NULL);

    TEST_ASSERT_OK(block_first);
    TEST_ASSERT_EQ(first_blocked, (uint64_t)1);
    TEST_ASSERT_OK(block_last);
    TEST_ASSERT_EQ(both_blocked & ~top, (uint64_t)1);
    TEST_ASSERT_OK(unblock_first);
    TEST_ASSERT_EQ(first_unblocked & ~top, (uint64_t)0);
    TEST_ASSERT_OK(unblock_last);
    TEST_ASSERT_EQ(none_blocked, (uint64_t)0);
}

/* ===== lc_signal_set ===== */

static void test_signal_set(void) {
    lc_signal_set set = lc_signal_set_empty();
    TEST_ASSERT_EQ(set.bits, (uint64_t)0);
    for (int32_t signo = 1; signo <= LC_SIGNAL_MAX; signo++)
        TEST_ASSERT(!lc_signal_set_has(set, signo));

    TEST_ASSERT_OK(lc_signal_set_add(&set, LC_SIGHUP));
    TEST_ASSERT_OK(lc_signal_set_add(&set, LC_SIGTERM));
    TEST_ASSERT_OK(lc_signal_set_add(&set, LC_SIGNAL_MAX));
    TEST_ASSERT_OK(lc_signal_set_add(&set, LC_SIGTERM));
    TEST_ASSERT_EQ(set.bits, (uint64_t)1 | (uint64_t)1 << 14 | (uint64_t)1 << 63);
    TEST_ASSERT(lc_signal_set_has(set, LC_SIGTERM));
    TEST_ASSERT(!lc_signal_set_has(set, LC_SIGALRM));

    TEST_ASSERT_OK(lc_signal_set_remove(&set, LC_SIGTERM));
    TEST_ASSERT_OK(lc_signal_set_remove(&set, LC_SIGALRM));
    TEST_ASSERT_EQ(set.bits, (uint64_t)1 | (uint64_t)1 << 63);

    set = lc_signal_set_full();
    TEST_ASSERT_EQ(set.bits, UINT64_MAX);
    for (int32_t signo = 1; signo <= LC_SIGNAL_MAX; signo++)
        TEST_ASSERT(lc_signal_set_has(set, signo));
}

/* ===== lc_signal_action ===== */

/* What the handler of lc_signal_action got, the last time it ran. */
static volatile int32_t action_calls;
static volatile int32_t action_signo;
static void *volatile action_context;
static lc_signal_info action_info;

static void record_action(int signo, const lc_signal_info *info, void *context) {
    action_signo = signo;
    action_context = context;
    action_info = *info;
    action_calls = action_calls + 1;
}

/* Waits up to 2 s for action_calls to reach n. A signal that a thread
 * unblocks is handled before the unblock returns, on a kernel; the wait
 * only gives an emulator time it might need. */
static bool wait_for_actions(int32_t n) {
    for (int32_t i = 0; i < 2000 && action_calls < n; i++) lc_time_sleep_milliseconds(1);
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    return action_calls >= n;
}

/* The action the kernel has for signo. */
static lc_kernel_sigaction installed_action(int32_t signo) {
    lc_kernel_sigaction sa = { 0 };
    (void)lc_syscall4(SYS_rt_sigaction, signo, 0, (int64_t)&sa, 8);
    return sa;
}

/* lc_signal_action installs lightc's handler with SA_SIGINFO, the mask it
 * is given and SA_RESTART only for LC_SIGNAL_RESTART. */
static void test_signal_action_installs_siginfo_handler(void) {
    lc_signal_set mask = lc_signal_set_empty();
    TEST_REQUIRE_OK(lc_signal_set_add(&mask, LC_SIGTERM));
    TEST_REQUIRE_OK(lc_signal_set_add(&mask, LC_SIGHUP));
    TEST_REQUIRE_OK(lc_signal_action(LC_SIGUSR1, record_action, mask, 0));
    lc_kernel_sigaction sa = installed_action(LC_SIGUSR1);
    TEST_ASSERT_EQ(sa.flags & SA_SIGINFO, (uint64_t)SA_SIGINFO);
    TEST_ASSERT_EQ(sa.flags & SA_RESTART, (uint64_t)0);
    TEST_ASSERT_EQ(sa.mask, mask.bits);
    TEST_ASSERT(sa.handler != NULL && sa.handler != SIG_IGN);
    TEST_ASSERT(sa.handler != (void *)(uintptr_t)record_action);

    TEST_REQUIRE_OK(lc_signal_action(LC_SIGUSR1, record_action, lc_signal_set_empty(),
                                     LC_SIGNAL_RESTART));
    sa = installed_action(LC_SIGUSR1);
    TEST_ASSERT_EQ(sa.flags & SA_SIGINFO, (uint64_t)SA_SIGINFO);
    TEST_ASSERT_EQ(sa.flags & SA_RESTART, (uint64_t)SA_RESTART);
    TEST_ASSERT_EQ(sa.mask, (uint64_t)0);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGUSR1));
}

/* A NULL handler, a flag other than LC_SIGNAL_RESTART, SIGKILL and
 * SIGSTOP give LC_ERR_INVAL, and the signal keeps its action. */
static void test_signal_action_rejects(void) {
    lc_signal_set none = lc_signal_set_empty();
    TEST_REQUIRE_OK(lc_signal_handle(LC_SIGUSR1, dummy_handler));
    lc_kernel_sigaction before = installed_action(LC_SIGUSR1);

    TEST_ASSERT_EQ(lc_signal_action(LC_SIGUSR1, NULL, none, 0).error, LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_signal_action(LC_SIGUSR1, record_action, none, 1).error, LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_signal_action(LC_SIGUSR1, record_action, none, 0x08000000 /* SA_ONSTACK */).error,
                   LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_signal_action(LC_SIGUSR1, record_action, none,
                                    LC_SIGNAL_RESTART | 0x80000000U).error, LC_ERR_INVAL);
    lc_kernel_sigaction after = installed_action(LC_SIGUSR1);
    TEST_ASSERT(after.handler == before.handler);
    TEST_ASSERT_EQ(after.flags, before.flags);

    TEST_ASSERT_EQ(lc_signal_action(SIGKILL, record_action, none, 0).error, LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_signal_action(19 /* SIGSTOP */, record_action, none, 0).error, LC_ERR_INVAL);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGUSR1));
}

/* The handler of lc_signal_action and of lc_signal_handle replace each
 * other, and lc_signal_ignore both: the last call for a signal decides. */
static volatile int32_t plain_calls;
static void count_plain(int signo) { (void)signo; plain_calls = plain_calls + 1; }

static void test_signal_action_and_handle_replace_each_other(void) {
    int32_t self = lc_kernel_get_process_id();
    lc_signal_set none = lc_signal_set_empty();
    action_calls = 0;
    plain_calls = 0;

    TEST_REQUIRE_OK(lc_signal_action(LC_SIGUSR1, record_action, none, 0));
    (void)lc_kernel_send_signal(self, LC_SIGUSR1);
    TEST_ASSERT(wait_for_actions(1));
    TEST_ASSERT_EQ(action_calls, 1);
    TEST_ASSERT_EQ(plain_calls, 0);
    TEST_ASSERT_EQ(action_signo, LC_SIGUSR1);
    TEST_ASSERT_EQ(action_info.signal, (uint32_t)LC_SIGUSR1);
    TEST_ASSERT_EQ(action_info.code, SI_USER);
    TEST_ASSERT_EQ(action_info.pid, (uint32_t)self);
    TEST_ASSERT(action_context != NULL);

    TEST_REQUIRE_OK(lc_signal_handle(LC_SIGUSR1, count_plain));
    (void)lc_kernel_send_signal(self, LC_SIGUSR1);
    TEST_ASSERT_EQ(plain_calls, 1);
    TEST_ASSERT_EQ(action_calls, 1);

    TEST_REQUIRE_OK(lc_signal_action(LC_SIGUSR1, record_action, none, 0));
    (void)lc_kernel_send_signal(self, LC_SIGUSR1);
    TEST_ASSERT(wait_for_actions(2));
    TEST_ASSERT_EQ(action_calls, 2);
    TEST_ASSERT_EQ(plain_calls, 1);

    TEST_REQUIRE_OK(lc_signal_ignore(LC_SIGUSR1));
    (void)lc_kernel_send_signal(self, LC_SIGUSR1);
    TEST_ASSERT_EQ(action_calls, 2);
    TEST_ASSERT_EQ(plain_calls, 1);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGUSR1));
}

/* Blocks `set` in the calling thread, giving the mask before in *old. */
static lc_sysret block_set(lc_signal_set set, uint64_t *old) {
    return lc_kernel_set_signal_mask(LC_SIG_BLOCK, &set.bits, old);
}

/* A handler of lc_signal_action sees the pid of the child that sent the
 * signal with kill, and its uid. SIGUSR1 stays blocked until the child is
 * gone, so the signal is the child's, and is handled when it is
 * unblocked. */
static void test_signal_action_sees_sender_pid(void) {
    lc_signal_set usr1 = lc_signal_set_empty();
    TEST_REQUIRE_OK(lc_signal_set_add(&usr1, LC_SIGUSR1));
    uint64_t old = 0;
    TEST_REQUIRE_EQ(block_set(usr1, &old), 0);
    action_calls = 0;
    TEST_REQUIRE_OK(lc_signal_action(LC_SIGUSR1, record_action, lc_signal_set_empty(), 0));

    int32_t parent = lc_kernel_get_process_id();
    lc_sysret child = lc_kernel_fork();
    if (child == 0) {
        (void)lc_kernel_send_signal(parent, LC_SIGUSR1);
        lc_kernel_exit(0);
    }
    int32_t status = -1;
    if (child > 0) (void)lc_kernel_wait_for_child((int32_t)child, &status, 0);
    TEST_ASSERT(child > 0);
    TEST_ASSERT_EQ(status, 0);
    TEST_ASSERT_EQ(action_calls, 0);

    TEST_ASSERT_OK(lc_signal_unblock(LC_SIGUSR1));
    TEST_ASSERT(wait_for_actions(1));
    TEST_ASSERT_EQ(action_calls, 1);
    TEST_ASSERT_EQ(action_signo, LC_SIGUSR1);
    TEST_ASSERT_EQ(action_info.signal, (uint32_t)LC_SIGUSR1);
    TEST_ASSERT_EQ(action_info.code, SI_USER);
    TEST_ASSERT_EQ(action_info.pid, (uint32_t)child);
    TEST_ASSERT_EQ(action_info.uid, lc_kernel_getuid());

    (void)lc_kernel_set_signal_mask(LC_SIG_SETMASK, &old, NULL);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGUSR1));
}

/* Reads one record from a new signalfd of `set`, which holds a pending
 * signal; false when there was none. */
static bool read_signal_fd(lc_signal_set set, lc_signal_info *info) {
    lc_sysret fd = lc_kernel_create_signal_fd(-1, &set.bits, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd < 0) return false;
    lc_sysret n = lc_kernel_read_bytes((int32_t)fd, info, sizeof *info);
    lc_kernel_close_file((int32_t)fd);
    return n == (lc_sysret)sizeof *info;
}

/* A handler of lc_signal_action gets the record a signalfd read gives:
 * one child sends SIGUSR2 and SIGUSR1, read from a signalfd and handled,
 * and the two records differ only in the signal's number. */
static void test_signal_action_info_matches_signalfd_for_kill(void) {
    lc_signal_set both = lc_signal_set_empty(), usr2 = lc_signal_set_empty();
    TEST_REQUIRE_OK(lc_signal_set_add(&both, LC_SIGUSR1));
    TEST_REQUIRE_OK(lc_signal_set_add(&both, SIGUSR2));
    TEST_REQUIRE_OK(lc_signal_set_add(&usr2, SIGUSR2));
    uint64_t old = 0;
    TEST_REQUIRE_EQ(block_set(both, &old), 0);
    action_calls = 0;
    TEST_REQUIRE_OK(lc_signal_action(LC_SIGUSR1, record_action, lc_signal_set_empty(), 0));

    int32_t parent = lc_kernel_get_process_id();
    lc_sysret child = lc_kernel_fork();
    if (child == 0) {
        (void)lc_kernel_send_signal(parent, SIGUSR2);
        (void)lc_kernel_send_signal(parent, LC_SIGUSR1);
        lc_kernel_exit(0);
    }
    if (child > 0) (void)lc_kernel_wait_for_child((int32_t)child, NULL, 0);
    TEST_ASSERT(child > 0);

    lc_signal_info from_fd;
    TEST_ASSERT(read_signal_fd(usr2, &from_fd));
    TEST_ASSERT_OK(lc_signal_unblock(LC_SIGUSR1));
    TEST_ASSERT(wait_for_actions(1));
    (void)lc_kernel_set_signal_mask(LC_SIG_SETMASK, &old, NULL);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGUSR1));

    TEST_ASSERT_EQ(from_fd.signal, (uint32_t)SIGUSR2);
    TEST_ASSERT_EQ(from_fd.pid, (uint32_t)child);
    lc_signal_info from_handler = action_info;
    TEST_ASSERT_EQ(from_handler.signal, (uint32_t)LC_SIGUSR1);
    from_handler.signal = from_fd.signal;
    TEST_ASSERT_EQ(lc_bytes_compare(&from_handler, &from_fd, sizeof from_fd), 0);
}

/* The same for SIGCHLD, whose record has the child's pid, status and
 * times: one child's from a signalfd, the next one's handled. They differ
 * only in the pid and the times. */
static void test_signal_action_info_matches_signalfd_for_sigchld(void) {
    lc_signal_set chld = lc_signal_set_empty();
    TEST_REQUIRE_OK(lc_signal_set_add(&chld, SIGCHLD));
    uint64_t old = 0;
    TEST_REQUIRE_EQ(block_set(chld, &old), 0);

    lc_sysret first = lc_kernel_fork();
    if (first == 0) lc_kernel_exit(3);
    if (first > 0) (void)lc_kernel_wait_for_child((int32_t)first, NULL, 0);
    lc_signal_info from_fd;
    bool read = first > 0 && read_signal_fd(chld, &from_fd);

    action_calls = 0;
    lc_result installed = lc_signal_action(SIGCHLD, record_action, lc_signal_set_empty(), 0);
    lc_sysret second = lc_kernel_fork();
    if (second == 0) lc_kernel_exit(3);
    if (second > 0) (void)lc_kernel_wait_for_child((int32_t)second, NULL, 0);
    (void)lc_kernel_set_signal_mask(LC_SIG_SETMASK, &old, NULL);
    bool handled = wait_for_actions(1);
    (void)lc_signal_reset(SIGCHLD);

    TEST_REQUIRE(read);
    TEST_REQUIRE_OK(installed);
    TEST_REQUIRE(second > 0);
    TEST_REQUIRE(handled);
    TEST_ASSERT_EQ(from_fd.signal, (uint32_t)SIGCHLD);
    TEST_ASSERT_EQ(from_fd.code, 1);             /* CLD_EXITED */
    TEST_ASSERT_EQ(from_fd.status, 3);
    TEST_ASSERT_EQ(from_fd.pid, (uint32_t)first);
    lc_signal_info from_handler = action_info;
    TEST_ASSERT_EQ(from_handler.pid, (uint32_t)second);
    from_handler.pid = from_fd.pid;
    from_handler.utime = from_fd.utime;
    from_handler.stime = from_fd.stime;
    TEST_ASSERT_EQ(lc_bytes_compare(&from_handler, &from_fd, sizeof from_fd), 0);
}

/* A fault's record has the address: a child writes to address 16, and its
 * handler of SIGSEGV checks what it got, then exits with 0 when it was
 * right, or with 10 plus a bit for each field that was wrong. */
static void check_fault(int signo, const lc_signal_info *info, void *context) {
    int32_t wrong = 0;
    if (signo != LC_SIGSEGV || info->signal != (uint32_t)LC_SIGSEGV) wrong |= 1;
    if (info->code != 1 /* SEGV_MAPERR */) wrong |= 2;
    if (info->addr != 16) wrong |= 4;
    if (info->pid != 0 || info->uid != 0 || context == NULL) wrong |= 8;
    lc_kernel_exit(wrong == 0 ? 0 : 10 + wrong);
}

static void test_signal_action_sees_fault_address(void) {
    lc_sysret child = lc_kernel_fork();
    if (child == 0) {
        if (lc_is_err(lc_signal_action(LC_SIGSEGV, check_fault, lc_signal_set_empty(), 0)))
            lc_kernel_exit(2);
        volatile int32_t *volatile address = (volatile int32_t *)(uintptr_t)16;
        *address = 1;
        lc_kernel_exit(3);
    }
    TEST_REQUIRE(child > 0);
    int32_t status = -1;
    TEST_REQUIRE_EQ(lc_kernel_wait_for_child((int32_t)child, &status, 0), child);
    TEST_ASSERT_EQ(status, 0);   /* exited with 0 */
}

/* ===== LC_SIGNAL_RESTART ===== */

/* The handler writes to the pipe that a child waits on, so the child
 * writes its byte only once the read has been interrupted. */
static int32_t handled_fd = -1;
static volatile int32_t restart_handled;

static void note_handled(int signo, const lc_signal_info *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
    restart_handled = restart_handled + 1;
    (void)lc_kernel_write_bytes(handled_fd, "h", 1);
}

/* The state of process pid, from /proc/<pid>/stat: 'S' while it sleeps in
 * a system call; 0 when it cannot be read. */
static char process_state(int32_t pid) {
    char path[32] = "/proc/";
    size_t at = 6;
    char digits[12];
    size_t n = 0;
    for (uint32_t v = (uint32_t)pid; n == 0 || v != 0; v /= 10) digits[n++] = (char)('0' + v % 10);
    while (n > 0) path[at++] = digits[--n];
    lc_bytes_copy(path + at, "/stat", 6);

    char buf[512];
    lc_sysret fd = lc_kernel_open_file(path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return 0;
    lc_sysret length = lc_kernel_read_all((int32_t)fd, buf, sizeof buf);
    lc_kernel_close_file((int32_t)fd);
    /* "pid (comm) S ...": the state follows the last ')' and a space. */
    for (lc_sysret i = length - 1; i >= 0; i--)
        if (buf[i] == ')') return i + 2 < length ? buf[i + 2] : 0;
    return 0;
}

/*
 * A read of a pipe waits. A child waits until the reader has slept for 20
 * polls of 1 ms in a row, so that the read is surely under way, and sends
 * SIGALRM, whose handler of lc_signal_action runs; then, once the handler
 * has written to the pipe it waits on, the child writes a byte. With
 * LC_SIGNAL_RESTART the kernel restarts the read, which returns the byte;
 * without it the read returns -EINTR, and the next read the byte. The
 * child's own SIGALRM ends it after 5 s if no handler ran, and the read
 * then sees the end of the pipe instead of waiting for ever.
 */
static void check_pipe_read(uint32_t flags) {
    int32_t data[2], handled[2];
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(data, O_CLOEXEC), 0);
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(handled, O_CLOEXEC), 0);
    handled_fd = handled[1];
    restart_handled = 0;
    TEST_REQUIRE_OK(lc_signal_action(LC_SIGALRM, note_handled, lc_signal_set_empty(), flags));

    int32_t parent = lc_kernel_get_process_id();
    lc_sysret child = lc_kernel_fork();
    if (child == 0) {
        (void)lc_kernel_reset_signal(LC_SIGALRM);
        alarm_in(5000);
        lc_kernel_close_file(data[0]);
        lc_kernel_close_file(handled[1]);
        for (int32_t asleep = 0; asleep < 20; lc_time_sleep_milliseconds(1))
            asleep = process_state(parent) == 'S' ? asleep + 1 : 0;
        (void)lc_kernel_send_signal(parent, LC_SIGALRM);
        char h;
        if (lc_kernel_read_bytes(handled[0], &h, 1) == 1)
            (void)lc_kernel_write_bytes(data[1], "x", 1);
        lc_kernel_exit(0);
    }
    lc_kernel_close_file(data[1]);
    lc_kernel_close_file(handled[0]);

    char byte = 0;
    lc_sysret first = -1, second = -1;
    if (child > 0) {
        first = lc_kernel_read_bytes(data[0], &byte, 1);
        if (first != 1) second = lc_kernel_read_bytes(data[0], &byte, 1);
        (void)lc_kernel_wait_for_child((int32_t)child, NULL, 0);
    }
    lc_kernel_close_file(data[0]);
    lc_kernel_close_file(handled[1]);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGALRM));

    TEST_ASSERT(child > 0);
    TEST_ASSERT_EQ(restart_handled, 1);
    if (flags & LC_SIGNAL_RESTART) {
        TEST_ASSERT_EQ(first, 1);
    } else {
        TEST_ASSERT_EQ(first, -(lc_sysret)LC_ERR_INTR);
        TEST_ASSERT_EQ(second, 1);
    }
    TEST_ASSERT_EQ(byte, 'x');
}

static void test_signal_action_restart_resumes_pipe_read(void) {
    check_pipe_read(LC_SIGNAL_RESTART);
}

static void test_signal_action_without_restart_ends_pipe_read(void) {
    check_pipe_read(0);
}

/* ===== The record lightc's handler fills, kind by kind =====
 *
 * The kernel gives lightc's handler the siginfo_t of a signal, and the
 * handler fills an lc_signal_info as fs/signalfd.c's signalfd_copyinfo
 * fills a signalfd's record, by the kind kernel/signal.c's siginfo_layout
 * gives the signal and its code. A test can send few of the kinds, so this
 * calls lightc's handler itself, as the kernel would, with a siginfo_t of
 * each kind. Its handler is found by its signal number, which the kernel
 * passes the same as si_signo; the test passes SIGUSR2's and sets
 * si_signo to the signal of each kind, so that one handler gets every
 * kind, and no handler of a fault signal is ever installed.
 */

typedef void (*kernel_handler)(int signo, siginfo_t *si, void *context);

/* What the handler of SA_SIGINFO that the kernel has for signo is. */
static kernel_handler kernel_handler_of(int32_t signo) {
    return (kernel_handler)(uintptr_t)installed_action(signo).handler;
}

typedef enum { KIND_KILL, KIND_TIMER, KIND_POLL, KIND_FAULT, KIND_MCEERR, KIND_CHLD, KIND_RT, KIND_SYS } kind;

static uint16_t load16(const uint8_t *p) { uint16_t v; lc_bytes_copy(&v, p, 2); return v; }
static uint32_t load32(const uint8_t *p) { uint32_t v; lc_bytes_copy(&v, p, 4); return v; }
static uint64_t load64(const uint8_t *p) { uint64_t v; lc_bytes_copy(&v, p, 8); return v; }

/* The record signalfd_copyinfo makes of si for a signal of kind k. It reads
 * the union at the offsets of the UAPI's siginfo_t on both arches, which
 * tests/abi_check.c checks src/kernel.h's against, not through its
 * fields. */
static lc_signal_info expected_record(const siginfo_t *si, kind k) {
    const uint8_t *u = (const uint8_t *)si + 16;
    lc_signal_info e = { 0 };
    e.signal = (uint32_t)si->si_signo;
    e.error = si->si_errno;
    e.code = si->si_code;
    switch (k) {
        case KIND_KILL:   e.pid = load32(u); e.uid = load32(u + 4); break;
        case KIND_TIMER:  e.tid = load32(u); e.overrun = load32(u + 4);
                          e.ptr_val = load64(u + 8); e.int_val = (int32_t)load32(u + 8); break;
        case KIND_POLL:   e.band = (uint32_t)load64(u); e.fd = (int32_t)load32(u + 8); break;
        case KIND_FAULT:  e.addr = load64(u); break;
        case KIND_MCEERR: e.addr = load64(u); e.addr_lsb = load16(u + 8); break;
        case KIND_CHLD:   e.pid = load32(u); e.uid = load32(u + 4); e.status = (int32_t)load32(u + 8);
                          e.utime = load64(u + 16); e.stime = load64(u + 24); break;
        case KIND_RT:     e.pid = load32(u); e.uid = load32(u + 4);
                          e.ptr_val = load64(u + 8); e.int_val = (int32_t)load32(u + 8); break;
        case KIND_SYS:    e.call_addr = load64(u); e.syscall = (int32_t)load32(u + 8);
                          e.arch = load32(u + 12); break;
    }
    return e;
}

/* Each signal and code, and the kind siginfo_layout gives it on x86_64
 * and aarch64. A code from 1 to 127 is one of the signal's own up to its
 * NSIG* number, else one of SIGPOLL's up to NSIGPOLL, else a kill's. */
static const struct {
    int32_t signo;
    int32_t code;
    kind    k;
} kind_cases[] = {
    { LC_SIGUSR1, SI_USER,         KIND_KILL },
    { LC_SIGUSR1, SI_KERNEL,       KIND_KILL },
    { LC_SIGUSR1, -1 /* SI_QUEUE */,   KIND_RT },
    { LC_SIGUSR1, -6 /* SI_TKILL */,   KIND_RT },
    { LC_SIGUSR1, -60 /* SI_ASYNCNL */, KIND_RT },
    { LC_SIGUSR1, SI_TIMER,        KIND_TIMER },
    { LC_SIGUSR1, SI_SIGIO,        KIND_POLL },
    { LC_SIGUSR1, 1,               KIND_POLL },
    { LC_SIGUSR1, NSIGPOLL,        KIND_POLL },
    { LC_SIGUSR1, NSIGPOLL + 1,    KIND_KILL },
    { LC_SIGUSR1, SI_KERNEL - 1,   KIND_KILL },
    { LC_SIGUSR1, SI_KERNEL + 1,   KIND_KILL },
    { LC_SIGSEGV, 1 /* SEGV_MAPERR */, KIND_FAULT },
    { LC_SIGSEGV, NSIGSEGV,        KIND_FAULT },
    { LC_SIGSEGV, NSIGSEGV + 1,    KIND_KILL },
    { LC_SIGSEGV, SI_USER,         KIND_KILL },
    { LC_SIGSEGV, -1,              KIND_RT },
    { LC_SIGILL,  NSIGILL,         KIND_FAULT },
    { LC_SIGFPE,  NSIGFPE,         KIND_FAULT },
    { LC_SIGTRAP, NSIGTRAP,        KIND_FAULT },
    { LC_SIGTRAP, NSIGTRAP + 1,    KIND_KILL },
    { LC_SIGBUS,  2 /* BUS_ADRERR */, KIND_FAULT },
    { LC_SIGBUS,  BUS_MCEERR_AR,   KIND_MCEERR },
    { LC_SIGBUS,  BUS_MCEERR_AO,   KIND_MCEERR },
    { LC_SIGBUS,  NSIGBUS + 1,     KIND_POLL },
    { SIGCHLD,    1 /* CLD_EXITED */, KIND_CHLD },
    { SIGCHLD,    NSIGCHLD,        KIND_CHLD },
    { SIGCHLD,    NSIGCHLD + 1,    KIND_KILL },
    { SIGCHLD,    SI_USER,         KIND_KILL },
    { SIGPOLL,    1 /* POLL_IN */, KIND_POLL },
    { SIGPOLL,    NSIGPOLL + 1,    KIND_KILL },
    { LC_SIGSYS,  1 /* SYS_SECCOMP */, KIND_SYS },
    { LC_SIGSYS,  NSIGSYS,         KIND_SYS },
    { LC_SIGSYS,  NSIGSYS + 1,     KIND_POLL },
    { 40,         2,               KIND_POLL },
    { 40,         -1,              KIND_RT },
    { LC_SIGNAL_MAX, SI_USER,      KIND_KILL },
};

/* A siginfo_t whose union holds a different byte at each offset, so that a
 * field read from the wrong place gives the wrong value. */
static siginfo_t patterned_siginfo(int32_t signo, int32_t code) {
    siginfo_t si;
    uint8_t *bytes = (uint8_t *)&si;
    for (size_t i = 0; i < sizeof si; i++) bytes[i] = (uint8_t)(i * 37 + 11);
    si.si_signo = signo;
    si.si_errno = 5;
    si.si_code = code;
    return si;
}

static void test_signal_action_fills_each_kind(void) {
    TEST_REQUIRE_OK(lc_signal_action(SIGUSR2, record_action, lc_signal_set_empty(), 0));
    kernel_handler handler = kernel_handler_of(SIGUSR2);
    TEST_REQUIRE(handler != NULL);
    int32_t context = 0;

    for (size_t i = 0; i < sizeof kind_cases / sizeof kind_cases[0]; i++) {
        siginfo_t si = patterned_siginfo(kind_cases[i].signo, kind_cases[i].code);
        lc_signal_info expected = expected_record(&si, kind_cases[i].k);
        action_calls = 0;
        handler(SIGUSR2, &si, &context);
        bool same = action_calls == 1 && action_signo == SIGUSR2 && action_context == &context &&
                    lc_bytes_compare(&action_info, &expected, sizeof expected) == 0;
        if (!same) {
            _test_print_text("  signal ");
            lc_print_signed(TEST_FD, kind_cases[i].signo);
            _test_print_text(" code ");
            lc_print_signed(TEST_FD, kind_cases[i].code);
            _test_print_text(": not the record signalfd gives\n");
        }
        TEST_ASSERT(same);
    }

    /* A signal with no handler of lc_signal_action, or a number that is
     * none, calls nothing; nor does SIGKILL after an install the kernel
     * refused. */
    siginfo_t si = patterned_siginfo(LC_SIGUSR1, SI_USER);
    TEST_ASSERT_EQ(lc_signal_action(SIGKILL, record_action, lc_signal_set_empty(), 0).error,
                   LC_ERR_INVAL);
    action_calls = 0;
    handler(SIGKILL, &si, &context);
    handler(63, &si, &context);
    handler(0, &si, &context);
    handler(LC_SIGNAL_MAX + 1, &si, &context);
    TEST_ASSERT_EQ(action_calls, 0);
    TEST_ASSERT_OK(lc_signal_reset(SIGUSR2));
}

#ifdef LC_FAULTINJECT
/* In the faultinject build: when rt_sigaction fails, lc_signal_action
 * returns its error and leaves the signal's handler as it was. */
static void unwanted_action(int signo, const lc_signal_info *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
    plain_calls = plain_calls + 1;
}

static void test_signal_action_failed_install_keeps_handler(void) {
    lc_fault_reset();
    TEST_REQUIRE_OK(lc_signal_action(SIGUSR2, record_action, lc_signal_set_empty(), 0));
    kernel_handler handler = kernel_handler_of(SIGUSR2);
    lc_fault_rule rule = { .nr = SYS_rt_sigaction, .nth = 1, .count = 1, .error = LC_ERR_FAULT };
    TEST_REQUIRE_OK(lc_fault_arm(&rule));
    lc_result failed = lc_signal_action(SIGUSR2, unwanted_action, lc_signal_set_empty(), 0);
    lc_fault_reset();
    TEST_ASSERT_EQ(failed.error, LC_ERR_FAULT);

    siginfo_t si = patterned_siginfo(SIGUSR2, SI_USER);
    action_calls = 0;
    plain_calls = 0;
    handler(SIGUSR2, &si, NULL);
    TEST_ASSERT_EQ(action_calls, 1);
    TEST_ASSERT_EQ(plain_calls, 0);
    TEST_ASSERT_OK(lc_signal_reset(SIGUSR2));
}
#endif

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    TEST_RUN(test_signal_handle_register);
    TEST_RUN(test_signal_block_unblock);
    TEST_RUN(test_signal_ignore);
    TEST_RUN(test_signal_reset_call);
    TEST_RUN(test_signal_delivery_roundtrip);
    TEST_RUN(test_signal_handle_no_restart);

    TEST_RUN(test_signal_numbers_out_of_range);
    TEST_RUN(test_signal_numbers_at_the_ends);
    TEST_RUN(test_signal_set);
    TEST_RUN(test_signal_action_installs_siginfo_handler);
    TEST_RUN(test_signal_action_rejects);
    TEST_RUN(test_signal_action_and_handle_replace_each_other);
    TEST_RUN(test_signal_action_sees_sender_pid);
    TEST_RUN(test_signal_action_info_matches_signalfd_for_kill);
    TEST_RUN(test_signal_action_info_matches_signalfd_for_sigchld);
    TEST_RUN(test_signal_action_sees_fault_address);
    TEST_RUN(test_signal_action_restart_resumes_pipe_read);
    TEST_RUN(test_signal_action_without_restart_ends_pipe_read);
    TEST_RUN(test_signal_action_fills_each_kind);
#ifdef LC_FAULTINJECT
    TEST_RUN(test_signal_action_failed_install_keeps_handler);
#endif

    return test_main();
}
