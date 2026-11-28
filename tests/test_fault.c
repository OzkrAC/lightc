/*
 * test_fault.c — the fault rules and chaos points of the faultinject build
 * (<lightc/fault.h>).
 *
 * configure builds this suite only in the faultinject mode, where
 * lc_syscall0 to lc_syscall6, lc_kernel_clone and the clone of
 * lc_thread_create check the rules lc_fault_arm armed. Each test that arms
 * a rule ends with lc_fault_reset, so the next starts with an empty table.
 * Until then, it makes no system call that a rule it armed could catch
 * other than the ones it means to: while a test has a getpid, gettid or
 * getuid rule armed, lightc makes none of those calls of its own, and its
 * only sched_yield is that of the chaos points.
 *
 * LC_CHAOS in the environment seeds the chaos points for the whole suite
 * (test_chaos_init in test.h). The chaos tests seed their own and put the
 * environment's seed back when they end.
 */

#include "test.h"
#include <lightc/fault.h>
#include <lightc/heap.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#ifndef LC_FAULTINJECT
#error "test_fault.c tests the faultinject build: ./configure faultinject"
#endif

/* main's envp, for the tests that seed the chaos points and then put back
 * what LC_CHAOS says. */
static char **main_envp;

static int64_t arm(int64_t nr, uint32_t nth, uint32_t count, int32_t error) {
    lc_fault_rule rule = { .nr = nr, .nth = nth, .count = count, .error = error };
    lc_result r = lc_fault_arm(&rule);
    return r.error == 0 ? r.value : -1;
}

static int32_t exit_status(int32_t status) {
    return (status >> 8) & 0xff;   /* WEXITSTATUS */
}

/* ===== The item's two rules: mmap fails with ENOMEM, clone with EAGAIN ===== */

/*
 * A 1 MiB allocation is a large one: lc_heap_allocate maps it with one
 * mmap, unless the large cache has a block that big. The suite frees no
 * large block before this test, so the cache is empty and the rule catches
 * that mmap. The rule fails one call only, so the next allocation maps.
 */
static void test_mmap_rule_fails_a_large_allocation(void) {
    lc_heap_stats stats;
    lc_heap_get_stats(&stats);
    TEST_REQUIRE_EQ(stats.large_cache_count, 0u);

    int64_t id = arm(SYS_mmap, 1, 1, LC_ERR_NOMEM);
    TEST_REQUIRE(id >= 0);

    lc_result_ptr r = lc_heap_allocate(1024 * 1024);
    TEST_ASSERT_EQ(r.error, LC_ERR_NOMEM);
    TEST_ASSERT_NULL(r.value);
    TEST_ASSERT_EQ(lc_fault_hits(id), 1u);

    lc_result_ptr again = lc_heap_allocate(1024 * 1024);
    TEST_ASSERT_PTR_OK(again);
    TEST_ASSERT_EQ(lc_fault_hits(id), 1u);
    lc_heap_free(again.value);

    lc_fault_reset();
}

static int32_t set_flag(void *arg) {
    atomic_store((_Atomic(int32_t) *)arg, 1);
    return 0;
}

/*
 * lc_thread_create maps the stack and guards it, then clones in thread.S;
 * the faultinject build checks the SYS_clone rules before that clone. The
 * failed create leaves the handle all zeros, so a join returns at once, and
 * the rule fails one clone only, so the next create starts a thread.
 */
static void test_clone_rule_fails_a_thread_create(void) {
    int64_t id = arm(SYS_clone, 1, 1, LC_ERR_AGAIN);
    TEST_REQUIRE(id >= 0);

    _Atomic(int32_t) ran = 0;
    lc_thread t;
    lc_result r = lc_thread_create(&t, set_flag, &ran);
    TEST_ASSERT_EQ(r.error, LC_ERR_AGAIN);
    TEST_ASSERT_EQ(lc_fault_hits(id), 1u);
    TEST_ASSERT_EQ(t.tid, 0);
    TEST_ASSERT_NULL(t.stack_base);
    lc_thread_join(&t);
    TEST_ASSERT_EQ(atomic_load(&ran), 0);

    r = lc_thread_create(&t, set_flag, &ran);
    TEST_REQUIRE_OK(r);
    lc_thread_join(&t);
    TEST_ASSERT_EQ(atomic_load(&ran), 1);
    TEST_ASSERT_EQ(lc_fault_hits(id), 1u);

    lc_fault_reset();
}

/* ===== The other clones: lc_kernel_clone and lc_kernel_fork ===== */

static int clone_child(void *arg) {
    (void)arg;
    return 7;
}

/* lc_kernel_clone makes its clone in clone.S, and lc_kernel_fork through
 * lc_syscall5: one SYS_clone rule catches both. */
static void test_clone_rule_fails_kernel_clone_and_fork(void) {
    size_t size = 64 * 1024;
    uint8_t *stack = lc_kernel_map_memory(NULL, size, PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_REQUIRE(stack != MAP_FAILED);

    int64_t id = arm(SYS_clone, 1, 2, LC_ERR_AGAIN);
    TEST_REQUIRE(id >= 0);
    TEST_ASSERT_EQ(lc_kernel_clone(SIGCHLD, stack + size, clone_child, NULL), -LC_ERR_AGAIN);
    TEST_ASSERT_EQ(lc_kernel_fork(), -LC_ERR_AGAIN);
    TEST_ASSERT_EQ(lc_fault_hits(id), 2u);

    /* The rule is spent: both make their clone again. */
    int32_t status = 0;
    lc_sysret pid = lc_kernel_clone(SIGCHLD, stack + size, clone_child, NULL);
    TEST_ASSERT(pid > 0);
    if (pid > 0) {
        TEST_ASSERT_EQ(lc_kernel_wait_for_child((int32_t)pid, &status, 0), pid);
        TEST_ASSERT_EQ(exit_status(status), 7);
    }
    pid = lc_kernel_fork();
    if (pid == 0) lc_kernel_exit(9);
    TEST_ASSERT(pid > 0);
    if (pid > 0) {
        TEST_ASSERT_EQ(lc_kernel_wait_for_child((int32_t)pid, &status, 0), pid);
        TEST_ASSERT_EQ(exit_status(status), 9);
    }
    TEST_ASSERT_EQ(lc_fault_hits(id), 2u);

    lc_fault_reset();
    lc_kernel_unmap_memory(stack, size);
}

/* ===== Which calls a rule fails ===== */

/* nth 3 and count 2: the third and fourth calls fail, the others are made. */
static void test_rule_fails_calls_nth_to_nth_plus_count(void) {
    lc_sysret pid = lc_syscall0(SYS_getpid);
    TEST_REQUIRE(pid > 0);

    int64_t id = arm(SYS_getpid, 3, 2, LC_ERR_PERM);
    TEST_REQUIRE(id >= 0);
    lc_sysret want[6] = { pid, pid, -LC_ERR_PERM, -LC_ERR_PERM, pid, pid };
    for (int i = 0; i < 6; i++)
        TEST_ASSERT_EQ(lc_syscall0(SYS_getpid), want[i]);
    TEST_ASSERT_EQ(lc_fault_hits(id), 2u);

    lc_fault_reset();
}

/* The inline wrappers of <lightc/syscall.h>, compiled into this program,
 * call lc_syscallN too, so a rule catches them. */
static void test_rule_catches_an_inline_wrapper(void) {
    int32_t pid = lc_kernel_get_process_id();
    TEST_REQUIRE(pid > 0);

    int64_t id = arm(SYS_getpid, 1, 1, LC_ERR_PERM);
    TEST_REQUIRE(id >= 0);
    TEST_ASSERT_EQ(lc_kernel_get_process_id(), -LC_ERR_PERM);
    TEST_ASSERT_EQ(lc_kernel_get_process_id(), pid);
    TEST_ASSERT_EQ(lc_fault_hits(id), 1u);

    lc_fault_reset();
}

/* count 0 fails every call from nth on, until lc_fault_disarm; the rule
 * keeps its count of failed calls after that. */
static void test_count_zero_fails_until_disarmed(void) {
    lc_sysret pid = lc_syscall0(SYS_getpid);
    TEST_REQUIRE(pid > 0);

    int64_t id = arm(SYS_getpid, 2, 0, LC_ERR_IO);
    TEST_REQUIRE(id >= 0);
    TEST_ASSERT_EQ(lc_syscall0(SYS_getpid), pid);
    int failed = 0;
    for (int i = 0; i < 50; i++)
        failed += lc_syscall0(SYS_getpid) == -LC_ERR_IO;
    TEST_ASSERT_EQ(failed, 50);
    TEST_ASSERT_EQ(lc_fault_hits(id), 50u);

    lc_fault_disarm(id);
    TEST_ASSERT_EQ(lc_syscall0(SYS_getpid), pid);
    TEST_ASSERT_EQ(lc_fault_hits(id), 50u);
    lc_fault_disarm(id);   /* a second disarm does nothing */
    TEST_ASSERT_EQ(lc_fault_hits(id), 50u);

    lc_fault_reset();
    TEST_ASSERT_EQ(lc_fault_hits(id), 0u);
}

/* A rule fails only its own system call. */
static void test_rule_fails_only_its_system_call(void) {
    lc_sysret pid = lc_syscall0(SYS_getpid);
    TEST_REQUIRE(pid > 0);

    int64_t id = arm(SYS_gettid, 1, 0, LC_ERR_PERM);
    TEST_REQUIRE(id >= 0);
    TEST_ASSERT_EQ(lc_syscall0(SYS_getpid), pid);
    TEST_ASSERT_EQ(lc_syscall0(SYS_gettid), -LC_ERR_PERM);
    TEST_ASSERT_EQ(lc_fault_hits(id), 1u);

    lc_fault_reset();
}

/* Each rule of a system call counts every call of it. When two fail the
 * same call, the first armed gives the error, and both count it. */
static void test_two_rules_of_one_system_call(void) {
    lc_sysret pid = lc_syscall0(SYS_getpid);
    TEST_REQUIRE(pid > 0);

    int64_t first = arm(SYS_getpid, 1, 1, LC_ERR_PERM);
    int64_t second = arm(SYS_getpid, 1, 2, LC_ERR_IO);
    TEST_REQUIRE(first >= 0);
    TEST_REQUIRE(second >= 0);
    TEST_ASSERT_NEQ(first, second);
    TEST_ASSERT_EQ(lc_syscall0(SYS_getpid), -LC_ERR_PERM);
    TEST_ASSERT_EQ(lc_syscall0(SYS_getpid), -LC_ERR_IO);
    TEST_ASSERT_EQ(lc_syscall0(SYS_getpid), pid);
    TEST_ASSERT_EQ(lc_fault_hits(first), 1u);
    TEST_ASSERT_EQ(lc_fault_hits(second), 2u);

    lc_fault_reset();
}

/* lc_syscall0 to lc_syscall6 each check the rules: a rule on a system call
 * of each arity fails one call made with that many arguments, and the call
 * after it is made, which none of these fails with EXDEV. */
static void test_every_arity_checks_the_rules(void) {
    static const char path[] = "/nonexistent-lightc-fault";
    int64_t nrs[7] = { SYS_getpid, SYS_close, SYS_munmap, SYS_write,
                       SYS_openat, SYS_prctl, SYS_mmap };
    for (int n = 0; n <= 6; n++)
        TEST_REQUIRE(arm(nrs[n], 1, 1, LC_ERR_XDEV) >= 0);

    for (int round = 0; round < 2; round++) {
        lc_sysret got[7];
        got[0] = lc_syscall0(SYS_getpid);
        got[1] = lc_syscall1(SYS_close, -1);
        got[2] = lc_syscall2(SYS_munmap, 0, 0);
        got[3] = lc_syscall3(SYS_write, -1, (int64_t)path, 1);
        got[4] = lc_syscall4(SYS_openat, AT_FDCWD, (int64_t)path, O_RDONLY, 0);
        got[5] = lc_syscall5(SYS_prctl, -1, 0, 0, 0, 0);
        got[6] = lc_syscall6(SYS_mmap, 0, 0, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        for (int n = 0; n <= 6; n++) {
            if (round == 0) TEST_ASSERT_EQ(got[n], -LC_ERR_XDEV);
            else            TEST_ASSERT_NEQ(got[n], -LC_ERR_XDEV);
        }
    }
    for (int n = 0; n <= 6; n++)
        TEST_ASSERT_EQ(lc_fault_hits(n), 1u);

    lc_fault_reset();
}

/* ===== Arming ===== */

static void test_arm_rejects_bad_rules(void) {
    TEST_ASSERT_EQ(lc_fault_arm(NULL).error, LC_ERR_INVAL);
    lc_fault_rule bad[] = {
        { .nr = -1, .nth = 1, .count = 1, .error = LC_ERR_PERM },
        { .nr = SYS_getuid, .nth = 0, .count = 1, .error = LC_ERR_PERM },
        { .nr = SYS_getuid, .nth = 1, .count = 1, .error = 0 },
        { .nr = SYS_getuid, .nth = 1, .count = 1, .error = -LC_ERR_PERM },
        { .nr = SYS_getuid, .nth = 1, .count = 1, .error = 4096 },
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++)
        TEST_ASSERT_EQ(lc_fault_arm(&bad[i]).error, LC_ERR_INVAL);

    lc_fault_rule edge = { .nr = SYS_getuid, .nth = UINT32_MAX, .count = 0, .error = 4095 };
    lc_result r = lc_fault_arm(&edge);
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(r.value, 0);

    TEST_ASSERT_EQ(lc_fault_hits(-1), 0u);
    TEST_ASSERT_EQ(lc_fault_hits(LC_FAULT_MAX_RULES), 0u);
    lc_fault_disarm(-1);                   /* ignored */
    lc_fault_disarm(LC_FAULT_MAX_RULES);   /* ignored */
    lc_fault_disarm(LC_FAULT_MAX_RULES - 1);   /* no rule has it yet: ignored */

    lc_fault_reset();
}

/* The table holds LC_FAULT_MAX_RULES rules. A disarmed rule keeps its
 * place; lc_fault_reset empties the table, and ids start from 0 again. */
static void test_table_holds_max_rules_until_reset(void) {
    lc_fault_rule rule = { .nr = SYS_getuid, .nth = UINT32_MAX, .count = 1, .error = LC_ERR_PERM };
    for (int64_t i = 0; i < LC_FAULT_MAX_RULES; i++) {
        lc_result r = lc_fault_arm(&rule);
        TEST_ASSERT_OK(r);
        TEST_ASSERT_EQ(r.value, i);
    }
    TEST_ASSERT_EQ(lc_fault_arm(&rule).error, LC_ERR_FULL);
    lc_fault_disarm(0);
    TEST_ASSERT_EQ(lc_fault_arm(&rule).error, LC_ERR_FULL);

    lc_fault_reset();
    lc_result r = lc_fault_arm(&rule);
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(r.value, 0);
    lc_fault_reset();
}

/* ===== Threads ===== */

#define CALLERS 4
#define CALLS_EACH 5000
#define FIRST_FAILED 5001
#define FAILED_IN_ALL 10000

typedef struct {
    lc_barrier *start;
    int32_t failed;
} caller_state;

static int32_t make_getuid_calls(void *arg) {
    caller_state *state = arg;
    lc_barrier_wait(state->start);
    for (int i = 0; i < CALLS_EACH; i++)
        state->failed += lc_syscall0(SYS_getuid) == -LC_ERR_PERM;
    return 0;
}

/* A rule counts the calls of every thread: of the 20000 calls that 4
 * threads make at once, exactly the 10000 from the 5001st on fail. The
 * rule's calls are in the middle, where all four threads make them, so a
 * count that lost a call to a race would fail more. */
static void test_rule_counts_the_calls_of_every_thread(void) {
    lc_barrier start;
    lc_barrier_init(&start, CALLERS);
    caller_state state[CALLERS];
    lc_thread threads[CALLERS];
    int64_t id = arm(SYS_getuid, FIRST_FAILED, FAILED_IN_ALL, LC_ERR_PERM);
    TEST_REQUIRE(id >= 0);

    int started = 0;
    for (int i = 0; i < CALLERS; i++) {
        state[i] = (caller_state){ .start = &start, .failed = 0 };
        lc_result r = lc_thread_create(&threads[i], make_getuid_calls, &state[i]);
        TEST_ASSERT_OK(r);
        if (lc_is_ok(r)) started++;
    }
    TEST_REQUIRE_EQ(started, CALLERS);   /* else the barrier never opens */

    int32_t failed = 0;
    for (int i = 0; i < CALLERS; i++) {
        lc_thread_join(&threads[i]);
        failed += state[i].failed;
    }
    TEST_ASSERT_EQ(failed, FAILED_IN_ALL);
    TEST_ASSERT_EQ(lc_fault_hits(id), (uint64_t)FAILED_IN_ALL);

    lc_fault_reset();
}

/* ===== Chaos points ===== */

static lc_spinlock chaos_lock = LC_SPINLOCK_INIT;

static int32_t allocate_one(void *arg) {
    lc_result_ptr r = lc_heap_allocate(64);
    *(void **)arg = lc_ptr_is_ok(r) ? r.value : NULL;
    return 0;
}

/* Each acquire of a spinlock passes one chaos point, as does a try-acquire
 * that takes the lock, and a free into a page of another thread passes at
 * least one, one per try of its CAS. */
static void test_chaos_points_in_spinlock_and_remote_free(void) {
    /* This thread's own heap first, so that the free below takes no lock. */
    lc_result_ptr mine = lc_heap_allocate(64);
    TEST_REQUIRE_PTR_OK(mine);

    void *theirs = NULL;
    lc_thread t;
    TEST_REQUIRE_OK(lc_thread_create(&t, allocate_one, &theirs));
    lc_thread_join(&t);
    TEST_REQUIRE_NOT_NULL(theirs);

    lc_chaos_seed(12345);
    TEST_ASSERT_EQ(lc_chaos_count(), 0u);
    lc_spinlock_acquire(&chaos_lock);
    lc_spinlock_release(&chaos_lock);
    TEST_ASSERT_EQ(lc_chaos_count(), 1u);
    TEST_ASSERT(lc_spinlock_try_acquire(&chaos_lock));    /* takes it: a chaos point */
    TEST_ASSERT_EQ(lc_chaos_count(), 2u);
    TEST_ASSERT(!lc_spinlock_try_acquire(&chaos_lock));   /* fails: none */
    lc_spinlock_release(&chaos_lock);
    TEST_ASSERT_EQ(lc_chaos_count(), 2u);

    lc_heap_free(theirs);   /* their page: a remote free */
    TEST_ASSERT(lc_chaos_count() >= 3u);
    uint64_t after_remote = lc_chaos_count();
    lc_heap_free(mine.value);   /* our page: no chaos point */
    TEST_ASSERT_EQ(lc_chaos_count(), after_remote);

    /* Seed 0 turns them off. */
    lc_chaos_seed(0);
    lc_spinlock_acquire(&chaos_lock);
    lc_spinlock_release(&chaos_lock);
    TEST_ASSERT_EQ(lc_chaos_count(), 0u);

    test_chaos_init(main_envp);
}

/* One thread that passes the same points after the same seed gets the same
 * delays: here measured as the passes that yield, which a SYS_sched_yield
 * rule counts. A rule fails a yield, which the chaos point ignores. */
static void test_chaos_seed_repeats_its_yields(void) {
    uint64_t yields[2];
    for (int run = 0; run < 2; run++) {
        int64_t id = arm(SYS_sched_yield, 1, 0, LC_ERR_PERM);
        TEST_REQUIRE(id >= 0);
        lc_chaos_seed(777);
        for (int i = 0; i < 400; i++) lc_chaos_point();
        lc_chaos_seed(0);
        yields[run] = lc_fault_hits(id);
        lc_fault_reset();
    }
    /* 1 pass in 8 yields: about 50 of 400. */
    TEST_ASSERT_EQ(yields[0], yields[1]);
    TEST_ASSERT(yields[0] > 10u && yields[0] < 120u);

    test_chaos_init(main_envp);
}

#define CHAOS_THREADS 4
#define CHAOS_ROUNDS 2000
#define CHAOS_BLOCKS 256

typedef struct {
    lc_barrier *barrier;
    int64_t *counter;
    void **blocks;          /* this thread's allocations */
    void **neighbour;       /* the next thread's, which this one frees */
    int32_t failures;
} chaos_state;

static int32_t chaos_worker(void *arg) {
    chaos_state *s = arg;
    for (int i = 0; i < CHAOS_BLOCKS; i++) {
        lc_result_ptr r = lc_heap_allocate(16 + (size_t)(i % 7) * 48);
        s->blocks[i] = lc_ptr_is_ok(r) ? r.value : NULL;
        if (s->blocks[i] == NULL) s->failures++;
        else *(int64_t *)s->blocks[i] = i;
    }
    lc_barrier_wait(s->barrier);
    for (int i = 0; i < CHAOS_ROUNDS; i++) {
        lc_spinlock_acquire(&chaos_lock);
        (*s->counter)++;
        lc_spinlock_release(&chaos_lock);
    }
    for (int i = 0; i < CHAOS_BLOCKS; i++) {
        if (s->neighbour[i] == NULL) continue;
        if (*(int64_t *)s->neighbour[i] != i) s->failures++;
        lc_heap_free(s->neighbour[i]);   /* a remote free */
    }
    return 0;
}

/* Threads that contend for a spinlock and free each other's blocks, with
 * the chaos points on, still count right, and the debug heap finds no
 * double free or bad header. */
static void test_chaos_keeps_locks_and_remote_frees_right(void) {
    static void *blocks[CHAOS_THREADS][CHAOS_BLOCKS];
    lc_barrier barrier;
    lc_barrier_init(&barrier, CHAOS_THREADS);
    int64_t counter = 0;
    chaos_state state[CHAOS_THREADS];
    lc_thread threads[CHAOS_THREADS];

    lc_chaos_seed(4242);
    int started = 0;
    for (int i = 0; i < CHAOS_THREADS; i++) {
        state[i] = (chaos_state){
            .barrier = &barrier, .counter = &counter, .blocks = blocks[i],
            .neighbour = blocks[(i + 1) % CHAOS_THREADS], .failures = 0,
        };
        lc_result r = lc_thread_create(&threads[i], chaos_worker, &state[i]);
        TEST_ASSERT_OK(r);
        if (lc_is_ok(r)) started++;
    }
    TEST_REQUIRE_EQ(started, CHAOS_THREADS);
    for (int i = 0; i < CHAOS_THREADS; i++) lc_thread_join(&threads[i]);
    uint64_t passes = lc_chaos_count();
    test_chaos_init(main_envp);

    TEST_ASSERT_EQ(counter, (int64_t)CHAOS_THREADS * CHAOS_ROUNDS);
    for (int i = 0; i < CHAOS_THREADS; i++) TEST_ASSERT_EQ(state[i].failures, 0);
    TEST_ASSERT(passes > 0u);
}

/* ===== LC_CHAOS ===== */

static void test_chaos_init_reads_lc_chaos(void) {
    char *seeded[] = { "PATH=/bin", "LC_CHAOSX=1", "LC_CHAOS=31", "LC_CHAOS=0", NULL };
    test_chaos_init(seeded);
    lc_spinlock_acquire(&chaos_lock);
    lc_spinlock_release(&chaos_lock);
    TEST_ASSERT_EQ(lc_chaos_count(), 1u);   /* the first LC_CHAOS counts */

    char *unset[] = { "LC_CHAOSX=1", NULL };
    test_chaos_init(unset);
    lc_spinlock_acquire(&chaos_lock);
    lc_spinlock_release(&chaos_lock);
    TEST_ASSERT_EQ(lc_chaos_count(), 0u);

    char *empty[] = { "LC_CHAOS=", NULL };
    test_chaos_init(empty);
    lc_spinlock_acquire(&chaos_lock);
    lc_spinlock_release(&chaos_lock);
    TEST_ASSERT_EQ(lc_chaos_count(), 0u);

    char *largest[] = { "LC_CHAOS=18446744073709551615", NULL };
    test_chaos_init(largest);
    lc_spinlock_acquire(&chaos_lock);
    lc_spinlock_release(&chaos_lock);
    TEST_ASSERT_EQ(lc_chaos_count(), 1u);

    test_chaos_init(main_envp);
}

/* A value that is no seed stops the suite with status 2: here in a child. */
static void test_chaos_init_stops_on_a_bad_value(void) {
    char *bad[][2] = {
        { "LC_CHAOS=12x", NULL },
        { "LC_CHAOS=-1", NULL },
        { "LC_CHAOS=18446744073709551616", NULL },
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        lc_sysret pid = lc_kernel_fork();
        if (pid == 0) {
            test_chaos_init(bad[i]);
            lc_kernel_exit(0);
        }
        TEST_ASSERT(pid > 0);
        if (pid <= 0) continue;
        int32_t status = 0;
        TEST_ASSERT_EQ(lc_kernel_wait_for_child((int32_t)pid, &status, 0), pid);
        TEST_ASSERT_EQ(exit_status(status), 2);
    }
}

/* ===== The heap at a thread's exit, when its system calls fail =====
 *
 * When a thread of lc_thread_create returns, lc_heap_thread_exit gives its
 * heap state back: it madvises the pages it returns, unmaps a segment they
 * empty, points the TLS register at the sentinel (arch_prctl on x86_64) and
 * unmaps the thread's local. Each worker below arms its rule just before it
 * returns, so that the rule sees the calls of its exit, and main resets the
 * table once the kernel has cleared the thread's `alive`, after the exit and
 * before lc_thread_join unmaps the stack.
 */

#define EXIT_WAIT_MS 10000

/* Starts func(arg), waits until its thread has exited, resets the rule
 * table, then joins it. */
static bool run_then_reset(lc_thread_func func, void *arg) {
    lc_thread t;
    if (lc_is_err(lc_thread_create(&t, func, arg))) return false;
    int64_t start = lc_time_start_timer();
    bool exited = true;
    while (atomic_load_explicit(&t.alive, memory_order_acquire) != 0) {
        if (lc_time_elapsed_milliseconds(start) > EXIT_WAIT_MS) {
            exited = false;
            break;
        }
        lc_time_sleep_microseconds(100);
    }
    lc_fault_reset();
    lc_thread_join(&t);
    return exited;
}

static bool same_segment(const void *a, const void *b) {
    uintptr_t mask = ~(uintptr_t)(4 * 1024 * 1024 - 1);   /* a segment is 4 MiB, so aligned */
    return ((uintptr_t)a & mask) == ((uintptr_t)b & mask);
}

/* 16 blocks of the 4096-byte class fill a 64 KiB page. A segment holds 63
 * pages, and main holds a page of one, so 126 pages need at least one new
 * segment, which holds pages of this thread's only. */
#define SEGMENT_BLOCK_SIZE 4000
#define SEGMENT_BLOCKS     (16 * 126)

typedef struct {
    void            *blocks[SEGMENT_BLOCKS];
    uint32_t         got;
    void            *slots;            /* lc_heap_tls_slots of the thread */
    int64_t          rule;
    _Atomic(int32_t) phase;            /* 1: allocated; 2: main freed them */
    bool             allocated;
} exit_fault_work;

static exit_fault_work exit_fault_first, exit_fault_second;

static bool wait_phase(_Atomic(int32_t) *phase, int32_t value) {
    int64_t start = lc_time_start_timer();
    while (atomic_load_explicit(phase, memory_order_acquire) != value) {
        if (lc_time_elapsed_milliseconds(start) > EXIT_WAIT_MS) return false;
        lc_time_sleep_microseconds(100);
    }
    return true;
}

/* Allocates enough to take new segments, waits while main frees it all,
 * then arms munmap to fail: its exit drains main's frees, empties those
 * segments and unmaps them and its local, which all fail. */
static int32_t munmaps_fail_at_exit(void *arg) {
    exit_fault_work *w = arg;
    w->got = 0;
    for (uint32_t i = 0; i < SEGMENT_BLOCKS; i++) {
        lc_result_ptr r = lc_heap_allocate(SEGMENT_BLOCK_SIZE);
        if (lc_ptr_is_err(r)) break;
        w->blocks[i] = r.value;
        ((uint8_t *)r.value)[0] = (uint8_t)i;
        w->got++;
    }
    w->slots = lc_heap_tls_slots();
    atomic_store_explicit(&w->phase, 1, memory_order_release);
    (void)wait_phase(&w->phase, 2);
    w->rule = arm(SYS_munmap, 1, 0, LC_ERR_NOMEM);
    return 0;
}

/* With every mmap failing, allocates as much as the thread above did: it
 * can only with the local and the segments whose munmap failed. */
static int32_t allocate_with_mmap_failing(void *arg) {
    exit_fault_work *w = arg;
    w->rule = arm(SYS_mmap, 1, 0, LC_ERR_NOMEM);
    w->got = 0;
    for (uint32_t i = 0; i < SEGMENT_BLOCKS; i++) {
        lc_result_ptr r = lc_heap_allocate(SEGMENT_BLOCK_SIZE);
        if (lc_ptr_is_err(r)) break;
        w->blocks[i] = r.value;
        ((uint8_t *)r.value)[SEGMENT_BLOCK_SIZE - 1] = (uint8_t)i;
        w->got++;
    }
    w->slots = lc_heap_tls_slots();
    w->allocated = w->got == SEGMENT_BLOCKS && lc_fault_hits(w->rule) == 0;
    for (uint32_t i = 0; i < w->got; i++) lc_heap_free(w->blocks[i]);
    lc_fault_disarm(w->rule);
    return 0;
}

/* A segment whose munmap fails stays on the heap's list with every page
 * free, and a local whose munmap fails is kept for the next thread, so a
 * later thread needs no mmap for either, rather than both staying mapped
 * where nothing reaches them. */
static void test_heap_exit_keeps_what_munmap_failed_to_unmap(void) {
    lc_result_ptr mine = lc_heap_allocate(SEGMENT_BLOCK_SIZE);   /* holds a page of a segment */
    TEST_REQUIRE_PTR_OK(mine);

    exit_fault_work *first = &exit_fault_first;
    first->rule = -1;
    atomic_store(&first->phase, 0);
    lc_thread t;
    TEST_REQUIRE_OK(lc_thread_create(&t, munmaps_fail_at_exit, first));
    bool allocated = wait_phase(&first->phase, 1);
    TEST_ASSERT(allocated);
    TEST_ASSERT_EQ(first->got, (uint32_t)SEGMENT_BLOCKS);
    bool new_segment = false;
    for (uint32_t i = 0; i < first->got; i++) {
        if (!same_segment(first->blocks[i], mine.value)) new_segment = true;
        lc_heap_free(first->blocks[i]);   /* remote frees: its exit drains them */
    }
    TEST_ASSERT(new_segment);
    atomic_store_explicit(&first->phase, 2, memory_order_release);
    int64_t start = lc_time_start_timer();
    while (atomic_load_explicit(&t.alive, memory_order_acquire) != 0 &&
           lc_time_elapsed_milliseconds(start) < EXIT_WAIT_MS)
        lc_time_sleep_microseconds(100);
    TEST_ASSERT(first->rule >= 0);
    uint64_t failed = first->rule >= 0 ? lc_fault_hits(first->rule) : 0;
    lc_fault_reset();
    lc_thread_join(&t);
    TEST_ASSERT(failed >= 2u);   /* a new segment at least, and the local */

    exit_fault_work *second = &exit_fault_second;
    second->rule = -1;
    TEST_REQUIRE(run_then_reset(allocate_with_mmap_failing, second));
    TEST_ASSERT(second->allocated);
    TEST_ASSERT_EQ(second->got, (uint32_t)SEGMENT_BLOCKS);
    TEST_ASSERT(second->slots == first->slots);   /* the kept local */

    lc_heap_free(mine.value);
}

typedef struct {
    int64_t rule;
    void   *theirs;     /* a block of main's, which the thread frees */
    void   *slots;
    int32_t error;
} exit_fault_small;

/* With every madvise failing, the exit still gives the thread's emptied
 * page back to its segment. */
static int32_t madvise_fails_at_exit(void *arg) {
    exit_fault_small *w = arg;
    void *blocks[8];
    for (int i = 0; i < 8; i++) {
        lc_result_ptr r = lc_heap_allocate(500);
        blocks[i] = lc_ptr_is_ok(r) ? r.value : NULL;
        if (blocks[i] == NULL) w->error = r.error;
    }
    for (int i = 0; i < 8; i++) lc_heap_free(blocks[i]);
    w->rule = arm(SYS_madvise, 1, 0, LC_ERR_NOMEM);
    return 0;
}

static void test_heap_exit_survives_failed_madvise(void) {
    lc_heap_stats before, after;
    lc_heap_get_stats(&before);
    exit_fault_small w = { .rule = -1 };
    TEST_REQUIRE(run_then_reset(madvise_fails_at_exit, &w));
    lc_heap_get_stats(&after);
    TEST_ASSERT_EQ(w.error, 0);
    TEST_ASSERT(w.rule >= 0);
#if LC_STATS
    TEST_ASSERT(after.heap_pages_reclaimed >= before.heap_pages_reclaimed + 1);
#endif

    /* The page that kept its memory is a fresh one again. */
    exit_fault_small again = { .rule = -1 };
    TEST_REQUIRE(run_then_reset(madvise_fails_at_exit, &again));
    TEST_ASSERT_EQ(again.error, 0);
}

/* With every mmap failing from its start, a thread gets no local: its
 * allocation fails with LC_ERR_NOMEM, it has no TLS slots, its free of a
 * block of another thread still goes onto that page's remote list, and its
 * exit has nothing to give back. */
static int32_t no_local_at_all(void *arg) {
    exit_fault_small *w = arg;
    w->rule = arm(SYS_mmap, 1, 0, LC_ERR_NOMEM);
    lc_result_ptr r = lc_heap_allocate(64);
    w->error = r.error;
    if (lc_ptr_is_ok(r)) lc_heap_free(r.value);
    w->slots = lc_heap_tls_slots();
    lc_heap_free(w->theirs);
    return 0;
}

static void test_heap_thread_without_a_local(void) {
    lc_result_ptr mine = lc_heap_allocate(64);
    TEST_REQUIRE_PTR_OK(mine);
    exit_fault_small w = { .rule = -1, .theirs = mine.value };
    TEST_REQUIRE(run_then_reset(no_local_at_all, &w));
    TEST_ASSERT_EQ(w.error, LC_ERR_NOMEM);
    TEST_ASSERT_NULL(w.slots);

    /* The free above went onto this thread's page, which drains it. */
    lc_result_ptr again = lc_heap_allocate(64);
    TEST_REQUIRE_PTR_OK(again);
    TEST_ASSERT(again.value == mine.value);
    lc_heap_free(again.value);
}

#if defined(__x86_64__)
/* When arch_prctl fails at the exit, FS still points at the local, which
 * then stays mapped, out of the table and with no page; the next thread
 * maps a local of its own. */
static int32_t arch_prctl_fails_at_exit(void *arg) {
    exit_fault_small *w = arg;
    lc_result_ptr r = lc_heap_allocate(64);
    w->error = r.error;
    if (lc_ptr_is_ok(r)) lc_heap_free(r.value);
    w->slots = lc_heap_tls_slots();
    w->rule = arm(SYS_arch_prctl, 1, 1, LC_ERR_PERM);
    return 0;
}

static void test_heap_exit_survives_failed_arch_prctl(void) {
    exit_fault_small first = { .rule = -1 };
    lc_thread t;
    TEST_REQUIRE_OK(lc_thread_create(&t, arch_prctl_fails_at_exit, &first));
    int64_t start = lc_time_start_timer();
    while (atomic_load_explicit(&t.alive, memory_order_acquire) != 0 &&
           lc_time_elapsed_milliseconds(start) < EXIT_WAIT_MS)
        lc_time_sleep_microseconds(100);
    TEST_ASSERT(first.rule >= 0);
    TEST_ASSERT_EQ(first.rule >= 0 ? lc_fault_hits(first.rule) : 0, 1u);
    lc_fault_reset();
    lc_thread_join(&t);
    TEST_ASSERT_EQ(first.error, 0);

    exit_fault_small second = { .rule = -1 };
    TEST_REQUIRE(run_then_reset(arch_prctl_fails_at_exit, &second));
    TEST_ASSERT_EQ(second.error, 0);
    TEST_ASSERT(second.slots != first.slots);
}
#endif

/* ===== main ===== */

int main(int argc, char **argv, char **envp) {
    main_envp = envp;
    test_init(argc, argv);
    test_chaos_init(envp);

    /* First: no large block may have been freed before it. */
    TEST_RUN(test_mmap_rule_fails_a_large_allocation);
    TEST_RUN(test_clone_rule_fails_a_thread_create);
    TEST_RUN(test_clone_rule_fails_kernel_clone_and_fork);

    TEST_RUN(test_rule_fails_calls_nth_to_nth_plus_count);
    TEST_RUN(test_rule_catches_an_inline_wrapper);
    TEST_RUN(test_count_zero_fails_until_disarmed);
    TEST_RUN(test_rule_fails_only_its_system_call);
    TEST_RUN(test_two_rules_of_one_system_call);
    TEST_RUN(test_every_arity_checks_the_rules);

    TEST_RUN(test_arm_rejects_bad_rules);
    TEST_RUN(test_table_holds_max_rules_until_reset);

    TEST_RUN(test_rule_counts_the_calls_of_every_thread);

    TEST_RUN(test_chaos_points_in_spinlock_and_remote_free);
    TEST_RUN(test_chaos_seed_repeats_its_yields);
    TEST_RUN(test_chaos_keeps_locks_and_remote_frees_right);
    TEST_RUN(test_chaos_init_reads_lc_chaos);
    TEST_RUN(test_chaos_init_stops_on_a_bad_value);

    /* The heap at a thread's exit, when its system calls fail */
    TEST_RUN(test_heap_exit_keeps_what_munmap_failed_to_unmap);
    TEST_RUN(test_heap_exit_survives_failed_madvise);
    TEST_RUN(test_heap_thread_without_a_local);
#if defined(__x86_64__)
    TEST_RUN(test_heap_exit_survives_failed_arch_prctl);
#endif

    return test_main();
}
