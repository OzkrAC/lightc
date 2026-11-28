/*
 * HEAP-tid-reuse-tls — a thread that gets the id of a thread that has
 * exited must start with every TLS slot NULL.
 *
 * lc_tls_set and lc_tls_get keep a thread's values in its heap local, and
 * the heap keeps every local, in a table keyed by thread id, after its
 * thread has exited. A thread's first lc_tls_* call or allocation takes
 * over the local of its id when there is one. So once the kernel had handed
 * a dead thread's id out again, the new thread took over the dead one's
 * slots as they were, and lc_tls_get gave it the dead thread's values. The
 * slots are now cleared when a thread takes a local over.
 *
 * A thread of lc_thread_create now gives its local back when its function
 * returns (HEAP-thread-exit-leak), so only a thread that ends itself with
 * its own SYS_exit leaves a local behind for a thread of its id to take
 * over. The marked threads end so, which keeps this test on the takeover.
 *
 * MARKED threads each set a key to a value of their own and exit. Probe
 * threads then run one at a time, each joined before the next starts,
 * until the kernel's thread ids have wrapped round and WANTED probes got
 * the id of a marked thread. Each of those reads the key, which must be
 * NULL. A probe with any other id touches no TLS, so it leaves no local
 * behind. Other processes take ids too, so the test counts the probes that
 * get any marked id rather than waiting for one id to come back.
 *
 * The ids wrap at kernel.pid_max, the host's also under qemu-user, whose
 * threads have host thread ids. At 32768, as CI sets it, that takes about
 * 32,000 threads, a second or two. With a higher pid_max it takes
 * minutes, so the test then skips and says why; ninja regress then fails,
 * as for any program that skips every test, unless LIGHTC_TEST_ALLOW_SKIP
 * names HEAP-tid-reuse-tls.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>

#define PID_MAX_PATH  "/proc/sys/kernel/pid_max"
#define PID_MAX_LIMIT 32768     /* the largest kernel.pid_max the test runs at */
#define MARKED        64        /* threads that set the key */
#define WANTED        32        /* probes with a marked id that the test checks */
#define TIME_LIMIT_MS 60000     /* a bound for a slow emulator; normally ~1 s */

/* What a thread saw. */
typedef struct {
    int32_t tid;        /* its thread id */
    bool    reused;     /* a probe: whether a marked thread had the id */
    void   *found;      /* what lc_tls_get(key) gave it */
} report;

static lc_tls_key key;
static report marked[MARKED];
static uint8_t is_marked[PID_MAX_LIMIT / 8];   /* one bit per thread id */

/* Sets the key to this thread's own report, a value no other thread sets,
 * and ends the thread with SYS_exit, which leaves its local in the table. */
static int32_t marker(void *arg) {
    report *r = arg;
    r->tid = lc_kernel_get_thread_id();
    r->found = lc_tls_get(key);
    lc_tls_set(key, r);
    (void)lc_syscall1(SYS_exit, 0);
    return 0;
}

static int32_t prober(void *arg) {
    report *r = arg;
    int32_t tid = lc_kernel_get_thread_id();
    r->tid = tid;
    r->reused = tid > 0 && tid < PID_MAX_LIMIT && (is_marked[tid / 8] >> (tid % 8)) & 1;
    if (r->reused) r->found = lc_tls_get(key);
    return 0;
}

/* kernel.pid_max, or -1 when it cannot be read. */
static int64_t read_pid_max(void) {
    lc_sysret fd = lc_kernel_open_file(PID_MAX_PATH, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;
    char text[32];
    lc_sysret length = lc_kernel_read_all((int32_t)fd, text, sizeof(text));
    lc_kernel_close_file((int32_t)fd);
    int64_t value = 0;
    lc_sysret i = 0;
    for (; i < length && text[i] >= '0' && text[i] <= '9'; i++)
        value = value * 10 + (text[i] - '0');
    return i > 0 ? value : -1;
}

static void test_reused_tid_starts_with_empty_tls(void) {
    int64_t pid_max = read_pid_max();
    if (pid_max < 0) TEST_SKIP(PID_MAX_PATH " cannot be read: is /proc mounted?");
    if (pid_max > PID_MAX_LIMIT) {
        static char reason[192];
        lc_format f = lc_format_start(reason, sizeof(reason));
        lc_format_add_text(&f, "kernel.pid_max is ");
        lc_format_add_signed(&f, pid_max);
        lc_format_add_text(&f, ", above 32768, so thread ids take minutes to wrap;"
                               " sysctl -w kernel.pid_max=32768 lets this test run");
        lc_format_finish(&f);
        TEST_SKIP(reason);
    }
    TEST_REQUIRE(lc_tls_key_create(&key));

    for (uint32_t i = 0; i < MARKED; i++) {
        lc_thread thread;
        TEST_REQUIRE_OK(lc_thread_create(&thread, marker, &marked[i]));
        lc_thread_join(&thread);
        int32_t tid = marked[i].tid;
        TEST_REQUIRE(tid > 0 && tid < PID_MAX_LIMIT);
        TEST_ASSERT(marked[i].found == NULL);
        is_marked[tid / 8] |= (uint8_t)(1u << (tid % 8));
    }

    /* Within pid_max threads the ids wrap round to the marked ones; twice
     * that bounds the loop. */
    int64_t limit = 2 * pid_max + MARKED;
    int64_t threads = 0, reused = 0, stale = 0;
    int64_t start = lc_time_start_timer();
    while (reused < WANTED && threads < limit
           && lc_time_elapsed_milliseconds(start) < TIME_LIMIT_MS) {
        report r = { 0 };
        lc_thread thread;
        TEST_REQUIRE_OK(lc_thread_create(&thread, prober, &r));
        lc_thread_join(&thread);
        threads++;
        if (!r.reused) continue;
        reused++;
        if (r.found == NULL) continue;
        if (stale++ == 0) {
            int32_t owner = -1;
            for (uint32_t i = 0; i < MARKED; i++)
                if (r.found == &marked[i]) owner = marked[i].tid;
            _test_print_text("  probe ");
            lc_print_signed(TEST_FD, threads);
            _test_print_text(", of id ");
            lc_print_signed(TEST_FD, r.tid);
            _test_print_text(", read the value the exited thread of id ");
            lc_print_signed(TEST_FD, owner);
            _test_print_text(" had set\n");
        }
    }

    _test_print_text("  ");
    lc_print_signed(TEST_FD, threads);
    _test_print_text(" probes in ");
    lc_print_signed(TEST_FD, lc_time_elapsed_milliseconds(start));
    _test_print_text(" ms; ");
    lc_print_signed(TEST_FD, reused);
    _test_print_text(" got the id of a marked thread, and ");
    lc_print_signed(TEST_FD, stale);
    _test_print_text(" of those found its value\n");
    if (reused == 0)
        _test_print_text("  no probe got a marked id, so the ids did not wrap round in time\n");
    TEST_ASSERT(reused > 0);
    TEST_ASSERT_EQ(stale, 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_reused_tid_starts_with_empty_tls);
    return test_main();
}
