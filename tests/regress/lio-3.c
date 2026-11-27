/*
 * lio-3 — lio_loop_run_threaded must join the workers that started, and
 * only those.
 *
 * It created worker i into handle i and, at the end, joined handles 0 up
 * to the number of workers that had started. When a create in the middle
 * failed, it joined that failed handle, whose fields the failed create had
 * left as they were, and never joined the last worker: with garbage in the
 * failed handle's alive, the join waited for ever. A worker now goes into
 * the next unused handle, so the live ones are packed at the front.
 *
 * lc_thread_create and lc_thread_join are wrapped: tests/regress/lio-3.ldflags
 * links this with -Wl,--wrap for both. The second of three worker creates
 * fails, with its handle's bytes 0xff, as an uninitialised one's might be.
 * The wrapped join passes a handle on only when it holds a live worker, and
 * counts the others. lio_loop_run_threaded runs in a thread of its own, and
 * after lio_loop_stop it must have joined the two live workers and no
 * other handle. A child runs it under a 5 s watchdog. The program needs
 * io_uring, which qemu-user lacks, and skips there.
 */

#include "test.h"
#include <lightio/lightio.h>
#include <lightc/async.h>
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#define PORT        19942
#define WORKERS     3
#define ITIMER_REAL 0

lc_result __real_lc_thread_create(lc_thread *thread, lc_thread_func func, void *arg);
void      __real_lc_thread_join(lc_thread *thread);

static _Atomic(int32_t) returned;
static int32_t runner_tid;
static _Atomic(int32_t) creates;
static int32_t live_tids[WORKERS];
static int32_t live_count, good_joins, bad_joins;

static int32_t run_threaded(void *loop) {
    lio_loop_run_threaded(loop, WORKERS);
    atomic_store(&returned, 1);
    return 0;
}

lc_result __wrap_lc_thread_create(lc_thread *thread, lc_thread_func func, void *arg);
lc_result __wrap_lc_thread_create(lc_thread *thread, lc_thread_func func, void *arg) {
    if (func == run_threaded) {
        lc_result r = __real_lc_thread_create(thread, func, arg);
        runner_tid = (int32_t)r.value;
        return r;
    }
    int32_t i = atomic_fetch_add(&creates, 1);
    if (i == 1) {   /* the second worker's create fails */
        lc_bytes_fill(thread, 0xff, sizeof *thread);
        return lc_err(LC_ERR_AGAIN);
    }
    lc_result r = __real_lc_thread_create(thread, func, arg);
    if (lc_is_ok(r) && live_count < WORKERS) live_tids[live_count++] = (int32_t)r.value;
    return r;
}

void __wrap_lc_thread_join(lc_thread *thread);
void __wrap_lc_thread_join(lc_thread *thread) {
    if (thread->tid == runner_tid) {
        __real_lc_thread_join(thread);
        return;
    }
    for (int32_t k = 0; k < live_count; k++) {
        if (live_tids[k] != 0 && thread->tid == live_tids[k]) {
            live_tids[k] = 0;
            good_joins++;
            __real_lc_thread_join(thread);
            return;
        }
    }
    bad_joins++;   /* a handle with no live worker: the real join would wait for ever */
}

static void serve_nothing(lio_stream *stream) {
    (void)stream;
}

/* 0 when the two live workers, and only they, were joined. */
static int32_t run(void) {
    lio_loop *loop = lio_loop_create();
    if (!loop) return 10;
    if (lc_is_err(lio_tcp_serve(loop, PORT, serve_nothing))) return 11;
    lc_thread runner;
    if (lc_is_err(lc_thread_create(&runner, run_threaded, loop))) return 12;
    /* lio_loop_run_threaded sets running after it has made the workers, so
     * stop until it returns. */
    for (int i = 0; i < 40 && !atomic_load(&returned); i++) {
        lc_time_sleep_milliseconds(100);
        lio_loop_stop(loop);
    }
    lc_thread_join(&runner);
    if (atomic_load(&creates) != WORKERS) return 13;
    if (bad_joins != 0) return 20 + bad_joins;   /* the defect: 21 */
    return good_joins == WORKERS - 1 ? 0 : 30 + good_joins;
}

static void test_workers_after_a_failed_create_are_joined(void) {
    lc_result_ptr ring = lc_async_ring_create(4);
    if (lc_ptr_is_err(ring)) TEST_SKIP("io_uring not available");
    lc_async_ring_destroy(ring.value);

    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, 5, 0 };   /* struct itimerval: 5 s, once */
        (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)watchdog, 0);
        lc_kernel_exit(run());
    }
    TEST_REQUIRE(pid > 0);
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status & 0x7f, 0);
    TEST_ASSERT_EQ((status >> 8) & 0xff, 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_workers_after_a_failed_create_are_joined);
    return test_main();
}
