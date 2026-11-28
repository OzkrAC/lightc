#include <lightc/signal.h>
#include <lightc/syscall.h>
#include <stdatomic.h>
#include "kernel.h"          /* SA_RESTORER, SA_SIGINFO, siginfo_t, SI_*, NSIG* */

/*
 * x86_64 requires a restorer trampoline that calls rt_sigreturn.
 * The kernel does not provide one automatically on this architecture.
 *
 * This MUST be a naked function: the kernel enters it with %rsp pointing at
 * the signal frame, and rt_sigreturn (15) restores the interrupted context
 * from that exact stack location. A compiler-emitted prologue would move %rsp
 * (e.g. `push %rbp` at -O0) so the kernel reads the frame at the wrong address
 * and corrupts the restore. `naked` guarantees no prologue/epilogue at any
 * optimization level, so it must contain only basic asm and never returns.
 */
#if defined(__x86_64__)
__attribute__((naked)) static void signal_restorer(void) {
    __asm__ volatile("mov $15, %rax\n\tsyscall");
}
#endif

/* Linux's signals are 1 to 64: one bit each of the kernel's 64-bit mask. */
static bool signal_number_valid(int signo) {
    return signo >= 1 && signo <= LC_SIGNAL_MAX;
}

/* The bit of a valid signal number in a mask. */
static uint64_t signal_bit(int signo) {
    return (uint64_t)1 << (signo - 1);
}

/*
 * Internal helper: install a sigaction with the given handler, flags and
 * mask. Handles arch-specific restorer setup.
 */
static lc_result signal_set_action(int signo, void *handler, uint64_t flags, uint64_t mask) {
    lc_kernel_sigaction sa = {0};
    sa.handler = handler;
    sa.mask    = mask;

#if defined(__x86_64__)
    sa.flags    = flags | SA_RESTORER;
    sa.restorer = (void *)(uintptr_t)signal_restorer;
#else
    sa.flags = flags;
#endif

    return lc_result_from_sysret(lc_syscall4(SYS_rt_sigaction, signo, (int64_t)&sa, 0, 8));
}

lc_result lc_signal_handle(int signo, lc_signal_handler handler) {
    if (!signal_number_valid(signo)) return lc_err(LC_ERR_INVAL);
    return signal_set_action(signo, (void *)(uintptr_t)handler, 0, 0);
}

lc_result lc_signal_block(int signo) {
    if (!signal_number_valid(signo)) return lc_err(LC_ERR_INVAL);
    uint64_t mask = signal_bit(signo);
    return lc_result_from_sysret(lc_kernel_set_signal_mask(LC_SIG_BLOCK, &mask, NULL));
}

lc_result lc_signal_unblock(int signo) {
    if (!signal_number_valid(signo)) return lc_err(LC_ERR_INVAL);
    uint64_t mask = signal_bit(signo);
    return lc_result_from_sysret(lc_kernel_set_signal_mask(LC_SIG_UNBLOCK, &mask, NULL));
}

void lc_on_crash(lc_crash_handler handler) {
    (void)lc_signal_handle(LC_SIGSEGV, handler);
    (void)lc_signal_handle(LC_SIGBUS,  handler);
    (void)lc_signal_handle(LC_SIGABRT, handler);
}

void lc_on_shutdown(lc_signal_handler handler) {
    (void)lc_signal_handle(LC_SIGTERM, handler);
    (void)lc_signal_handle(LC_SIGINT,  handler);
}

lc_result lc_signal_ignore(int signo) {
    if (!signal_number_valid(signo)) return lc_err(LC_ERR_INVAL);
    return signal_set_action(signo, SIG_IGN, 0, 0);
}

lc_result lc_signal_reset(int signo) {
    if (!signal_number_valid(signo)) return lc_err(LC_ERR_INVAL);
    lc_kernel_sigaction sa = {0};
    sa.handler = SIG_DFL;
    sa.flags   = 0;
    sa.restorer = NULL;
    sa.mask = 0;
    return lc_result_from_sysret(lc_syscall4(SYS_rt_sigaction, signo, (int64_t)&sa, 0, 8));
}

/* --- Sets of signals --- */

lc_signal_set lc_signal_set_empty(void) {
    return (lc_signal_set){ 0 };
}

lc_signal_set lc_signal_set_full(void) {
    return (lc_signal_set){ UINT64_MAX };
}

lc_result lc_signal_set_add(lc_signal_set *set, int signo) {
    if (!signal_number_valid(signo)) return lc_err(LC_ERR_INVAL);
    set->bits |= signal_bit(signo);
    return lc_ok(0);
}

lc_result lc_signal_set_remove(lc_signal_set *set, int signo) {
    if (!signal_number_valid(signo)) return lc_err(LC_ERR_INVAL);
    set->bits &= ~signal_bit(signo);
    return lc_ok(0);
}

bool lc_signal_set_has(lc_signal_set set, int signo) {
    return signal_number_valid(signo) && (set.bits & signal_bit(signo)) != 0;
}

/* --- lc_signal_action: a handler that gets an lc_signal_info --- */

/* The flags lc_signal_action takes, each the kernel's SA_* value. */
#define SIGNAL_ACTION_FLAGS  LC_SIGNAL_RESTART

/*
 * The handler of lc_signal_action for each signal, signal n at n - 1.
 * lc_signal_action stores to it and signal_trampoline loads from it, in a
 * signal handler, so each slot is an atomic pointer: a load that a store
 * races with gives the old handler or the new one, never a torn one. A
 * slot is NULL until lc_signal_action first installs a handler for its
 * signal; only a failed first install puts NULL back.
 */
static _Atomic(lc_signal_action_handler) actions[LC_SIGNAL_MAX];

/* What the kernel's siginfo_layout (kernel/signal.c) calls SIL_*: which
 * fields of the siginfo_t a signal's kind fills. */
typedef enum {
    LAYOUT_KILL,
    LAYOUT_TIMER,
    LAYOUT_POLL,
    LAYOUT_FAULT,
    LAYOUT_FAULT_MCEERR,
    LAYOUT_CHLD,
    LAYOUT_RT,
    LAYOUT_SYS,
} signal_layout;

/*
 * The kind of a signal of number signo and si_code code, as the kernel's
 * siginfo_layout gives it on x86_64 and aarch64. A code from 1 to 127 is
 * the kernel's: it is one of the signal's own when the signal has codes of
 * its own and it is no more than their number, else one of SIGPOLL's when
 * it is no more than theirs. A code of 0 is kill's, and SI_KERNEL the
 * kernel's own; of those below 0, SI_TIMER is a POSIX timer's and SI_SIGIO
 * a queued SIGIO's, and the others are queued signals, tkill's among them.
 * The SIL_FAULT_* kinds that differ from SIL_FAULT only in fields that a
 * signalfd does not give are LAYOUT_FAULT here, and SIL_FAULT_TRAPNO is
 * only on other arches.
 */
static signal_layout signal_layout_of(int signo, int code) {
    if (code > SI_USER && code < SI_KERNEL) {
        int limit = 0;
        signal_layout own = LAYOUT_KILL;
        switch (signo) {
            case LC_SIGILL:  limit = NSIGILL;  own = LAYOUT_FAULT; break;
            case LC_SIGFPE:  limit = NSIGFPE;  own = LAYOUT_FAULT; break;
            case LC_SIGSEGV: limit = NSIGSEGV; own = LAYOUT_FAULT; break;
            case LC_SIGBUS:  limit = NSIGBUS;  own = LAYOUT_FAULT; break;
            case LC_SIGTRAP: limit = NSIGTRAP; own = LAYOUT_FAULT; break;
            case SIGCHLD:    limit = NSIGCHLD; own = LAYOUT_CHLD;  break;
            case SIGPOLL:    limit = NSIGPOLL; own = LAYOUT_POLL;  break;
            case LC_SIGSYS:  limit = NSIGSYS;  own = LAYOUT_SYS;   break;
            default: break;
        }
        if (code <= limit) {
            if (signo == LC_SIGBUS && code >= BUS_MCEERR_AR && code <= BUS_MCEERR_AO)
                return LAYOUT_FAULT_MCEERR;
            return own;
        }
        return code <= NSIGPOLL ? LAYOUT_POLL : LAYOUT_KILL;
    }
    if (code == SI_TIMER) return LAYOUT_TIMER;
    if (code == SI_SIGIO) return LAYOUT_POLL;
    if (code < 0) return LAYOUT_RT;
    return LAYOUT_KILL;
}

/*
 * Fill *info from the kernel's siginfo_t, as the kernel's signalfd_copyinfo
 * (fs/signalfd.c) fills the signalfd_siginfo that a read of a signalfd
 * gives: every field zero, then signal, error and code, then the fields of
 * the signal's kind, with the same conversions.
 */
static void signal_info_fill(lc_signal_info *info, const siginfo_t *si) {
    *info = (lc_signal_info){ 0 };
    info->signal = (uint32_t)si->si_signo;
    info->error  = si->si_errno;
    info->code   = si->si_code;
    switch (signal_layout_of(si->si_signo, si->si_code)) {
        case LAYOUT_KILL:
            info->pid = (uint32_t)si->_sifields._kill._pid;
            info->uid = si->_sifields._kill._uid;
            break;
        case LAYOUT_TIMER:
            info->tid     = (uint32_t)si->_sifields._timer._tid;
            info->overrun = (uint32_t)si->_sifields._timer._overrun;
            info->ptr_val = (uint64_t)(uintptr_t)si->_sifields._rt._sigval.sival_ptr;
            info->int_val = si->_sifields._rt._sigval.sival_int;
            break;
        case LAYOUT_POLL:
            info->band = (uint32_t)si->_sifields._sigpoll._band;
            info->fd   = si->_sifields._sigpoll._fd;
            break;
        case LAYOUT_FAULT:
            info->addr = (uint64_t)(uintptr_t)si->_sifields._sigfault._addr;
            break;
        case LAYOUT_FAULT_MCEERR:
            info->addr     = (uint64_t)(uintptr_t)si->_sifields._sigfault._addr;
            info->addr_lsb = (uint16_t)si->_sifields._sigfault._addr_lsb;
            break;
        case LAYOUT_CHLD:
            info->pid    = (uint32_t)si->_sifields._sigchld._pid;
            info->uid    = si->_sifields._sigchld._uid;
            info->status = si->_sifields._sigchld._status;
            info->utime  = (uint64_t)si->_sifields._sigchld._utime;
            info->stime  = (uint64_t)si->_sifields._sigchld._stime;
            break;
        case LAYOUT_RT:
            info->pid     = (uint32_t)si->_sifields._rt._pid;
            info->uid     = si->_sifields._rt._uid;
            info->ptr_val = (uint64_t)(uintptr_t)si->_sifields._rt._sigval.sival_ptr;
            info->int_val = si->_sifields._rt._sigval.sival_int;
            break;
        case LAYOUT_SYS:
            info->call_addr = (uint64_t)(uintptr_t)si->_sifields._sigsys._call_addr;
            info->syscall   = si->_sifields._sigsys._syscall;
            info->arch      = si->_sifields._sigsys._arch;
            break;
    }
}

/*
 * The handler lc_signal_action installs, with SA_SIGINFO, for every signal:
 * it finds the signal's handler in actions, fills an lc_signal_info on its
 * own stack from the kernel's siginfo_t and calls the handler with it and
 * the kernel's context.
 *
 * It runs in a signal handler, so it touches only its stack frame, the
 * kernel's siginfo_t and one slot of actions, with an atomic load: it makes
 * no system call, takes no lock and allocates nothing, so it changes no
 * state that the code it interrupted could see, and it reads no thread
 * pointer. It returns to the kernel's restorer, or on x86_64 to
 * signal_restorer, which rt_sigreturn then leaves for the interrupted code.
 */
static void signal_trampoline(int signo, siginfo_t *si, void *context) {
    if (!signal_number_valid(signo)) return;
    lc_signal_action_handler handler =
        atomic_load_explicit(&actions[signo - 1], memory_order_acquire);
    if (handler == NULL) return;
    lc_signal_info info;
    signal_info_fill(&info, si);
    handler(signo, &info, context);
}

lc_result lc_signal_action(int signo, lc_signal_action_handler handler,
                           lc_signal_set mask, uint32_t flags) {
    if (!signal_number_valid(signo) || handler == NULL || (flags & ~SIGNAL_ACTION_FLAGS) != 0)
        return lc_err(LC_ERR_INVAL);

    /* The slot first, so the trampoline finds the new handler from the
     * moment the kernel can call it for this signal. */
    _Atomic(lc_signal_action_handler) *slot = &actions[signo - 1];
    lc_signal_action_handler previous =
        atomic_exchange_explicit(slot, handler, memory_order_acq_rel);

    lc_result r = signal_set_action(signo, (void *)(uintptr_t)signal_trampoline,
                                    SA_SIGINFO | (uint64_t)flags, mask.bits);
    if (lc_is_err(r)) {
        /* The kernel kept what it had, so the slot goes back to its handler,
         * unless another call has stored one since. */
        lc_signal_action_handler expected = handler;
        (void)atomic_compare_exchange_strong_explicit(slot, &expected, previous,
                                                      memory_order_acq_rel,
                                                      memory_order_relaxed);
    }
    return r;
}
