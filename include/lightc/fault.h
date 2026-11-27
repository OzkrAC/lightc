#ifndef LIGHTC_FAULT_H
#define LIGHTC_FAULT_H

/*
 * Fault injection and chaos points, for tests: only in the faultinject
 * build.
 *
 * ./configure faultinject builds lightc, and every program ninja builds,
 * with LC_FAULTINJECT defined. In that build lc_syscall0 to lc_syscall6,
 * lc_kernel_clone and the clone that starts a thread in lc_thread_create
 * are C functions that check a table of armed rules before they make their
 * system call. A rule names a system call and fails some of its calls with
 * an errno, as the kernel could: the call is not made, and it returns
 * -errno. So a test can make mmap fail with ENOMEM, or clone with EAGAIN,
 * and see what lightc does then. Every system call of the library goes
 * through one of them, as do the lc_kernel_* wrappers of <lightc/syscall.h>,
 * which are inline but call lc_syscallN. No rule checks the exits that the
 * assembly makes when a function it called returns: the exit_group after
 * main or after the function of an lc_kernel_clone child, and the exit of a
 * thread.
 *
 * A rule counts the calls of its system call from every thread, from the
 * moment it is armed. A forked child counts in its own copy of the table.
 * A call made at the same time as lc_fault_arm, lc_fault_disarm or
 * lc_fault_reset is checked against the table as it was before or as it is
 * after. A call checks the table with atomic loads and takes no lock, so a
 * signal handler and the child of a raw clone can make one; but none of the
 * three may run in a signal handler that interrupts one of them.
 *
 * The chaos points make threads interleave in more ways. lc_chaos_seed
 * turns them on: then each pass of a chaos point may yield the CPU
 * (sched_yield) or spin for a while, as a random number seeded by it says.
 * The library has one in lc_spinlock_acquire, once the lock is taken, so
 * the threads that want it wait, and one in the heap's lock-free push of a
 * block that a thread frees into a page of another, between its load of the
 * list's head and its compare-and-swap. The tests seed them from the
 * environment variable LC_CHAOS with tests/test.h's test_chaos_init, so a
 * run that fails can be run again with the seed it printed.
 *
 * Every other build compiles none of this: there this header declares
 * nothing, and liblightc.a has no lc_fault_ or lc_chaos_ symbol. A program
 * that uses it is compiled with -DLC_FAULTINJECT and linked with the
 * liblightc.a of a faultinject build.
 */

#include "types.h"

#ifdef LC_FAULTINJECT

/* How many rules the table holds, from one lc_fault_reset to the next. */
#define LC_FAULT_MAX_RULES 32

/*
 * A rule: counting from 1 the calls of system call `nr` made since the rule
 * was armed, it fails calls `nth` to `nth + count - 1` with -`error`, or
 * every call from `nth` on when `count` is 0. Calls before `nth` and after
 * the last it fails are made as usual.
 */
typedef struct {
    int64_t  nr;      /* the system call, as SYS_mmap */
    uint32_t nth;     /* the first call that fails: 1 is the next one */
    uint32_t count;   /* how many calls fail from there; 0 for every one */
    int32_t  error;   /* the errno they return negated, 1 to 4095, as LC_ERR_NOMEM */
} lc_fault_rule;

/*
 * Arm a copy of `*rule`, which then counts and fails calls as it says.
 * Returns the rule's id, for lc_fault_disarm and lc_fault_hits, in .value.
 * LC_ERR_INVAL when rule is NULL, nr is negative, nth is 0 or error is out
 * of its range; LC_ERR_FULL when LC_FAULT_MAX_RULES rules have been armed
 * since the last lc_fault_reset, as a disarmed rule keeps its place. When
 * more than one armed rule fails a call, the first armed gives its error,
 * and each of them counts it as one it failed.
 */
[[nodiscard]] lc_result lc_fault_arm(const lc_fault_rule *rule);

/* Disarm rule `id`, so it counts and fails no more calls; an id that no
 * armed rule has is ignored. lc_fault_hits still gives its count. */
void lc_fault_disarm(int64_t id);

/* The number of calls rule `id` has failed since it was armed, or 0 for an
 * id that no rule has. */
uint64_t lc_fault_hits(int64_t id);

/* Disarm every rule and empty the table, so that ids start from 0 again.
 * Call it while no other thread makes system calls: a call that races with
 * it can count toward a rule armed after it. */
void lc_fault_reset(void);

/* Seed the chaos points with `seed`, or turn them off with 0, and set
 * lc_chaos_count to 0. One thread that passes the same points after the
 * same seed is delayed the same way each time. */
void lc_chaos_seed(uint64_t seed);

/* Pass a chaos point: once lc_chaos_seed has seeded them, yield the CPU
 * or spin for a while, as the next random number says, or do nothing. */
void lc_chaos_point(void);

/* How many chaos points have been passed since lc_chaos_seed last seeded
 * them, or 0 while they are off. Threads that pass points at the same time
 * can each count one pass as the same, so it is exact in one thread. */
uint64_t lc_chaos_count(void);

#endif /* LC_FAULTINJECT */

#endif /* LIGHTC_FAULT_H */
