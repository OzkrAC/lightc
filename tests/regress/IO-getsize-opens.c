/*
 * IO-getsize-opens — lc_file_get_size must not open the file.
 *
 * get_size opened the file for reading and took its size from
 * lseek(SEEK_END). Opening a FIFO for reading waits for a writer, so on a
 * FIFO the call never returned. A file the caller may stat but not read
 * failed with EACCES, and a /proc file with EINVAL from the lseek. It now
 * asks statx, which opens nothing.
 *
 * Before get_size on the FIFO, a timer is set to send SIGALRM after 2 s,
 * to a handler installed without SA_RESTART, so that with the defect the
 * open fails with EINTR instead of blocking for ever. Root may open any
 * file, so as root only the FIFO and /proc show the defect. The files go
 * in build/regress, so run this from the source root, as ninja regress
 * does.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/signal.h>
#include <lightc/syscall.h>

#define FIFO        "build/regress/IO-getsize-opens.fifo"
#define WRITE_ONLY  "build/regress/IO-getsize-opens.write-only"

#define S_IFIFO     0010000
#define ITIMER_REAL 0

/* struct itimerval: the interval, then the time to the first signal. */
typedef struct {
    int64_t interval_seconds;
    int64_t interval_microseconds;
    int64_t value_seconds;
    int64_t value_microseconds;
} test_itimerval;

static volatile int32_t alarms;

static void on_alarm(int signo) {
    (void)signo;
    alarms = alarms + 1;
}

/* SIGALRM once, after `seconds`, or none for 0. */
static lc_sysret set_timer(int64_t seconds) {
    test_itimerval timer = { 0, 0, seconds, 0 };
    return lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)&timer, 0);
}

static void print_result(const char *label, lc_result r) {
    _test_print_text("  get_size(");
    _test_print_text(label);
    _test_print_text("): value ");
    lc_print_signed(TEST_FD, r.value);
    _test_print_text(", error ");
    _test_print_text(lc_error_name(r.error));
    _test_print_text("\n");
}

static void test_fifo_does_not_block(void) {
    (void)lc_kernel_unlinkat(AT_FDCWD, FIFO, 0);
    TEST_REQUIRE_EQ(lc_kernel_mknodat(AT_FDCWD, FIFO, S_IFIFO | 0600, 0), 0);

    alarms = 0;
    TEST_REQUIRE_OK(lc_signal_handle(LC_SIGALRM, on_alarm));
    TEST_REQUIRE_EQ(set_timer(2), 0);
    lc_result r = lc_file_get_size(FIFO);
    (void)set_timer(0);
    (void)lc_signal_reset(LC_SIGALRM);

    print_result("a FIFO", r);
    TEST_ASSERT_EQ(alarms, 0);          /* it returned before the timer */
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(r.value, 0);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, FIFO, 0), 0);
}

static void test_write_only_file(void) {
    (void)lc_kernel_unlinkat(AT_FDCWD, WRITE_ONLY, 0);
    TEST_REQUIRE_OK(lc_file_write_all(WRITE_ONLY, "seventeen bytes!\n", 17));
    TEST_REQUIRE_EQ(lc_kernel_fchmodat(AT_FDCWD, WRITE_ONLY, 0200, 0), 0);

    lc_result r = lc_file_get_size(WRITE_ONLY);
    print_result("a 0200 file", r);
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(r.value, 17);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, WRITE_ONLY, 0), 0);
}

static void test_proc_file(void) {
    lc_result r = lc_file_get_size("/proc/self/stat");
    print_result("/proc/self/stat", r);
    TEST_ASSERT_OK(r);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_fifo_does_not_block);
    TEST_RUN(test_write_only_file);
    TEST_RUN(test_proc_file);
    return test_main();
}
