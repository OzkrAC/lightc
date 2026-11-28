#ifndef LIGHTC_PROBE_H
#define LIGHTC_PROBE_H

#include "types.h"

/*
 * Kernel feature probes: whether this process can use a system call that
 * is newer than the kernel lightc needs, or that a sandbox may take away.
 *
 * lightc needs Linux 5.10, and two of the features below are newer:
 * fchmodat2 (6.6) and mount_setattr (5.12). The others are in every kernel
 * lightc runs on, but a seccomp filter can still answer their calls with
 * ENOSYS, as container runtimes do for clone3, and the sysctl
 * kernel.io_uring_disabled (6.6) can forbid io_uring. docs/contracts.html
 * gives, for each feature, what lightc does without it.
 */

/* A feature that lc_probe_kernel tests, with the Linux release that added
 * it. 0 is none. */
typedef enum {
    LC_KFEAT_FCHMODAT2 = 1, /* fchmodat2, 6.6: lc_kernel_fchmodat with flags */
    LC_KFEAT_FACCESSAT2,    /* faccessat2, 5.8: lc_kernel_faccessat2 */
    LC_KFEAT_OPENAT2,       /* openat2, 5.6 */
    LC_KFEAT_CLOSE_RANGE,   /* close_range, 5.9 */
    LC_KFEAT_CLONE3,        /* clone3, 5.3 */
    LC_KFEAT_PIDFD,         /* pidfd_open, 5.3 */
    LC_KFEAT_MOUNT_API,     /* open_tree, move_mount, fsopen, fsconfig, fsmount
                               and fspick, 5.2; not mount_setattr */
    LC_KFEAT_MOUNT_SETATTR, /* mount_setattr, 5.12 */
    LC_KFEAT_IO_URING       /* io_uring_setup, 5.1, and that this process may
                               create rings: lc_async_ring_create, lightio */
} lc_kernel_feature;

/*
 * Test whether this process can use the kernel feature `feature`. Error 0
 * means it can. Otherwise the error says why not:
 *
 *   LC_ERR_NOSYS  a call of the feature answers ENOSYS: the kernel lacks
 *                 it, or a seccomp filter answers it so.
 *   LC_ERR_PERM   a call answers EPERM, which the kernel itself never
 *                 answers to the probe's arguments, so a seccomp filter
 *                 refuses it; or, for LC_KFEAT_IO_URING, the sysctl
 *                 kernel.io_uring_disabled forbids this process io_uring.
 *   LC_ERR_INVAL  `feature` is not one of LC_KFEAT_*.
 *
 * The first call for a feature makes each of its system calls once, with
 * arguments the kernel refuses before it does anything, such as a flag no
 * call defines, and keeps the answer: later calls make no system call.
 * Threads may call it at once, and each gets the same answer; threads that
 * make the first call at the same moment may each make the system calls.
 * Any other error a call answers, such as EINVAL, means the kernel has the
 * call. fsopen, fsmount, fspick and move_mount refuse a caller without
 * CAP_SYS_ADMIN with EPERM before they look at their arguments, so EPERM
 * from them is the kernel's: LC_KFEAT_MOUNT_API is absent with LC_ERR_PERM
 * only when open_tree or fsconfig answers EPERM.
 *
 * The answers belong to the process: a child of fork inherits them, and a
 * seccomp filter installed after a feature's first probe does not change
 * that feature's answer. A program that execs starts with none, so the
 * program a launcher execs under a filter probes what the filter leaves it.
 * A feature that is present can still fail for want of a resource, as a
 * ring does without memory.
 */
[[nodiscard]] lc_result lc_probe_kernel(lc_kernel_feature feature);

#endif /* LIGHTC_PROBE_H */
