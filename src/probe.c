/*
 * probe.c — lc_probe_kernel: which of the newer kernel features this
 * process can use, each tested once.
 *
 * Each probe makes a feature's system calls with arguments that the kernel
 * refuses before it does anything: a flag bit that no call defines, fd -1,
 * an empty path, an argument struct of size 0. So a probe changes nothing,
 * opens nothing and starts no process, and a kernel that has the call
 * answers with an error such as EINVAL. ENOSYS means the call is absent:
 * the kernel lacks it, or a seccomp filter answers it so. EPERM means a
 * filter refuses it, where the call never answers these arguments with
 * EPERM itself; for io_uring_setup it can also be kernel.io_uring_disabled,
 * which the kernel checks first. Every other answer means the kernel ran
 * its own code for the call.
 */

#include <lightc/probe.h>
#include <lightc/syscall.h>
#include <stdatomic.h>
#include "kernel.h"          /* io_uring_params */

/* The answer of each feature, by its LC_KFEAT_* value: 0 until it is
 * probed, then 1 plus the error lc_probe_kernel returns for it. The value is
 * the whole answer, and every probe of a feature finds the same one, so two
 * threads that probe at once may both make the calls, and both store the
 * same value: relaxed loads and stores are enough. */
static _Atomic int32_t answers[LC_KFEAT_IO_URING + 1];

/* The top bit, which is no flag of the probed calls, nor a command of
 * fsconfig. */
static const uint32_t unknown_bit = 0x80000000u;

/* The argument struct of openat2, clone3 and mount_setattr: zeros, larger
 * than each of theirs. Their probes pass size 0, which each call refuses
 * before it reads the struct, so this is read only by an emulator or a
 * sandbox that copies the struct first. */
static const uint64_t zeros[16] = {0};

/* What `ret` from one probe call says: 0 if the kernel has the call, or
 * the error that says it is absent. EPERM is the kernel's own answer to a
 * caller without CAP_SYS_ADMIN where `eperm_is_the_kernels`. */
static int32_t absent(lc_sysret ret, bool eperm_is_the_kernels) {
    if (ret == -LC_ERR_NOSYS) return LC_ERR_NOSYS;
    if (ret == -LC_ERR_PERM && !eperm_is_the_kernels) return LC_ERR_PERM;
    return 0;
}

/* Each of the mount API's six calls, until one is absent. open_tree,
 * move_mount, fsopen, fsmount and fspick check their flags first, and
 * fsconfig its fd; move_mount, fsopen, fsmount and fspick check the
 * caller's CAP_SYS_ADMIN before that. */
static int32_t probe_mount_api(void) {
    int32_t error = absent(lc_syscall3(SYS_open_tree, -1, (int64_t)"", unknown_bit), false);
    if (error == 0)
        error = absent(lc_syscall5(SYS_move_mount, -1, (int64_t)"", -1, (int64_t)"", unknown_bit), true);
    if (error == 0)
        error = absent(lc_syscall2(SYS_fsopen, (int64_t)"", unknown_bit), true);
    if (error == 0)
        error = absent(lc_syscall5(SYS_fsconfig, -1, unknown_bit, 0, 0, 0), false);
    if (error == 0)
        error = absent(lc_syscall3(SYS_fsmount, -1, unknown_bit, 0), true);
    if (error == 0)
        error = absent(lc_syscall3(SYS_fspick, -1, (int64_t)"", unknown_bit), true);
    return error;
}

/* Probes `feature`, a valid one, and returns its error. */
static int32_t probe(lc_kernel_feature feature) {
    switch (feature) {
    case LC_KFEAT_FCHMODAT2:        /* flags are checked first */
        return absent(lc_syscall4(SYS_fchmodat2, -1, (int64_t)"", 0, unknown_bit), false);
    case LC_KFEAT_FACCESSAT2:       /* the mode, F_OK here, then the flags */
        return absent(lc_syscall4(SYS_faccessat2, -1, (int64_t)"", 0, unknown_bit), false);
    case LC_KFEAT_OPENAT2:          /* a size below struct open_how's first */
        return absent(lc_syscall4(SYS_openat2, -1, (int64_t)"", (int64_t)zeros, 0), false);
    case LC_KFEAT_CLOSE_RANGE:      /* the flags, then first > last; no fd is that high */
        return absent(lc_syscall3(SYS_close_range, 0xffffffff, 0xfffffffe, unknown_bit), false);
    case LC_KFEAT_CLONE3:           /* a size below struct clone_args's first */
        return absent(lc_syscall2(SYS_clone3, (int64_t)zeros, 0), false);
    case LC_KFEAT_PIDFD:            /* pid 0 */
        return absent(lc_syscall2(SYS_pidfd_open, 0, 0), false);
    case LC_KFEAT_MOUNT_API:
        return probe_mount_api();
    case LC_KFEAT_MOUNT_SETATTR:    /* the flags, then the size */
        return absent(lc_syscall5(SYS_mount_setattr, -1, (int64_t)"", unknown_bit, (int64_t)zeros, 0), false);
    case LC_KFEAT_IO_URING: {       /* kernel.io_uring_disabled, then 0 entries */
        io_uring_params params = {0};
        return absent(lc_kernel_io_ring_setup(0, &params), false);
    }
    }
    return LC_ERR_INVAL;
}

lc_result lc_probe_kernel(lc_kernel_feature feature) {
    int32_t f = (int32_t)feature;
    if (f < (int32_t)LC_KFEAT_FCHMODAT2 || f > (int32_t)LC_KFEAT_IO_URING) return lc_err(LC_ERR_INVAL);
    int32_t answer = atomic_load_explicit(&answers[f], memory_order_relaxed);
    if (answer == 0) {
        answer = 1 + probe(feature);
        atomic_store_explicit(&answers[f], answer, memory_order_relaxed);
    }
    return answer == 1 ? lc_ok(0) : lc_err(answer - 1);
}
