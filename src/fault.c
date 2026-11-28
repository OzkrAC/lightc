/*
 * lightc — fault injection and chaos points, for the faultinject build
 *
 * <lightc/fault.h> says what they do. ./configure faultinject defines
 * LC_FAULTINJECT for the C sources and the assembly. With it, the assembly
 * names its entry points that make system calls lc_fault_raw_syscall0 to
 * lc_fault_raw_syscall6, lc_fault_raw_kernel_clone and
 * lc_fault_raw_thread_start, and this file defines lc_syscall0 to
 * lc_syscall6, lc_kernel_clone and lc_thread_start, under the names the
 * rest of lightc calls, as C functions that check the armed rules before
 * they call those. Every other build compiles nothing of this file, which
 * then only includes headers.
 *
 * The shims pass their nr on as it came. ci/abilint.sh checks the arity of
 * each lc_syscallN call by its SYS_ name, so it reads the definitions here,
 * "lc_syscallN(int64_t nr, ...)", as it reads the prototypes in
 * <lightc/syscall.h>, and fails on an lc_fault_raw_ name outside this file:
 * only the shims may make a system call that no rule checks.
 */

#include <lightc/fault.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>

#ifdef LC_FAULTINJECT

#include <stdatomic.h>

/* --- The assembly entry points, as the faultinject build names them --- */

lc_sysret lc_fault_raw_syscall0(int64_t nr);
lc_sysret lc_fault_raw_syscall1(int64_t nr, int64_t a0);
lc_sysret lc_fault_raw_syscall2(int64_t nr, int64_t a0, int64_t a1);
lc_sysret lc_fault_raw_syscall3(int64_t nr, int64_t a0, int64_t a1, int64_t a2);
lc_sysret lc_fault_raw_syscall4(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3);
lc_sysret lc_fault_raw_syscall5(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                                int64_t a4);
lc_sysret lc_fault_raw_syscall6(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                                int64_t a4, int64_t a5);
lc_sysret lc_fault_raw_kernel_clone(int64_t flags, void *stack_top, int (*fn)(void *), void *arg);
int64_t lc_fault_raw_thread_start(uint64_t flags, uint8_t *stack_top,
                                  int32_t *parent_tid, int32_t *child_tid, void *tls,
                                  lc_thread_func func, void *arg);

/* What thread.c calls to start a thread, as thread.c declares it. */
int64_t lc_thread_start(uint64_t flags, uint8_t *stack_top,
                        int32_t *parent_tid, int32_t *child_tid, void *tls,
                        lc_thread_func func, void *arg);

/* --- The rule table ---
 *
 * A call reads the table without a lock, so a signal handler can make one
 * while the thread it interrupted is in the middle of lc_fault_arm, and the
 * child of a raw clone, which has no thread of its own, can too. Each slot
 * is a sequence lock: its writer makes `sequence` odd, changes the rule's
 * fields and makes it even again, and a reader that sees it odd, or sees it
 * change while it reads the fields, skips the slot, as if the change had
 * not begun. So no call takes one rule's system call with another's errno.
 * A slot goes to a new rule only after lc_fault_reset, so a call counts
 * toward the rule whose fields it read, unless it races with that reset.
 * Writers take fault_writer, a lock of their own: lc_spinlock_acquire has a
 * chaos point in it.
 */

typedef struct {
    _Atomic(uint32_t) sequence;   /* odd while a writer changes the fields below */
    _Atomic(bool)     armed;
    _Atomic(int64_t)  nr;
    _Atomic(uint32_t) nth;
    _Atomic(uint32_t) count;
    _Atomic(int32_t)  error;
    _Atomic(uint64_t) calls;      /* the calls of nr counted since the arming */
    _Atomic(uint64_t) hits;       /* the calls the rule failed */
} fault_slot;

static fault_slot fault_table[LC_FAULT_MAX_RULES];
static _Atomic(uint32_t) fault_used;    /* slots given to rules since the last reset */
static _Atomic(uint32_t) fault_armed;   /* rules armed now: with none, a call checks nothing */
static _Atomic(uint32_t) fault_writer;  /* 1 while lc_fault_arm, _disarm or _reset runs */

static void writer_lock(void) {
    while (atomic_exchange_explicit(&fault_writer, 1, memory_order_acquire) != 0) {
        /* spin: writers are short, and tests arm from one thread */
    }
}

static void writer_unlock(void) {
    atomic_store_explicit(&fault_writer, 0, memory_order_release);
}

/* Begin and end a change of `slot`'s fields. The caller holds the writer
 * lock. The fence keeps the field stores after the odd sequence, the release
 * store keeps them before the even one. */
static void slot_write_begin(fault_slot *slot) {
    uint32_t s = atomic_load_explicit(&slot->sequence, memory_order_relaxed);
    atomic_store_explicit(&slot->sequence, s + 1, memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
}

static void slot_write_end(fault_slot *slot) {
    uint32_t s = atomic_load_explicit(&slot->sequence, memory_order_relaxed);
    atomic_store_explicit(&slot->sequence, s + 1, memory_order_release);
}

/* The negated errno with which an armed rule fails this call of `nr`, or 0
 * when the call is to be made. Every armed rule of nr counts the call. */
static lc_sysret fault_check(int64_t nr) {
    if (atomic_load_explicit(&fault_armed, memory_order_acquire) == 0) return 0;

    uint32_t used = atomic_load_explicit(&fault_used, memory_order_acquire);
    if (used > LC_FAULT_MAX_RULES) used = LC_FAULT_MAX_RULES;

    lc_sysret result = 0;
    for (uint32_t i = 0; i < used; i++) {
        fault_slot *slot = &fault_table[i];
        uint32_t before = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        if (before & 1) continue;
        bool armed     = atomic_load_explicit(&slot->armed, memory_order_relaxed);
        int64_t rule   = atomic_load_explicit(&slot->nr, memory_order_relaxed);
        uint32_t nth   = atomic_load_explicit(&slot->nth, memory_order_relaxed);
        uint32_t count = atomic_load_explicit(&slot->count, memory_order_relaxed);
        int32_t error  = atomic_load_explicit(&slot->error, memory_order_relaxed);
        atomic_thread_fence(memory_order_acquire);
        if (atomic_load_explicit(&slot->sequence, memory_order_relaxed) != before) continue;
        if (!armed || rule != nr) continue;

        uint64_t call = atomic_fetch_add_explicit(&slot->calls, 1, memory_order_relaxed) + 1;
        if (call < nth) continue;
        if (count != 0 && call - nth >= count) continue;
        atomic_fetch_add_explicit(&slot->hits, 1, memory_order_relaxed);
        if (result == 0) result = -(lc_sysret)error;
    }
    return result;
}

lc_result lc_fault_arm(const lc_fault_rule *rule) {
    if (rule == NULL || rule->nr < 0 || rule->nth == 0 ||
        rule->error < 1 || rule->error > 4095)
        return lc_err(LC_ERR_INVAL);

    writer_lock();
    uint32_t id = atomic_load_explicit(&fault_used, memory_order_relaxed);
    if (id >= LC_FAULT_MAX_RULES) {
        writer_unlock();
        return lc_err(LC_ERR_FULL);
    }
    fault_slot *slot = &fault_table[id];
    slot_write_begin(slot);
    atomic_store_explicit(&slot->nr, rule->nr, memory_order_relaxed);
    atomic_store_explicit(&slot->nth, rule->nth, memory_order_relaxed);
    atomic_store_explicit(&slot->count, rule->count, memory_order_relaxed);
    atomic_store_explicit(&slot->error, rule->error, memory_order_relaxed);
    atomic_store_explicit(&slot->calls, 0, memory_order_relaxed);
    atomic_store_explicit(&slot->hits, 0, memory_order_relaxed);
    atomic_store_explicit(&slot->armed, true, memory_order_relaxed);
    slot_write_end(slot);
    /* A reader that sees the new count sees the slot whole. */
    atomic_store_explicit(&fault_used, id + 1, memory_order_release);
    atomic_fetch_add_explicit(&fault_armed, 1, memory_order_release);
    writer_unlock();
    return lc_ok(id);
}

void lc_fault_disarm(int64_t id) {
    if (id < 0 || id >= LC_FAULT_MAX_RULES) return;
    writer_lock();
    fault_slot *slot = &fault_table[id];
    if ((uint64_t)id < atomic_load_explicit(&fault_used, memory_order_relaxed) &&
        atomic_load_explicit(&slot->armed, memory_order_relaxed)) {
        slot_write_begin(slot);
        atomic_store_explicit(&slot->armed, false, memory_order_relaxed);
        slot_write_end(slot);
        atomic_fetch_sub_explicit(&fault_armed, 1, memory_order_release);
    }
    writer_unlock();
}

uint64_t lc_fault_hits(int64_t id) {
    if (id < 0 || id >= LC_FAULT_MAX_RULES) return 0;
    return atomic_load_explicit(&fault_table[id].hits, memory_order_relaxed);
}

void lc_fault_reset(void) {
    writer_lock();
    atomic_store_explicit(&fault_armed, 0, memory_order_release);
    uint32_t used = atomic_load_explicit(&fault_used, memory_order_relaxed);
    for (uint32_t i = 0; i < used && i < LC_FAULT_MAX_RULES; i++) {
        fault_slot *slot = &fault_table[i];
        slot_write_begin(slot);
        atomic_store_explicit(&slot->armed, false, memory_order_relaxed);
        atomic_store_explicit(&slot->nr, 0, memory_order_relaxed);
        atomic_store_explicit(&slot->calls, 0, memory_order_relaxed);
        atomic_store_explicit(&slot->hits, 0, memory_order_relaxed);
        slot_write_end(slot);
    }
    atomic_store_explicit(&fault_used, 0, memory_order_release);
    writer_unlock();
}

/* --- The shims: what the rest of lightc calls --- */

lc_sysret lc_syscall0(int64_t nr) {
    lc_sysret fault = fault_check(nr);
    if (fault < 0) return fault;
    return lc_fault_raw_syscall0(nr);
}

lc_sysret lc_syscall1(int64_t nr, int64_t a0) {
    lc_sysret fault = fault_check(nr);
    if (fault < 0) return fault;
    return lc_fault_raw_syscall1(nr, a0);
}

lc_sysret lc_syscall2(int64_t nr, int64_t a0, int64_t a1) {
    lc_sysret fault = fault_check(nr);
    if (fault < 0) return fault;
    return lc_fault_raw_syscall2(nr, a0, a1);
}

lc_sysret lc_syscall3(int64_t nr, int64_t a0, int64_t a1, int64_t a2) {
    lc_sysret fault = fault_check(nr);
    if (fault < 0) return fault;
    return lc_fault_raw_syscall3(nr, a0, a1, a2);
}

lc_sysret lc_syscall4(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3) {
    lc_sysret fault = fault_check(nr);
    if (fault < 0) return fault;
    return lc_fault_raw_syscall4(nr, a0, a1, a2, a3);
}

lc_sysret lc_syscall5(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4) {
    lc_sysret fault = fault_check(nr);
    if (fault < 0) return fault;
    return lc_fault_raw_syscall5(nr, a0, a1, a2, a3, a4);
}

lc_sysret lc_syscall6(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4,
                      int64_t a5) {
    lc_sysret fault = fault_check(nr);
    if (fault < 0) return fault;
    return lc_fault_raw_syscall6(nr, a0, a1, a2, a3, a4, a5);
}

/* clone.S and thread.S make their clone themselves, so the SYS_clone rules
 * are checked here. A child never returns into these: it runs fn or func
 * on the stack it was given, and exits from the assembly. */

lc_sysret lc_kernel_clone(int64_t flags, void *stack_top, int (*fn)(void *), void *arg) {
    lc_sysret fault = fault_check(SYS_clone);
    if (fault < 0) return fault;
    return lc_fault_raw_kernel_clone(flags, stack_top, fn, arg);
}

int64_t lc_thread_start(uint64_t flags, uint8_t *stack_top,
                        int32_t *parent_tid, int32_t *child_tid, void *tls,
                        lc_thread_func func, void *arg) {
    lc_sysret fault = fault_check(SYS_clone);
    if (fault < 0) return fault;
    return lc_fault_raw_thread_start(flags, stack_top, parent_tid, child_tid, tls, func, arg);
}

/* --- Chaos points ---
 *
 * Each pass takes the next number of a sequence that the seed starts, and
 * mixes it with splitmix64's finalizer into 64 random bits: 1 pass in 8
 * yields, 1 in 4 spins for 1 to 4096 rounds of the CPU's spin-wait hint,
 * and the rest do nothing. The seed and the pass count are read with
 * relaxed loads and the count written with a relaxed store, not a
 * read-modify-write, so a pass that does not yield adds no fence
 * instruction to the code around it: a locked add is a full fence on
 * x86_64, and could hide the very reorderings that the point is there to
 * show. Two threads that pass points at once can then take the same number.
 */

static _Atomic(uint64_t) chaos_seed_value;   /* 0: the points do nothing */
static _Atomic(uint64_t) chaos_passes;       /* points passed since the seed */

void lc_chaos_seed(uint64_t seed) {
    atomic_store_explicit(&chaos_passes, 0, memory_order_relaxed);
    atomic_store_explicit(&chaos_seed_value, seed, memory_order_relaxed);
}

uint64_t lc_chaos_count(void) {
    return atomic_load_explicit(&chaos_passes, memory_order_relaxed);
}

static uint64_t chaos_mix(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}

static inline void chaos_spin_hint(void) {
#if defined(__x86_64__)
    __asm__ volatile("pause" ::: "memory");
#elif defined(__aarch64__)
    __asm__ volatile("yield" ::: "memory");
#endif
}

void lc_chaos_point(void) {
    uint64_t seed = atomic_load_explicit(&chaos_seed_value, memory_order_relaxed);
    if (seed == 0) return;

    uint64_t pass = atomic_load_explicit(&chaos_passes, memory_order_relaxed);
    atomic_store_explicit(&chaos_passes, pass + 1, memory_order_relaxed);

    uint64_t r = chaos_mix(seed + (pass + 1) * 0x9e3779b97f4a7c15ull);
    switch (r & 7) {
    case 0:
        (void)lc_syscall0(SYS_sched_yield);
        break;
    case 1:
    case 2: {
        uint64_t rounds = 1 + (r >> 3) % 4096;
        for (uint64_t i = 0; i < rounds; i++)
            chaos_spin_hint();
        break;
    }
    default:
        break;
    }
}

#endif /* LC_FAULTINJECT */
