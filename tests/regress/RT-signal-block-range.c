/*
 * RT-signal-block-range — lc_signal_block, lc_signal_unblock and
 * lc_signal_handle must refuse a number that is no signal with
 * LC_ERR_INVAL, and leave the thread's mask as it was.
 *
 * Linux has signals 1 to 64, one bit each of the 64-bit mask. lc_signal_block
 * and lc_signal_unblock shifted 1 left by signo - 1 whatever signo was, which
 * is undefined behaviour outside 1 to 64: x86_64 and aarch64 take the shift
 * count modulo 64, so 65 and 129 blocked or unblocked SIGHUP, 0 signal 64
 * and -1 signal 63, and each returned success. The ubsan build traps on the
 * shift instead. Both now check the number first, as lc_signal_handle does,
 * and make no system call for a bad one.
 *
 * Each test reads the mask before and after, both from rt_sigprocmask and
 * from the SigBlk line of /proc/thread-self/status, and needs both
 * unchanged. Under qemu-user SigBlk can be that of the emulator's own
 * thread, which a refused number does not change either.
 */

#include "test.h"
#include <lightc/signal.h>
#include <lightc/syscall.h>

/* Numbers that are no signal: below 1 or above 64. */
static const int32_t bad_numbers[] = { 0, -1, 65, 129 };
#define BAD_COUNT (sizeof bad_numbers / sizeof bad_numbers[0])

/* The calling thread's blocked signals, as rt_sigprocmask gives them. */
static uint64_t blocked_signals(void) {
    uint64_t none = 0, old = 0;
    (void)lc_kernel_set_signal_mask(LC_SIG_BLOCK, &none, &old);
    return old;
}

/* The SigBlk line of /proc/thread-self/status, in *mask; false when it
 * cannot be read. */
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

/* The two views of the thread's mask. */
typedef struct {
    uint64_t blocked;
    uint64_t sigblk;
    bool     sigblk_read;
} mask_views;

static mask_views read_masks(void) {
    mask_views v = { 0, 0, false };
    v.sigblk_read = read_sigblk(&v.sigblk);
    v.blocked = blocked_signals();
    return v;
}

static void print_mask(const char *what, uint64_t mask) {
    _test_print_text(what);
    lc_print_hex(TEST_FD, mask);
    lc_print_newline(TEST_FD);
}

/* Sets the thread's mask to `mask`, calls op with each bad number, which
 * must return LC_ERR_INVAL, and checks that neither view of the mask
 * changed. With no signal blocked a wrong block shows; with every one
 * blocked, a wrong unblock. */
static void check_keeps_mask(uint64_t mask, lc_result (*op)(int)) {
    uint64_t saved = 0;
    TEST_REQUIRE_EQ(lc_kernel_set_signal_mask(LC_SIG_SETMASK, &mask, &saved), 0);
    mask_views before = read_masks();
    lc_result results[BAD_COUNT];
    for (size_t i = 0; i < BAD_COUNT; i++) results[i] = op(bad_numbers[i]);
    mask_views after = read_masks();
    (void)lc_kernel_set_signal_mask(LC_SIG_SETMASK, &saved, NULL);

    for (size_t i = 0; i < BAD_COUNT; i++) {
        if (results[i].error != LC_ERR_INVAL) {
            _test_print_text("  signal number ");
            lc_print_signed(TEST_FD, bad_numbers[i]);
            _test_print_text(" was taken\n");
        }
        TEST_ASSERT_EQ(results[i].error, LC_ERR_INVAL);
    }
    if (after.blocked != before.blocked) {
        print_mask("  mask before: ", before.blocked);
        print_mask("  mask after:  ", after.blocked);
    }
    TEST_ASSERT_EQ(after.blocked, before.blocked);
    TEST_ASSERT(before.sigblk_read && after.sigblk_read);
    TEST_ASSERT_EQ(after.sigblk, before.sigblk);
}

/* lc_signal_block(65) blocked SIGHUP; 0, -1 and 129 blocked another. */
static void test_block_of_no_signal_blocks_nothing(void) {
    check_keeps_mask(0, lc_signal_block);
}

/* lc_signal_unblock(65) unblocked SIGHUP, and the others another. */
static void test_unblock_of_no_signal_unblocks_nothing(void) {
    check_keeps_mask(UINT64_MAX, lc_signal_unblock);
}

/* The repro's case: SIGHUP blocked for real stays blocked. */
static void test_unblock_65_keeps_sighup_blocked(void) {
    uint64_t saved = blocked_signals(), none = 0;
    TEST_REQUIRE_EQ(lc_kernel_set_signal_mask(LC_SIG_SETMASK, &none, NULL), 0);
    lc_result blocked = lc_signal_block(SIGHUP);
    lc_result unblocked = lc_signal_unblock(65);
    uint64_t mask = blocked_signals();
    (void)lc_kernel_set_signal_mask(LC_SIG_SETMASK, &saved, NULL);
    TEST_ASSERT_OK(blocked);
    TEST_ASSERT_EQ(unblocked.error, LC_ERR_INVAL);
    TEST_ASSERT_EQ(mask, (uint64_t)1);
}

static void any_handler(int signo) {
    (void)signo;
}

static lc_result handle(int signo) {
    return lc_signal_handle(signo, any_handler);
}

/* lc_signal_handle refuses them too, and blocks nothing. */
static void test_handle_of_no_signal_is_refused(void) {
    check_keeps_mask(0, handle);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_block_of_no_signal_blocks_nothing);
    TEST_RUN(test_unblock_of_no_signal_unblocks_nothing);
    TEST_RUN(test_unblock_65_keeps_sighup_blocked);
    TEST_RUN(test_handle_of_no_signal_is_refused);
    return test_main();
}
