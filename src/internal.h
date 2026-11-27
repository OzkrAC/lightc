/*
 * internal.h — what more than one of lightc's sources share, and no
 * program may use: the library's own slots of the per-thread block, and
 * the guarded stacks of coroutines.
 *
 * Everything here is static inline or an enumerator, so it adds no symbol
 * to liblightc.a.
 */

#ifndef LIGHTC_SRC_INTERNAL_H
#define LIGHTC_SRC_INTERNAL_H

#include <lightc/coroutine.h>
#include <lightc/memory.h>
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>

/* --- The library's slots of the per-thread block ---
 *
 * The block that lc_heap_tls_slots gives has LC_TLS_MAX_KEYS slots for the
 * keys of lc_tls_key_create, and these after them, which no key reaches:
 * the library's own per-thread pointers. A program that takes every key
 * therefore cannot make the library share one of its keys. As the keys'
 * slots do, these start NULL in a new thread and in one that reuses the id
 * of a thread that has exited. */
enum {
    LC_SLOT_SCHEDULER = LC_TLS_MAX_KEYS,   /* coroutine.c: the current lc_scheduler */
    LC_SLOT_LIO_LOOP,                      /* lightio.c: the current lio_loop */
    LC_SLOT_END                            /* the size of the block */
};

/* --- Guarded coroutine stacks --- */

/* Maps a stack of `usable` bytes above a PROT_NONE guard of one page, so an
 * overflow faults where it starts instead of running into the mapping
 * below. The guard is the kernel's page, as mprotect rounds a shorter
 * length up to a whole one. Returns the mapping's base, the guard's start,
 * and sets *total to the size to unmap; returns NULL, with nothing mapped,
 * when either call fails. */
static inline void *guarded_stack_map(size_t usable, size_t *total) {
    size_t guard = lc_runtime_page_size();
    size_t size = usable + guard;
    void *base = lc_kernel_map_memory(NULL, size, PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) return NULL;
    if (lc_kernel_protect_memory(base, guard, PROT_NONE) < 0) {
        lc_kernel_unmap_memory(base, size);
        return NULL;
    }
    *total = size;
    return base;
}

/* Sets `context` so that the first lc_coroutine_switch to it enters `entry`
 * on the stack mapped at [base, base + total), with every other register
 * 0. The stack pointer is what the ABI has at a function's entry.
 *
 * x86_64: lc_coroutine_switch's `ret` pops the return address off the
 * stack, so `entry` goes in the low slot of 16 bytes at the top. rsp stays
 * 16-byte aligned, and the `ret` enters `entry` with rsp ≡ 8 mod 16, as
 * after a call. Reserving 8 bytes would enter it with rsp ≡ 0 mod 16, and
 * the first aligned SSE spill (movaps) of its call tree would fault.
 *
 * aarch64: the switch restores lr and returns to it, so `entry` goes in x30,
 * and sp is the 16-byte-aligned top. */
static inline void coroutine_context_init(lc_coroutine_context *context, void *base, size_t total,
                                          void (*entry)(void)) {
    lc_bytes_fill(context, 0, sizeof(*context));
    uint64_t *stack_top = (uint64_t *)((uintptr_t)((uint8_t *)base + total) & ~(uintptr_t)15);
#if defined(__x86_64__)
    stack_top -= 2;
    *stack_top = (uint64_t)(uintptr_t)entry;
    context->rsp = (uint64_t)(uintptr_t)stack_top;
#elif defined(__aarch64__)
    context->sp  = (uint64_t)(uintptr_t)stack_top;
    context->x30 = (uint64_t)(uintptr_t)entry;
#endif
}

#endif /* LIGHTC_SRC_INTERNAL_H */
