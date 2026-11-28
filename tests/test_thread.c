/*
 * test_thread.c — tests for lightc threading and spinlock.
 */

#include "test.h"
#include <lightc/thread.h>
#include <lightc/heap.h>
#include <lightc/syscall.h>
#include <lightc/time.h>
#include <stdatomic.h>

/* ===== H1 regression: child allocates before main ever does ===== */

static _Atomic(int32_t) tls_child_alloc_ok;

static int32_t tls_child_first_alloc(void *arg) {
    (void)arg;
    /* The child's first heap allocation. Under the old racy bootstrap this set
     * a process-global "main TLS ready" flag as a side effect. */
    lc_result_ptr r = lc_heap_allocate(128);
    if (lc_ptr_is_ok(r)) {
        lc_heap_free(r.value);
        atomic_store(&tls_child_alloc_ok, 1);
    }
    return 0;
}

/*
 * Reproduces the H1 heap-TLS bootstrap race (x86_64). A child thread performs
 * the process's very first heap allocation, then — after it exits — the main
 * thread performs ITS first allocation. Under the old code the child's alloc
 * flipped the shared `main_thread_tls_ready` flag, so main's first `%fs:0`
 * read executed with FS base still 0 and segfaulted. With main's FS initialized
 * eagerly at startup, main's first read is always safe regardless of ordering.
 *
 * This only discriminates while the main thread has not yet allocated, so it
 * MUST be the first test run — nothing above it in main() may touch the heap.
 */
static void test_tls_child_allocates_before_main(void) {
    atomic_store(&tls_child_alloc_ok, 0);

    lc_thread t;
    TEST_ASSERT_OK(lc_thread_create(&t, tls_child_first_alloc, NULL));
    lc_thread_join(&t);
    TEST_ASSERT_EQ(atomic_load(&tls_child_alloc_ok), 1);

    /* Main thread's first-ever heap allocation — must not fault. */
    lc_result_ptr r = lc_heap_allocate(128);
    TEST_ASSERT_PTR_OK(r);
    uint8_t *b = (uint8_t *)r.value;
    b[0] = 0x5A;
    b[127] = 0xA5;
    TEST_ASSERT_EQ(b[0], (uint8_t)0x5A);
    TEST_ASSERT_EQ(b[127], (uint8_t)0xA5);
    lc_heap_free(r.value);
}

/* ===== lc_thread_create / lc_thread_join ===== */

static _Atomic(int32_t) shared_value;

static int32_t set_shared_value(void *arg) {
    int32_t val = (int32_t)(intptr_t)arg;
    atomic_store(&shared_value, val);
    return 0;
}

static void test_thread_create_join(void) {
    lc_thread t;
    atomic_store(&shared_value, 0);

    lc_result r = lc_thread_create(&t, set_shared_value, (void *)(intptr_t)42);
    TEST_ASSERT_OK(r);

    lc_thread_join(&t);

    TEST_ASSERT_EQ(atomic_load(&shared_value), 42);
}

/* ===== Multiple threads — atomic increment ===== */

#define NUM_THREADS     4
#define INCS_PER_THREAD 10000

static _Atomic(int32_t) atomic_counter;

static int32_t increment_counter(void *arg) {
    (void)arg;
    for (int i = 0; i < INCS_PER_THREAD; i++) {
        atomic_fetch_add(&atomic_counter, 1);
    }
    return 0;
}

static void test_multiple_threads_atomic(void) {
    lc_thread threads[NUM_THREADS];
    atomic_store(&atomic_counter, 0);

    for (int i = 0; i < NUM_THREADS; i++) {
        lc_result r = lc_thread_create(&threads[i], increment_counter, NULL);
        TEST_ASSERT_OK(r);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        lc_thread_join(&threads[i]);
    }

    TEST_ASSERT_EQ(atomic_load(&atomic_counter), NUM_THREADS * INCS_PER_THREAD);
}

/* ===== Spinlock — multiple threads contend ===== */

static lc_spinlock lock = LC_SPINLOCK_INIT;
static int32_t guarded_counter;

static int32_t spinlock_increment(void *arg) {
    (void)arg;
    for (int i = 0; i < INCS_PER_THREAD; i++) {
        lc_spinlock_acquire(&lock);
        guarded_counter++;
        lc_spinlock_release(&lock);
    }
    return 0;
}

static void test_spinlock_contention(void) {
    lc_thread threads[NUM_THREADS];
    guarded_counter = 0;
    lock.state = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        lc_result r = lc_thread_create(&threads[i], spinlock_increment, NULL);
        TEST_ASSERT_OK(r);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        lc_thread_join(&threads[i]);
    }

    TEST_ASSERT_EQ(guarded_counter, NUM_THREADS * INCS_PER_THREAD);
}

/* ===== lc_spinlock_try_acquire — lock held returns false ===== */

static lc_spinlock try_lock = LC_SPINLOCK_INIT;
static _Atomic(int32_t) try_result;

static int32_t try_acquire_thread(void *arg) {
    (void)arg;
    /* The main thread is holding the lock, so try_acquire should fail */
    bool got_it = lc_spinlock_try_acquire(&try_lock);
    atomic_store(&try_result, got_it ? 1 : 0);
    return 0;
}

static void test_spinlock_try_acquire(void) {
    try_lock.state = 0;
    atomic_store(&try_result, -1);

    /* Acquire the lock before spawning the thread */
    lc_spinlock_acquire(&try_lock);

    lc_thread t;
    lc_result r = lc_thread_create(&t, try_acquire_thread, NULL);
    TEST_ASSERT_OK(r);

    lc_thread_join(&t);

    /* The child should have failed to acquire */
    TEST_ASSERT_EQ(atomic_load(&try_result), 0);

    lc_spinlock_release(&try_lock);

    /* Now try_acquire should succeed since lock is free */
    TEST_ASSERT(lc_spinlock_try_acquire(&try_lock));
    lc_spinlock_release(&try_lock);
}

/* ===== Thread heap allocation ===== */

static _Atomic(int32_t) heap_alloc_ok;

static int32_t thread_heap_alloc(void *arg) {
    (void)arg;
    lc_result_ptr r = lc_heap_allocate(256);
    if (lc_ptr_is_ok(r)) {
        /* Write to verify it is usable memory */
        uint8_t *bytes = (uint8_t *)r.value;
        for (int i = 0; i < 256; i++) {
            bytes[i] = (uint8_t)i;
        }
        lc_heap_free(r.value);
        atomic_store(&heap_alloc_ok, 1);
    } else {
        atomic_store(&heap_alloc_ok, 0);
    }
    return 0;
}

static void test_thread_heap_allocation(void) {
    atomic_store(&heap_alloc_ok, 0);

    lc_thread t;
    lc_result r = lc_thread_create(&t, thread_heap_alloc, NULL);
    TEST_ASSERT_OK(r);

    lc_thread_join(&t);

    TEST_ASSERT_EQ(atomic_load(&heap_alloc_ok), 1);
}

/* ===== Cross-thread heap free ===== */

#define CROSS_HEAP_COUNT 16

static void *cross_ptrs[CROSS_HEAP_COUNT];
static _Atomic(int32_t) cross_free_done;

static int32_t thread_free_memory(void *arg) {
    (void)arg;
    for (int i = 0; i < CROSS_HEAP_COUNT; i++) {
        lc_heap_free(cross_ptrs[i]);
    }
    atomic_store(&cross_free_done, 1);
    return 0;
}

static void test_cross_thread_heap_free(void) {
    atomic_store(&cross_free_done, 0);

    /* Main thread allocates */
    for (int i = 0; i < CROSS_HEAP_COUNT; i++) {
        lc_result_ptr pr = lc_heap_allocate(64);
        TEST_ASSERT_PTR_OK(pr);
        cross_ptrs[i] = pr.value;
    }

    /* Child thread frees */
    lc_thread t;
    lc_result r = lc_thread_create(&t, thread_free_memory, NULL);
    TEST_ASSERT_OK(r);

    lc_thread_join(&t);

    TEST_ASSERT_EQ(atomic_load(&cross_free_done), 1);
}

/* ===== M4: lc_thread_join is idempotent ===== */

static int32_t noop_thread(void *arg) { (void)arg; return 0; }

/*
 * A second join must not unmap the stack again. We capture the stack range,
 * join once (which frees it), then force a new mapping onto the exact freed
 * range with MAP_FIXED and write a sentinel. A second join that still unmaps
 * would tear that mapping down, so reading the sentinel back would SIGSEGV;
 * with the idempotent join it survives.
 */
static void test_thread_join_idempotent(void) {
    lc_thread t;
    TEST_ASSERT_OK(lc_thread_create(&t, noop_thread, NULL));

    void  *base = t.stack_base;
    size_t size = t.stack_size;
    TEST_ASSERT_NOT_NULL(base);

    lc_thread_join(&t);                     /* first join frees [base, base+size) */

    /* Deterministically reuse the freed range and stamp a sentinel into it. */
    void *reused = lc_kernel_map_memory(base, size, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    TEST_ASSERT(reused != MAP_FAILED);
    TEST_ASSERT_EQ((uintptr_t)reused, (uintptr_t)base);
    volatile uint32_t *sentinel = (volatile uint32_t *)reused;
    *sentinel = 0xC0DECAFEu;

    lc_thread_join(&t);                     /* second join — must NOT unmap `reused` */

    TEST_ASSERT_EQ(*sentinel, 0xC0DECAFEu); /* mapping intact => no double-unmap */

    lc_kernel_unmap_memory(reused, size);
}

/* ===== Thread exit against remote frees and adoption ===== */

/*
 * Producers allocate blocks of six size classes, free one in four of them
 * themselves, publish the rest in slots and exit; consumer threads free
 * what is published. Each round's producers start once the last round's
 * have exited, and its blocks go into one of two sets of slots, which is
 * drained before it is used again.
 *
 * In the first test the consumers free a block as soon as it is published,
 * so each producer's exit drains, reclaims and abandons pages that remote
 * frees are pushing into at that moment, and frees that come after its swap
 * of a page's remote list are orphan frees. In the second the consumers
 * free a round's blocks only once its producers have exited: those are all
 * orphan frees, which two consumers make at once, so that one of them often
 * finds the heap's lock taken and leaves its free for the holder. The
 * next round's producers adopt the abandoned pages meanwhile, four threads
 * at once, and each of them exits holding adopted pages that frees are
 * still coming into, while the exits of the others apply orphan frees.
 *
 * Each block holds its producer and index at its start and its end, so a
 * block handed out twice, or written by the heap while in use, shows; the
 * debug heap stops the program on a double free. In the faultinject build,
 * with LC_CHAOS set, the chaos points in the spinlock and the remote free
 * make the threads interleave in more ways.
 */
#define EXIT_RACE_ROUNDS    60
#define EXIT_RACE_PRODUCERS 4
#define EXIT_RACE_CONSUMERS 2
#define EXIT_RACE_BLOCKS    192
#define EXIT_RACE_SETS      2
#define EXIT_RACE_WAIT_MS   10000

static _Atomic(uint64_t *) exit_race_slots[EXIT_RACE_SETS][EXIT_RACE_PRODUCERS][EXIT_RACE_BLOCKS];
static _Atomic(int32_t) exit_race_open[EXIT_RACE_SETS];   /* the consumers may free the set's blocks */
static _Atomic(int32_t) exit_race_stop;
static _Atomic(int64_t) exit_race_freed;
static _Atomic(int32_t) exit_race_bad;

typedef struct {
    uint32_t set;
    uint32_t producer;
} exit_race_job;

static size_t exit_race_size(uint32_t i) {
    return 16 + (size_t)(i % 6) * 40;
}

static uint64_t exit_race_tag(uint32_t set, uint32_t producer, uint32_t i) {
    return 0x5A00000000000000ull | ((uint64_t)set << 40) | ((uint64_t)producer << 20) | i;
}

static int32_t exit_race_producer(void *arg) {
    exit_race_job *job = arg;
    for (uint32_t i = 0; i < EXIT_RACE_BLOCKS; i++) {
        size_t size = exit_race_size(i);
        lc_result_ptr r = lc_heap_allocate(size);
        if (lc_ptr_is_err(r)) {
            atomic_fetch_add(&exit_race_bad, 1);
            continue;
        }
        uint64_t *block = r.value;
        uint64_t tag = exit_race_tag(job->set, job->producer, i);
        block[0] = tag;
        ((uint8_t *)block)[size - 1] = (uint8_t)tag;
        if (i % 4 == 3) {
            lc_heap_free(block);
            continue;
        }
        atomic_store_explicit(&exit_race_slots[job->set][job->producer][i], block,
                              memory_order_release);
    }
    return 0;
}

/* Frees every published block of the open sets, checking it; returns how
 * many it freed. */
static int64_t exit_race_sweep(uint32_t start) {
    int64_t freed = 0;
    for (uint32_t n = 0; n < EXIT_RACE_SETS * EXIT_RACE_PRODUCERS; n++) {
        uint32_t s = ((n + start) / EXIT_RACE_PRODUCERS) % EXIT_RACE_SETS;
        uint32_t p = (n + start) % EXIT_RACE_PRODUCERS;
        if (atomic_load_explicit(&exit_race_open[s], memory_order_acquire) == 0) continue;
        for (uint32_t i = 0; i < EXIT_RACE_BLOCKS; i++) {
            _Atomic(uint64_t *) *slot = &exit_race_slots[s][p][i];
            if (atomic_load_explicit(slot, memory_order_relaxed) == NULL) continue;
            uint64_t *block = atomic_exchange_explicit(slot, NULL, memory_order_acquire);
            if (block == NULL) continue;
            uint64_t tag = exit_race_tag(s, p, i);
            if (block[0] != tag || ((uint8_t *)block)[exit_race_size(i) - 1] != (uint8_t)tag)
                atomic_fetch_add(&exit_race_bad, 1);
            lc_heap_free(block);
            freed++;
        }
    }
    atomic_fetch_add(&exit_race_freed, freed);
    return freed;
}

static int32_t exit_race_consumer(void *arg) {
    uint32_t start = (uint32_t)(uintptr_t)arg;
    while (atomic_load_explicit(&exit_race_stop, memory_order_acquire) == 0) {
        if (exit_race_sweep(start) == 0) (void)lc_syscall0(SYS_sched_yield);
    }
    return 0;
}

/* Whether every slot of `set` is empty, waiting for the consumers at most
 * EXIT_RACE_WAIT_MS. */
static bool exit_race_drained(uint32_t set) {
    for (int64_t start = lc_time_start_timer();
         lc_time_elapsed_milliseconds(start) < EXIT_RACE_WAIT_MS;
         (void)lc_syscall0(SYS_sched_yield)) {
        bool empty = true;
        for (uint32_t p = 0; p < EXIT_RACE_PRODUCERS && empty; p++)
            for (uint32_t i = 0; i < EXIT_RACE_BLOCKS && empty; i++)
                if (atomic_load_explicit(&exit_race_slots[set][p][i], memory_order_relaxed) != NULL)
                    empty = false;
        if (empty) return true;
    }
    return false;
}

/*
 * After the rounds every block is freed, so every page they used must be
 * back in its segment. A thread then allocates, in each of the six classes,
 * as many blocks as fill a page, and they must all be in one page: a page
 * that kept a block whose free was lost would be on an abandoned list with
 * room, this thread would adopt it, and a page's worth would not fit in it.
 * Pages are 64 KiB, and the classes of exit_race_size are 32, 80, 112, 160,
 * 192 and 256 bytes.
 */
static const uint32_t exit_race_page_blocks[6] = {
    65536 / 32, 65536 / 80, 65536 / 112, 65536 / 160, 65536 / 192, 65536 / 256,
};
static void *exit_race_check_blocks[65536 / 32];

static int32_t exit_race_checker(void *arg) {
    int32_t *spilled = arg;
    for (uint32_t c = 0; c < 6; c++) {
        size_t size = exit_race_size(c);
        uint32_t count = exit_race_page_blocks[c];
        for (uint32_t i = 0; i < count; i++) {
            lc_result_ptr r = lc_heap_allocate(size);
            exit_race_check_blocks[i] = lc_ptr_is_ok(r) ? r.value : NULL;
            if (exit_race_check_blocks[i] == NULL ||
                (uintptr_t)exit_race_check_blocks[i] >> 16 != (uintptr_t)exit_race_check_blocks[0] >> 16)
                (*spilled)++;
        }
        for (uint32_t i = 0; i < count; i++) lc_heap_free(exit_race_check_blocks[i]);
    }
    return 0;
}

/* Runs the rounds; with `gated`, a round's blocks are freed only once its
 * producers have exited. */
static void exit_race_run(bool gated) {
    atomic_store(&exit_race_stop, 0);
    atomic_store(&exit_race_freed, 0);
    atomic_store(&exit_race_bad, 0);
    for (uint32_t s = 0; s < EXIT_RACE_SETS; s++) atomic_store(&exit_race_open[s], gated ? 0 : 1);
    lc_heap_stats before;
    lc_heap_get_stats(&before);

    lc_thread consumers[EXIT_RACE_CONSUMERS];
    int consuming = 0;
    for (int c = 0; c < EXIT_RACE_CONSUMERS; c++) {
        lc_result r = lc_thread_create(&consumers[c], exit_race_consumer,
                                       (void *)(uintptr_t)(c * 3));
        TEST_ASSERT_OK(r);
        if (lc_is_ok(r)) consuming++;
    }

    int64_t published = 0;
    bool drained = true;
    for (uint32_t round = 0; round < EXIT_RACE_ROUNDS && consuming > 0 && drained; round++) {
        uint32_t set = round % EXIT_RACE_SETS;
        drained = exit_race_drained(set);
        if (!drained) break;
        if (gated) atomic_store_explicit(&exit_race_open[set], 0, memory_order_release);
        exit_race_job jobs[EXIT_RACE_PRODUCERS];
        lc_thread producers[EXIT_RACE_PRODUCERS];
        int started = 0;
        for (uint32_t p = 0; p < EXIT_RACE_PRODUCERS; p++) {
            jobs[p] = (exit_race_job){ .set = set, .producer = p };
            lc_result r = lc_thread_create(&producers[started], exit_race_producer, &jobs[p]);
            TEST_ASSERT_OK(r);
            if (lc_is_ok(r)) started++;
        }
        for (int p = 0; p < started; p++) lc_thread_join(&producers[p]);
        published += (int64_t)started * (EXIT_RACE_BLOCKS - EXIT_RACE_BLOCKS / 4);
        atomic_store_explicit(&exit_race_open[set], 1, memory_order_release);
    }
    TEST_ASSERT(drained);

    atomic_store_explicit(&exit_race_stop, 1, memory_order_release);
    for (int c = 0; c < consuming; c++) lc_thread_join(&consumers[c]);
    for (uint32_t s = 0; s < EXIT_RACE_SETS; s++) atomic_store(&exit_race_open[s], 1);
    (void)exit_race_sweep(0);   /* what the consumers left when they stopped */
    int32_t spilled = 0;
    lc_thread checker;
    TEST_ASSERT_OK(lc_thread_create(&checker, exit_race_checker, &spilled));
    lc_thread_join(&checker);
    TEST_ASSERT_EQ(spilled, 0);

    TEST_ASSERT_EQ(consuming, EXIT_RACE_CONSUMERS);
    TEST_ASSERT_EQ(atomic_load(&exit_race_bad), 0);
    TEST_ASSERT_EQ(atomic_load(&exit_race_freed), published);
#if LC_STATS
    lc_heap_stats after;
    lc_heap_get_stats(&after);
    TEST_ASSERT_EQ(after.active_allocations, before.active_allocations);
#endif
}

static void test_thread_exit_races_remote_frees(void) {
    exit_race_run(false);
}

static void test_thread_adoption_races_remote_frees(void) {
    exit_race_run(true);
}

/* ===== main ===== */

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    test_chaos_init(envp);   /* LC_CHAOS, in the faultinject build */

    /* H1 regression — MUST be first: no main-thread heap allocation may
     * precede it, or the bootstrap race it targets cannot be observed. */
    TEST_RUN(test_tls_child_allocates_before_main);

    /* thread create/join */
    TEST_RUN(test_thread_create_join);
    TEST_RUN(test_thread_join_idempotent);

    /* multiple threads — atomic counter */
    TEST_RUN(test_multiple_threads_atomic);

    /* spinlock contention */
    TEST_RUN(test_spinlock_contention);

    /* spinlock try_acquire */
    TEST_RUN(test_spinlock_try_acquire);

    /* cross-thread heap free */
    TEST_RUN(test_cross_thread_heap_free);

    /* thread heap allocation */
    TEST_RUN(test_thread_heap_allocation);

    /* thread exit against remote frees and adoption */
    TEST_RUN(test_thread_exit_races_remote_frees);
    TEST_RUN(test_thread_adoption_races_remote_frees);

    return test_main();
}
