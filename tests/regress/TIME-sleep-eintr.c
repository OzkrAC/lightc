/*
 * TIME-sleep-eintr — lc_time_sleep_milliseconds and
 * lc_time_sleep_microseconds must return once their time has passed,
 * however often a signal handler interrupts them.
 *
 * The defect is not fixed yet. Each time a handled signal interrupts the
 * nanosleep, they start the whole sleep again, so a periodic signal that
 * comes more often than the sleep is long keeps them from ever returning.
 *
 * Each test sleeps 50 ms while setitimer sends SIGALRM every 20 ms. The
 * handler stops the timer at the 100th signal, 2 s in, so that with the
 * defect the sleep returns 50 ms after that instead of never. The sleep
 * must return while the signals still come, and not before 50 ms.
 */

#include "test.h"
#include <lightc/signal.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

#define ITIMER_REAL 0

#define SLEEP_MS    50
#define PERIOD_US   20000
#define MAX_SIGNALS 100

/* struct itimerval: the interval, then the time to the first signal. */
typedef struct {
    int64_t interval_seconds;
    int64_t interval_microseconds;
    int64_t value_seconds;
    int64_t value_microseconds;
} test_itimerval;

static volatile int32_t signals;

/* SIGALRM every period_us microseconds, or none for 0. */
static lc_sysret set_timer(int64_t period_us) {
    test_itimerval timer = { 0, period_us, 0, period_us };
    return lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)&timer, 0);
}

static void on_alarm(int signo) {
    (void)signo;
    signals = signals + 1;
    if (signals == MAX_SIGNALS) (void)set_timer(0);
}

/* Calls sleep(amount), meant to last SLEEP_MS, while SIGALRM comes every
 * PERIOD_US, and checks that it returned in time. */
static void check_sleep(void (*sleep)(int64_t), int64_t amount) {
    signals = 0;
    TEST_REQUIRE_OK(lc_signal_handle(LC_SIGALRM, on_alarm));
    TEST_REQUIRE_EQ(set_timer(PERIOD_US), 0);

    int64_t start = lc_time_start_timer();
    sleep(amount);
    int64_t slept = lc_time_elapsed_milliseconds(start);
    int32_t seen = signals;
    (void)set_timer(0);

    _test_print_text("  returned after ");
    lc_print_signed(TEST_FD, slept);
    _test_print_text(" ms and ");
    lc_print_signed(TEST_FD, seen);
    _test_print_text(" signals\n");
    TEST_ASSERT(seen < MAX_SIGNALS);    /* returned while the signals still came */
    TEST_ASSERT(slept >= SLEEP_MS);     /* and slept its whole time */
}

static void test_sleep_milliseconds_with_periodic_signal(void) {
    check_sleep(lc_time_sleep_milliseconds, SLEEP_MS);
}

static void test_sleep_microseconds_with_periodic_signal(void) {
    check_sleep(lc_time_sleep_microseconds, SLEEP_MS * 1000);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_sleep_milliseconds_with_periodic_signal);
    TEST_RUN(test_sleep_microseconds_with_periodic_signal);
    return test_main();
}
