#ifndef LIGHTC_SIGNAL_H
#define LIGHTC_SIGNAL_H

#include <lightc/types.h>

/* Signal numbers (Linux x86_64/aarch64) */
#define LC_SIGINT    2
#define LC_SIGQUIT   3
#define LC_SIGABRT   6
#define LC_SIGBUS    7
#define LC_SIGSEGV  11
#define LC_SIGPIPE  13
#define LC_SIGALRM  14
#define LC_SIGTERM  15

/*
 * More signal numbers, the same on x86_64 and aarch64. Linux numbers its
 * signals 1 to LC_SIGNAL_MAX. Each function of this header that takes a
 * signal number returns LC_ERR_INVAL for one outside that range, and then
 * makes no system call.
 */
#define LC_SIGHUP    1
#define LC_SIGILL    4
#define LC_SIGTRAP   5
#define LC_SIGFPE    8
#define LC_SIGUSR1  10
#define LC_SIGSYS   31

/* The highest signal number: a mask of the kernel's has one bit for each
 * of the 64 signals. */
#define LC_SIGNAL_MAX  64

/* Signal handler types */
typedef void (*lc_signal_handler)(int signo);
typedef void (*lc_crash_handler)(int signo);

/*
 * Install a handler for a specific signal. The handler gets the signal's
 * number. It is installed without SA_RESTART, so a signal that interrupts
 * a blocking system call ends it with EINTR, and it blocks no signal but
 * signo while it runs; lc_signal_action can restart the call, block more
 * and give the handler what the kernel tells of the signal. A number
 * outside 1 to LC_SIGNAL_MAX gives LC_ERR_INVAL; the kernel refuses
 * SIGKILL (9) and SIGSTOP (19) with it too.
 */
[[nodiscard]] lc_result lc_signal_handle(int signo, lc_signal_handler handler);

/* Block a signal. It is blocked in the calling thread only. A number
 * outside 1 to LC_SIGNAL_MAX gives LC_ERR_INVAL and leaves the thread's
 * mask as it was. */
[[nodiscard]] lc_result lc_signal_block(int signo);

/* Unblock a signal. It is unblocked in the calling thread only. A number
 * outside 1 to LC_SIGNAL_MAX gives LC_ERR_INVAL and leaves the thread's
 * mask as it was. */
[[nodiscard]] lc_result lc_signal_unblock(int signo);

/* Install a handler for the crash signals: SIGSEGV, SIGBUS and SIGABRT. */
void lc_on_crash(lc_crash_handler handler);

/* Install a handler for the shutdown signals: SIGTERM and SIGINT. */
void lc_on_shutdown(lc_signal_handler handler);

/* Ignore a signal */
[[nodiscard]] lc_result lc_signal_ignore(int signo);

/* Reset a signal to default behavior */
[[nodiscard]] lc_result lc_signal_reset(int signo);

/* --- Sets of signals --- */

/*
 * A set of signals, as the kernel's sigset_t holds it on x86_64 and
 * aarch64: signal n is bit n - 1 of bits. A set of {0} is empty, and
 * &set.bits is a mask that lc_kernel_set_signal_mask and
 * lc_kernel_create_signal_fd take.
 */
typedef struct {
    uint64_t bits;
} lc_signal_set;

/* Get a set that holds no signal. */
lc_signal_set lc_signal_set_empty(void);

/* Get the set of every signal, 1 to LC_SIGNAL_MAX. A mask of it blocks all
 * but SIGKILL and SIGSTOP, which the kernel never blocks. */
lc_signal_set lc_signal_set_full(void);

/* Add signal signo to *set. A number outside 1 to LC_SIGNAL_MAX gives
 * LC_ERR_INVAL and leaves *set as it was. */
[[nodiscard]] lc_result lc_signal_set_add(lc_signal_set *set, int signo);

/* Take signal signo out of *set. A number outside 1 to LC_SIGNAL_MAX
 * gives LC_ERR_INVAL and leaves *set as it was. */
[[nodiscard]] lc_result lc_signal_set_remove(lc_signal_set *set, int signo);

/* Tell whether set holds signal signo: false for a number outside 1 to
 * LC_SIGNAL_MAX. */
bool lc_signal_set_has(lc_signal_set set, int signo);

/* --- Handlers that get what the kernel tells of a signal --- */

/*
 * The record of one signal: the kernel's struct signalfd_siginfo, 128
 * bytes, which <lightc/syscall.h> defines with its fields. A read of a
 * signalfd gives one per signal, and a handler of lc_signal_action gets
 * the same record, filled as signalfd fills it.
 */
typedef struct lc_signal_info lc_signal_info;

/*
 * A handler that lc_signal_action installs. It gets the signal's number,
 * what the kernel tells of the signal, and context, the kernel's
 * ucontext_t of the code the signal interrupted, which the handler may
 * read and change. info points into the handler's stack frame, so it is
 * valid until the handler returns.
 */
typedef void (*lc_signal_action_handler)(int signo, const lc_signal_info *info, void *context);

/* lc_signal_action's flag to restart a system call that the handler
 * interrupted, where the kernel can, instead of ending it with EINTR: the
 * kernel's SA_RESTART. */
#define LC_SIGNAL_RESTART  0x10000000U

/*
 * Install a handler for signal signo that gets what the kernel tells of
 * each signal. lightc installs a handler of its own with SA_SIGINFO, which
 * fills an lc_signal_info from the kernel's siginfo_t as a signalfd read
 * fills it, and calls handler with it. While handler runs, signo and the
 * signals of mask are blocked, besides those already blocked.
 *
 * flags is 0 or LC_SIGNAL_RESTART. Without LC_SIGNAL_RESTART the handler
 * is installed as lc_signal_handle installs one, so a signal that
 * interrupts a blocking system call ends it with EINTR. With it, the
 * kernel restarts the call where it can, as signal(7) lists.
 *
 * A number outside 1 to LC_SIGNAL_MAX, a NULL handler or another flag
 * gives LC_ERR_INVAL, and so does SIGKILL (9) or SIGSTOP (19), which the
 * kernel refuses; the signal's handling is then as it was. Each signal has
 * one handler of lc_signal_action: a call replaces the one before it.
 * lc_signal_handle, lc_signal_ignore and lc_signal_reset replace what this
 * installs, as it replaces what they install: the last call for a signal
 * decides. lightc's handler makes no system call, takes no lock and
 * allocates nothing, so what handler may do is what any signal handler may.
 */
[[nodiscard]] lc_result lc_signal_action(int signo, lc_signal_action_handler handler,
                                         lc_signal_set mask, uint32_t flags);

#endif
