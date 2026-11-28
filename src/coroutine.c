#include <lightc/coroutine.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/heap.h>
#include <lightc/thread.h>
#include <lightc/memory.h>
#include "internal.h"          /* LC_SLOT_SCHEDULER, guarded_stack_map */

/*
 * Verify that the assembly context-switch offsets match the C struct layout.
 * If any of these fire, the assembly and C disagree on struct layout.
 */
#if defined(__x86_64__)
_Static_assert(sizeof(lc_coroutine_context) == 56,
               "x86_64 lc_coroutine_context must be 7 * 8 = 56 bytes");
#elif defined(__aarch64__)
_Static_assert(sizeof(lc_coroutine_context) == 168,
               "aarch64 lc_coroutine_context must be 21 * 8 = 168 bytes"
               " (x19-x28, x29, x30, sp, d8-d15)");
#endif

/* Implemented in arch-specific assembly (arch/{x86_64,aarch64}/coroutine.S) */
extern void lc_coroutine_switch(lc_coroutine_context *save,
                                lc_coroutine_context *restore);

/*
 * Per-thread pointer to the currently active scheduler, so each thread can
 * run its own. It is in the library's slot of the per-thread block, not in
 * a TLS key: with every key taken, the key the scheduler failed to get was
 * 0, the program's own.
 */
static lc_scheduler *get_current_scheduler(void) {
    void **slots = lc_heap_tls_slots();
    return slots ? (lc_scheduler *)slots[LC_SLOT_SCHEDULER] : NULL;
}

static void set_current_scheduler(lc_scheduler *sched) {
    void **slots = lc_heap_tls_slots();
    if (slots) slots[LC_SLOT_SCHEDULER] = sched;
}

/*
 * Trampoline — entered when a coroutine is first switched to.
 *
 * The context switch "returns" into this function (via ret/lr).
 * It calls the user's function, marks the coroutine finished,
 * and yields one last time to return control to the scheduler.
 */
static void coroutine_trampoline(void) {
    lc_scheduler *sched = get_current_scheduler();
    lc_coroutine *co = &sched->coroutines[sched->current];

    /* Run the user's coroutine function */
    co->func(co->arg);

    /* Mark as finished */
    co->state = LC_COROUTINE_FINISHED;
    sched->active_count--;

    /* Yield back — scheduler will find the next coroutine or return to main */
    lc_coroutine_yield();

    /* Should never reach here — the coroutine is finished and will never
     * be switched back to. */
    __builtin_unreachable();
}

/* --- Scheduler API --- */

lc_scheduler lc_scheduler_create_with_capacity(uint32_t max_coroutines) {
    lc_scheduler sched;
    lc_bytes_fill(&sched, 0, sizeof(sched));
    sched.capacity = max_coroutines;
    if (max_coroutines > 0) {
        lc_result_ptr alloc = lc_heap_allocate_zeroed(max_coroutines * sizeof(lc_coroutine));
        if (lc_ptr_is_err(alloc)) {
            /* Allocation failed — leave coroutines NULL but drop capacity to 0 so
             * lc_coroutine_create refuses instead of writing through a NULL array. */
            sched.capacity = 0;
        } else {
            sched.coroutines = alloc.value;
        }
    }
    return sched;
}

lc_scheduler lc_scheduler_create(void) {
    return lc_scheduler_create_with_capacity(LC_MAX_COROUTINES);
}

lc_coroutine *lc_coroutine_create(lc_scheduler *sched, lc_coroutine_func func, void *arg) {
    if (sched->count >= sched->capacity) return NULL;

    lc_coroutine *co = &sched->coroutines[sched->count];
    sched->count++;

    /* A stack with a guard page below it, and a context that enters the
     * trampoline on it, aligned as after a call. */
    size_t total_size;
    void *stack = guarded_stack_map(LC_COROUTINE_STACK_SIZE, &total_size);
    if (stack == NULL) {
        sched->count--;
        return NULL;
    }

    co->func       = func;
    co->arg        = arg;
    co->stack_base = stack;
    co->stack_size = total_size;
    co->scheduler  = sched;
    co->state      = LC_COROUTINE_READY;
    coroutine_context_init(&co->context, stack, total_size, coroutine_trampoline);

    return co;
}

void lc_scheduler_run(lc_scheduler *sched) {
    if (sched->count == 0) return;

    /* The scheduler current when we were called: none at top level, the outer
     * one when a coroutine runs a scheduler of its own. Every return below
     * makes it current again, so a later yield outside any coroutine of
     * `sched` is a no-op at top level and goes to the outer scheduler when
     * nested, never to the main_context of this finished run. */
    lc_scheduler *prev = get_current_scheduler();
    set_current_scheduler(sched);

    /*
     * Count the runnable (not-yet-finished) coroutines and find the first one.
     * A FINISHED coroutine must NOT be resurrected: its saved context sits just
     * past its final yield, right before __builtin_unreachable(), so switching
     * back into it is undefined behavior. This also makes re-running a
     * completed scheduler a safe no-op, and lets coroutines added after an
     * earlier run() start cleanly on a later run().
     */
    uint32_t runnable = 0;
    uint32_t first = sched->count;  /* sentinel = "none found" */
    for (uint32_t i = 0; i < sched->count; i++) {
        if (sched->coroutines[i].state != LC_COROUTINE_FINISHED) {
            if (first == sched->count) first = i;
            runnable++;
        }
    }
    if (runnable == 0) {  /* nothing left to run */
        set_current_scheduler(prev);
        return;
    }

    sched->active_count = runnable;

    /* Start the first runnable coroutine */
    sched->current = first;
    sched->coroutines[first].state = LC_COROUTINE_RUNNING;
    lc_coroutine_switch(&sched->main_context, &sched->coroutines[first].context);

    /*
     * We return here when all coroutines have finished.
     * (lc_coroutine_yield switches back to main_context when none remain.)
     */
    set_current_scheduler(prev);
}

void lc_coroutine_yield(void) {
    lc_scheduler *sched = get_current_scheduler();
    /* Called outside a running scheduler (no scheduler on this thread, or an
     * empty one): nothing to yield to. Return instead of NULL-dereferencing
     * sched or dividing by a zero coroutine count below. */
    if (sched == NULL || sched->count == 0) return;

    uint32_t current_idx = sched->current;
    lc_coroutine *current_co = &sched->coroutines[current_idx];

    /* Mark current as ready (unless it just finished) */
    if (current_co->state == LC_COROUTINE_RUNNING) {
        current_co->state = LC_COROUTINE_READY;
    }

    /* Find next non-finished coroutine (round-robin) */
    uint32_t next_idx = current_idx;
    bool found = false;
    for (uint32_t i = 1; i <= sched->count; i++) {
        uint32_t candidate = (current_idx + i) % sched->count;
        if (sched->coroutines[candidate].state != LC_COROUTINE_FINISHED) {
            next_idx = candidate;
            found = true;
            break;
        }
    }

    if (!found) {
        /* No more coroutines to run — return to lc_scheduler_run */
        lc_coroutine_switch(&current_co->context, &sched->main_context);
        return;
    }

    /* Switch to the next coroutine */
    sched->current = next_idx;
    sched->coroutines[next_idx].state = LC_COROUTINE_RUNNING;
    lc_coroutine_switch(&current_co->context, &sched->coroutines[next_idx].context);
}

void lc_scheduler_destroy(lc_scheduler *sched) {
    /* Never leave this thread's current scheduler pointing at one that is
     * gone: a later yield would read its freed coroutines. */
    if (get_current_scheduler() == sched) set_current_scheduler(NULL);
    for (uint32_t i = 0; i < sched->count; i++) {
        if (sched->coroutines[i].stack_base != NULL) {
            lc_kernel_unmap_memory(sched->coroutines[i].stack_base,
                                   sched->coroutines[i].stack_size);
            sched->coroutines[i].stack_base = NULL;
        }
    }
    lc_heap_free(sched->coroutines);
    sched->coroutines = NULL;
    sched->count = 0;
    sched->active_count = 0;
    sched->capacity = 0;
}
