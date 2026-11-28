#ifndef LIGHTC_SYSCALL_H
#define LIGHTC_SYSCALL_H

#include "types.h"

/*
 * Arch-specific syscall numbers
 */
#if defined(__x86_64__)
#include "arch/x86_64/syscall_nr.h"
#elif defined(__aarch64__)
#include "arch/aarch64/syscall_nr.h"
#else
#error "Unsupported architecture"
#endif

/*
 * Generic syscalls, implemented in arch-specific assembly: lc_syscallN
 * makes system call nr with N arguments, up to 6, the Linux maximum.
 *
 * In the faultinject build (./configure faultinject) they are C functions,
 * in src/fault.c, that first check the fault rules of <lightc/fault.h>: a
 * call that an armed rule fails returns -errno without being made.
 */
/* Make system call nr with no argument. */
lc_sysret lc_syscall0(int64_t nr);
/* Make system call nr with 1 argument. */
lc_sysret lc_syscall1(int64_t nr, int64_t a0);
/* Make system call nr with 2 arguments. */
lc_sysret lc_syscall2(int64_t nr, int64_t a0, int64_t a1);
/* Make system call nr with 3 arguments. */
lc_sysret lc_syscall3(int64_t nr, int64_t a0, int64_t a1, int64_t a2);
/* Make system call nr with 4 arguments. */
lc_sysret lc_syscall4(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3);
/* Make system call nr with 5 arguments. */
lc_sysret lc_syscall5(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4);
/* Make system call nr with 6 arguments. */
lc_sysret lc_syscall6(int64_t nr, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5);

/*
 * Constants
 */

/* openat: use current working directory as base */
#define AT_FDCWD  (-100)

/* open flags */
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0100
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000
#define O_CLOEXEC   02000000

/* These three differ per arch: arm64's asm/fcntl.h overrides the generic values */
#if defined(__x86_64__)
#define O_DIRECT    040000
#define O_DIRECTORY 0200000
#define O_NOFOLLOW  0400000
#elif defined(__aarch64__)
#define O_DIRECTORY 040000
#define O_NOFOLLOW  0100000
#define O_DIRECT    0200000
#endif

/* mmap prot */
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4

/* mmap flags */
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED    ((void *)-1)

/* lseek whence */
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2

/* standard file descriptors */
#define STDIN   0
#define STDOUT  1
#define STDERR  2

/* waitpid options */
#define WNOHANG  1

/* signal numbers */
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGKILL   9
#define SIGUSR1  10
#define SIGUSR2  12
#define SIGTERM  15
#define SIGCHLD  17

/* signal dispositions */
#define SIG_DFL  ((void *)0)
#define SIG_IGN  ((void *)1)

/* sigprocmask how */
#define LC_SIG_BLOCK    0
#define LC_SIG_UNBLOCK  1
#define LC_SIG_SETMASK  2

/* signalfd flags */
#define SFD_CLOEXEC   02000000
#define SFD_NONBLOCK  04000

/*
 * Typed kernel wrappers
 *
 * lc_kernel_* = talks directly to the Linux kernel.
 * All inlined — zero overhead over raw syscalls.
 */

/* --- Process --- */

/* End the process, every thread of it, with exit status `code`. */
static inline _Noreturn void lc_kernel_exit(int32_t code) {
    lc_syscall1(SYS_exit_group, code);
    __builtin_unreachable();
}

/* The process id of the caller. */
static inline int32_t lc_kernel_get_process_id(void) {
    return (int32_t)lc_syscall0(SYS_getpid);
}

/* Fork the current process. Returns 0 in child, child pid in parent. */
static inline lc_sysret lc_kernel_fork(void) {
    return lc_syscall5(SYS_clone, SIGCHLD, 0, 0, 0, 0);
}

/*
 * Clone a new process with the given flags and stack.
 *
 * Unlike raw SYS_clone, this is safe to use from C: fn and arg are placed
 * on the child's stack before the syscall, so the child never accesses the
 * parent's stack frame (which is unreachable after clone changes %rsp/sp).
 *
 * The child calls fn(arg) and then _exit_group — it never returns.
 * The parent receives the child's PID (positive) or -errno (negative).
 *
 * The caller must allocate the stack (e.g., via lc_kernel_map_memory)
 * and pass stack_top = base + size (stacks grow downward).
 *
 * In the faultinject build, the SYS_clone rules of <lightc/fault.h> are
 * checked first, as for a clone made through lc_syscall5.
 */
lc_sysret lc_kernel_clone(int64_t flags, void *stack_top,
                           int (*fn)(void *), void *arg);

/* Replace the process image with the program at `path`. Returns only on an error. */
static inline lc_sysret lc_kernel_execute(const char *path, char *const argv[], char *const envp[]) {
    return lc_syscall3(SYS_execve, (int64_t)path, (int64_t)argv, (int64_t)envp);
}

/* Wait for child `pid` to change state, and store its status in `*status`. */
static inline lc_sysret lc_kernel_wait_for_child(int32_t pid, int32_t *status, int32_t options) {
    return lc_syscall4(SYS_wait4, pid, (int64_t)status, options, 0);
}

/* Create a pipe: `fds[0]` reads, `fds[1]` writes. */
static inline lc_sysret lc_kernel_create_pipe(int32_t fds[2], int32_t flags) {
    return lc_syscall2(SYS_pipe2, (int64_t)fds, flags);
}

/* Make `new_fd` a copy of `old_fd`, closing what `new_fd` was before. */
static inline lc_sysret lc_kernel_duplicate_fd(int32_t old_fd, int32_t new_fd, int32_t flags) {
    return lc_syscall3(SYS_dup3, old_fd, new_fd, flags);
}

/* Start a new session with the caller as its leader. */
static inline lc_sysret lc_kernel_create_session(void) {
    return lc_syscall0(SYS_setsid);
}

/* --- File I/O --- */

/* Open `path`, relative to the current directory. Returns the fd. */
static inline lc_sysret lc_kernel_open_file(const char *path, int32_t flags, int32_t mode) {
    return lc_syscall4(SYS_openat, AT_FDCWD, (int64_t)path, flags, mode);
}

/* Open `path`, relative to the directory `dirfd`. Returns the fd. */
static inline lc_sysret lc_kernel_open_file_at(int32_t dirfd, const char *path, int32_t flags, int32_t mode) {
    return lc_syscall4(SYS_openat, dirfd, (int64_t)path, flags, mode);
}

/* Close `fd`. */
static inline lc_sysret lc_kernel_close_file(int32_t fd) {
    return lc_syscall1(SYS_close, fd);
}

/* One read of up to `count` bytes. Returns the count read, 0 at EOF. */
static inline lc_sysret lc_kernel_read_bytes(int32_t fd, void *buf, size_t count) {
    return lc_syscall3(SYS_read, fd, (int64_t)buf, (int64_t)count);
}

/* One write of up to `count` bytes. Returns the count written. */
static inline lc_sysret lc_kernel_write_bytes(int32_t fd, const void *buf, size_t count) {
    return lc_syscall3(SYS_write, fd, (int64_t)buf, (int64_t)count);
}

/*
 * EINTR- and partial-I/O-safe wrappers.
 *
 * The raw read/write syscalls can (a) fail with -EINTR when a signal arrives —
 * a transient condition that must be retried, not treated as EOF or a hard
 * error — and (b) transfer fewer bytes than requested. All buffered I/O, print,
 * and logging should go through these instead of the raw calls so a signal or a
 * short write never silently drops data.
 */

/* Write all `count` bytes: retries EINTR, loops over partial writes. Returns
 * `count` on full success, a negative errno on a hard error before completion,
 * or a short count if the fd stops accepting bytes (e.g. a 0-return). */
static inline lc_sysret lc_kernel_write_all(int32_t fd, const void *buf, size_t count) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t written = 0;
    while (written < count) {
        lc_sysret ret = lc_syscall3(SYS_write, fd, (int64_t)(p + written),
                                    (int64_t)(count - written));
        if (ret < 0) {
            if (ret == -LC_ERR_INTR) continue;  /* interrupted — retry */
            return ret;                          /* hard error */
        }
        if (ret == 0) break;                     /* no progress */
        written += (size_t)ret;
    }
    return (lc_sysret)written;
}

/* Single read that retries only EINTR. A short read is normal and returned as
 * is (0 = EOF); the caller decides whether to loop. Negative = hard error. */
static inline lc_sysret lc_kernel_read_retry(int32_t fd, void *buf, size_t count) {
    for (;;) {
        lc_sysret ret = lc_syscall3(SYS_read, fd, (int64_t)buf, (int64_t)count);
        if (ret == -LC_ERR_INTR) continue;
        return ret;
    }
}

/* Read up to `count` bytes, retrying EINTR and looping over short reads until
 * the buffer is full or EOF. Returns the byte count (< count means EOF) or a
 * negative errno on a hard error. */
static inline lc_sysret lc_kernel_read_all(int32_t fd, void *buf, size_t count) {
    uint8_t *p = (uint8_t *)buf;
    size_t total = 0;
    while (total < count) {
        lc_sysret ret = lc_syscall3(SYS_read, fd, (int64_t)(p + total),
                                    (int64_t)(count - total));
        if (ret < 0) {
            if (ret == -LC_ERR_INTR) continue;
            return ret;         /* hard error */
        }
        if (ret == 0) break;    /* EOF */
        total += (size_t)ret;
    }
    return (lc_sysret)total;
}

/* Move the file offset of `fd`. Returns the new offset. */
static inline lc_sysret lc_kernel_seek_position(int32_t fd, int64_t offset, int32_t whence) {
    return lc_syscall3(SYS_lseek, fd, offset, whence);
}

/* Issue the ioctl `request` on `fd`. */
static inline lc_sysret lc_kernel_device_control(int32_t fd, uint64_t request, void *arg) {
    return lc_syscall3(SYS_ioctl, fd, (int64_t)request, (int64_t)arg);
}

/* --- Directories --- */

/* Read directory entries of `fd` into `buf`, as getdents64 records. */
static inline lc_sysret lc_kernel_read_directory(int32_t fd, void *buf, size_t count) {
    return lc_syscall3(SYS_getdents64, fd, (int64_t)buf, (int64_t)count);
}

/* --- Sockets --- */

/*
 * Copy the local address of socket fd into addr, which has room for
 * *addr_length bytes, and set *addr_length to the address's full size. An
 * address larger than that room is cut short. After a bind to port 0 this
 * gives the port the kernel picked: pass an lc_socket_address from
 * <lightc/socket.h>, with *addr_length = sizeof(lc_socket_address).
 */
static inline lc_sysret lc_kernel_getsockname(int32_t fd, void *addr, uint32_t *addr_length) {
    return lc_syscall3(SYS_getsockname, fd, (int64_t)addr, (int64_t)addr_length);
}

/* --- Memory --- */

/* Map memory, as mmap does. Returns MAP_FAILED on any error. */
static inline void *lc_kernel_map_memory(void *addr, size_t len, int32_t prot, int32_t flags,
                                         int32_t fd, int64_t offset) {
    /* The raw syscall returns -errno (e.g. -ENOMEM) on failure, not MAP_FAILED.
     * Any value in [-4095, -1] is an error code; normalize it to MAP_FAILED so
     * callers can rely on a single `== MAP_FAILED` check. */
    uintptr_t ret = (uintptr_t)lc_syscall6(SYS_mmap, (int64_t)addr, (int64_t)len,
                                           prot, flags, fd, offset);
    if (ret >= (uintptr_t)-4095) return MAP_FAILED;
    return (void *)ret;
}

/* Unmap the `len` bytes at `addr`. */
static inline lc_sysret lc_kernel_unmap_memory(void *addr, size_t len) {
    return lc_syscall2(SYS_munmap, (int64_t)addr, (int64_t)len);
}

/* Change the protection of the `len` bytes at `addr`. */
static inline lc_sysret lc_kernel_protect_memory(void *addr, size_t len, int32_t prot) {
    return lc_syscall3(SYS_mprotect, (int64_t)addr, (int64_t)len, prot);
}

/* Move the end of the data segment, as brk does. */
static inline lc_sysret lc_kernel_set_heap_end(void *addr) {
    return lc_syscall1(SYS_brk, (int64_t)addr);
}

/* madvise advice flags */
#define MADV_DONTNEED  4

/* Advise the kernel about the `len` bytes at `addr`, as madvise does. */
static inline lc_sysret lc_kernel_advise_memory(void *addr, size_t len, int32_t advice) {
    return lc_syscall3(SYS_madvise, (int64_t)addr, (int64_t)len, advice);
}

/* --- Threading --- */

/* The thread id of the caller. */
static inline int32_t lc_kernel_get_thread_id(void) {
    return (int32_t)lc_syscall0(SYS_gettid);
}

/* futex operations */
#define FUTEX_WAIT  0
#define FUTEX_WAKE  1
/* Or'd into an operation: the futex is private to this process. A private
 * wait is woken only by a private wake of the same process. */
#define FUTEX_PRIVATE_FLAG 128

/*
 * Wait on the shared futex at `addr` while it holds `expected`.
 *
 * futex takes six arguments: uaddr, op, val, timeout, uaddr2 and val3.
 * FUTEX_WAIT reads only the first four, FUTEX_WAKE the first three, and
 * the rest are passed as 0. A NULL timeout waits with no time limit.
 * These two are shared futexes, which also work between processes that
 * map the same memory; lightc's sync objects use private ones.
 */
static inline lc_sysret lc_kernel_futex_wait(int32_t *addr, int32_t expected) {
    return lc_syscall6(SYS_futex, (int64_t)addr, FUTEX_WAIT, expected, 0, 0, 0);
}

/* Wake up to `count` waiters of the shared futex at `addr`. */
static inline lc_sysret lc_kernel_futex_wake(int32_t *addr, int32_t count) {
    return lc_syscall6(SYS_futex, (int64_t)addr, FUTEX_WAKE, count, 0, 0, 0);
}

/* --- Time --- */

#define LC_CLOCK_REALTIME  0
#define LC_CLOCK_MONOTONIC 1

/* lc_kernel_clock_sleep's flag: the time is a deadline on the clock */
#define LC_TIMER_ABSTIME   1

typedef struct {
    int64_t seconds;
    int64_t nanoseconds;
} lc_timespec;

/* Read the clock `clock_id` into `*ts`. */
static inline lc_sysret lc_kernel_get_clock(int32_t clock_id, lc_timespec *ts) {
    return lc_syscall2(SYS_clock_gettime, clock_id, (int64_t)ts);
}

/* Sleep for `*duration`. An interrupting signal ends it early with -EINTR. */
static inline lc_sysret lc_kernel_sleep(const lc_timespec *duration) {
    return lc_syscall2(SYS_nanosleep, (int64_t)duration, 0);
}

/* Sleep on the clock `clock_id` until `*request`, a deadline on it when
 * flags has LC_TIMER_ABSTIME, or else for `*request` as a duration. An
 * interrupting signal ends it early with -EINTR; a duration's sleep then
 * writes what was left to `*remaining`, unless it is NULL. A deadline is the
 * same after an interruption, so sleeping again to it loses no time. */
static inline lc_sysret lc_kernel_clock_sleep(int32_t clock_id, int32_t flags,
                                              const lc_timespec *request, lc_timespec *remaining) {
    return lc_syscall4(SYS_clock_nanosleep, clock_id, flags, (int64_t)request, (int64_t)remaining);
}

/* --- Signals --- */

/*
 * Kernel sigaction struct, as rt_sigaction reads and writes it.
 * Same layout on x86_64 and aarch64: 32 bytes, mask at offset 24. arm64
 * defines SA_RESTORER too (for AArch32 tasks), so its struct also has the
 * restorer slot. Only x86_64 uses it; on aarch64 restorer stays NULL and
 * SA_RESTORER is never set.
 */
typedef struct {
    void    *handler;
    uint64_t flags;
    void    *restorer;
    uint64_t mask;
} lc_kernel_sigaction;

/*
 * signalfd_siginfo — 128 bytes: the record a read of a signalfd gives for
 * each signal, and the one a handler of lc_signal_action (<lightc/signal.h>)
 * gets. The kernel's signalfd_copyinfo (fs/signalfd.c) fills signal, error
 * and code, then the fields of the signal's kind, and zeroes the rest: pid
 * and uid for a kill; pid, uid, int_val and ptr_val for a queued signal;
 * tid, overrun, int_val and ptr_val for a POSIX timer's; band and fd for
 * SIGIO; pid, uid, status, utime and stime for SIGCHLD; addr for a fault,
 * with addr_lsb for a memory error's SIGBUS; and syscall, call_addr and
 * arch for a seccomp filter's SIGSYS. trapno is only filled on arches
 * other than x86_64 and aarch64.
 */
typedef struct lc_signal_info {
    uint32_t signal;
    int32_t  error;
    int32_t  code;
    uint32_t pid;
    uint32_t uid;
    int32_t  fd;
    uint32_t tid;
    uint32_t band;
    uint32_t overrun;
    uint32_t trapno;
    int32_t  status;
    int32_t  int_val;
    uint64_t ptr_val;
    uint64_t utime;
    uint64_t stime;
    uint64_t addr;
    uint16_t addr_lsb;
    uint16_t _pad2;
    int32_t  syscall;
    uint64_t call_addr;
    uint32_t arch;
    uint8_t  _pad[28];
} lc_signal_info;

_Static_assert(sizeof(lc_signal_info) == 128, "lc_signal_info must be 128 bytes");

/* Reset a signal to SIG_DFL. Safe to call in child after fork (no restorer needed). */
static inline lc_sysret lc_kernel_reset_signal(int32_t signum) {
    lc_kernel_sigaction sa;
    sa.handler = SIG_DFL;
    sa.flags = 0;
    sa.restorer = NULL;
    sa.mask = 0;
    return lc_syscall4(SYS_rt_sigaction, signum, (int64_t)&sa, 0, 8);
}

/* Set signal mask. how: LC_SIG_BLOCK/UNBLOCK/SETMASK. mask is a 64-bit bitmask. */
static inline lc_sysret lc_kernel_set_signal_mask(int32_t how, const uint64_t *new_mask,
                                                    uint64_t *old_mask) {
    return lc_syscall4(SYS_rt_sigprocmask, how, (int64_t)new_mask, (int64_t)old_mask, 8);
}

/* Create or update a signalfd. fd=-1 to create new. mask is a 64-bit bitmask. */
static inline lc_sysret lc_kernel_create_signal_fd(int32_t fd, const uint64_t *mask,
                                                     int32_t flags) {
    return lc_syscall4(SYS_signalfd4, fd, (int64_t)mask, 8, flags);
}

/* Send a signal to a process. */
static inline lc_sysret lc_kernel_send_signal(int32_t pid, int32_t signal) {
    return lc_syscall2(SYS_kill, pid, signal);
}

/* --- Filesystem --- */

/* mount flags */
#define MS_RDONLY       1
#define MS_NOSUID       2
#define MS_NODEV        4
#define MS_NOEXEC       8
#define MS_REMOUNT      32
#define MS_BIND         4096
#define MS_REC          16384
#define MS_PRIVATE      (1 << 18)
#define MS_SLAVE        (1 << 19)

/* umount flags */
#define MNT_DETACH  2

/* unlinkat flags */
#define AT_REMOVEDIR  0x200

/* fchownat / fchmodat flags */
#define AT_SYMLINK_NOFOLLOW  0x100

/* faccessat2 flags */
#define AT_EACCESS  0x200

/* access modes */
#define F_OK  0
#define R_OK  4
#define W_OK  2
#define X_OK  1

/*
 * Mount `source` on `target`, as mount(2) does.
 *
 * It needs CAP_SYS_ADMIN in the user namespace that owns the caller's mount
 * namespace. So the first process of new user and mount namespaces, which
 * has every capability in them, can mount a tmpfs, make bind mounts and
 * change propagation, MS_PRIVATE among it, with no privilege outside. The
 * mounts that a mount namespace made in a new user namespace copied from
 * its parent are locked: see lc_kernel_umount.
 */
static inline lc_sysret lc_kernel_mount(const char *source, const char *target,
                                         const char *fstype, uint64_t flags, const void *data) {
    return lc_syscall5(SYS_mount, (int64_t)source, (int64_t)target,
                       (int64_t)fstype, (int64_t)flags, (int64_t)data);
}

/*
 * Unmount `target`.
 *
 * Of umount2's flags this header names MNT_DETACH. A busy mount gives
 * -EBUSY, but MNT_DETACH takes it from the tree at once, and the kernel
 * frees it when its last user is done. A mount that a mount namespace made
 * in a new user namespace copied from its parent is locked to the mount
 * above it and gives -EINVAL, so that the new user namespace cannot uncover
 * what the mount hides.
 */
static inline lc_sysret lc_kernel_umount(const char *target, int32_t flags) {
    return lc_syscall2(SYS_umount2, (int64_t)target, flags);
}

/*
 * Make `new_root` the root mount, and move the old one to `put_old`.
 *
 * new_root must be a mount point, and put_old a directory at or below it;
 * pivot_root(2) gives the rest of the rules. A container's setup then
 * changes directory to "/" and detaches the old root with
 * lc_kernel_umount(put_old, MNT_DETACH).
 */
static inline lc_sysret lc_kernel_pivot_root(const char *new_root, const char *put_old) {
    return lc_syscall2(SYS_pivot_root, (int64_t)new_root, (int64_t)put_old);
}

/* Change the current directory to `path`. */
static inline lc_sysret lc_kernel_chdir(const char *path) {
    return lc_syscall1(SYS_chdir, (int64_t)path);
}

/* Create the directory `path`, relative to `dirfd`. */
static inline lc_sysret lc_kernel_mkdirat(int32_t dirfd, const char *path, int32_t mode) {
    return lc_syscall3(SYS_mkdirat, dirfd, (int64_t)path, mode);
}

/*
 * Create the file node `path`, relative to `dirfd`: a FIFO, a device or the like.
 *
 * mode holds the file type, such as S_IFIFO (0010000) or S_IFCHR (0020000),
 * and the permission bits; dev is the device number, as the kernel's
 * new_encode_dev packs it: (minor & 0xff) | major << 8 | (minor & ~0xff) << 12.
 * A character or block device needs CAP_MKNOD in the initial user namespace,
 * so a process in a user namespace of its own gets -EPERM for one, except
 * for a whiteout, the character device 0:0, which overlayfs takes for a
 * deleted file. A FIFO needs no privilege.
 */
static inline lc_sysret lc_kernel_mknodat(int32_t dirfd, const char *path, int32_t mode, uint64_t dev) {
    return lc_syscall4(SYS_mknodat, dirfd, (int64_t)path, mode, (int64_t)dev);
}

/* Create the symlink `linkpath`, relative to `dirfd`, pointing at `target`. */
static inline lc_sysret lc_kernel_symlinkat(const char *target, int32_t dirfd, const char *linkpath) {
    return lc_syscall3(SYS_symlinkat, (int64_t)target, dirfd, (int64_t)linkpath);
}

/* Remove `path`, relative to `dirfd`: a directory with AT_REMOVEDIR. */
static inline lc_sysret lc_kernel_unlinkat(int32_t dirfd, const char *path, int32_t flags) {
    return lc_syscall3(SYS_unlinkat, dirfd, (int64_t)path, flags);
}

/* Read the target of the symlink `path` into `buf`. Returns its length, with no NUL. */
static inline lc_sysret lc_kernel_readlinkat(int32_t dirfd, const char *path, char *buf, size_t bufsiz) {
    return lc_syscall4(SYS_readlinkat, dirfd, (int64_t)path, (int64_t)buf, (int64_t)bufsiz);
}

/* Change the owner of `path`, relative to `dirfd`. */
static inline lc_sysret lc_kernel_fchownat(int32_t dirfd, const char *path,
                                            uint32_t uid, uint32_t gid, int32_t flags) {
    return lc_syscall5(SYS_fchownat, dirfd, (int64_t)path, uid, gid, flags);
}

/*
 * Change the mode of path, relative to dirfd. With flags 0 this is fchmodat,
 * which follows a symlink to its target. The kernel's fchmodat has no flags
 * argument, so non-zero flags such as AT_SYMLINK_NOFOLLOW go to fchmodat2,
 * which needs Linux 6.6. Older kernels return -ENOSYS for it, which this
 * returns as -EOPNOTSUPP. The call never falls back to fchmodat: that would
 * drop the flags and change the mode of the symlink's target. Linux has no
 * symlink modes, so AT_SYMLINK_NOFOLLOW on a symlink gives -EOPNOTSUPP on
 * every kernel.
 *
 * To refuse symlinks before 6.6, do what glibc does: open path with
 * O_PATH | O_NOFOLLOW | O_CLOEXEC, fstat the fd and stop with -EOPNOTSUPP if
 * it is a symlink, otherwise chmod "/proc/self/fd/<fd>" with flags 0 (needs
 * /proc mounted). This header defines neither O_PATH (010000000) nor an fstat
 * wrapper.
 */
static inline lc_sysret lc_kernel_fchmodat(int32_t dirfd, const char *path, int32_t mode, int32_t flags) {
    if (flags == 0) return lc_syscall3(SYS_fchmodat, dirfd, (int64_t)path, mode);
    lc_sysret ret = lc_syscall4(SYS_fchmodat2, dirfd, (int64_t)path, mode, flags);
    return ret == -LC_ERR_NOSYS ? -LC_ERR_OPNOTSUPP : ret;
}

/* Check whether `path`, relative to `dirfd`, allows `mode`. */
static inline lc_sysret lc_kernel_faccessat2(int32_t dirfd, const char *path, int32_t mode, int32_t flags) {
    return lc_syscall4(SYS_faccessat2, dirfd, (int64_t)path, mode, flags);
}

/* --- Namespaces / Containers --- */

/* clone flags for namespaces */
#define CLONE_NEWNS     0x00020000
#define CLONE_NEWUTS    0x04000000
#define CLONE_NEWIPC    0x08000000
#define CLONE_NEWUSER   0x10000000
#define CLONE_NEWPID    0x20000000
#define CLONE_NEWNET    0x40000000

/* Move the caller into new namespaces, as `flags` names. */
static inline lc_sysret lc_kernel_unshare(int32_t flags) {
    return lc_syscall1(SYS_unshare, flags);
}

/* Join the namespace that `fd` refers to. */
static inline lc_sysret lc_kernel_setns(int32_t fd, int32_t nstype) {
    return lc_syscall2(SYS_setns, fd, nstype);
}

/*
 * Set the host name of the UTS namespace.
 *
 * It takes `len` bytes of `name`, which need no NUL, and at most 64: a
 * longer name gives -EINVAL. It needs CAP_SYS_ADMIN in the user namespace
 * that owns the UTS namespace, so a process in new user and UTS namespaces
 * can name its own.
 */
static inline lc_sysret lc_kernel_sethostname(const char *name, size_t len) {
    return lc_syscall2(SYS_sethostname, (int64_t)name, (int64_t)len);
}

/* Operate on the process, as prctl(2) does with `option`. */
static inline lc_sysret lc_kernel_prctl(int32_t option, uint64_t arg2, uint64_t arg3,
                                         uint64_t arg4, uint64_t arg5) {
    return lc_syscall5(SYS_prctl, option, (int64_t)arg2, (int64_t)arg3,
                       (int64_t)arg4, (int64_t)arg5);
}

/* Operate on the process's seccomp state, as seccomp(2) does with `op`. */
static inline lc_sysret lc_kernel_seccomp(uint32_t op, uint32_t flags, void *args) {
    return lc_syscall3(SYS_seccomp, op, flags, (int64_t)args);
}

/* --- Credentials --- */

/* The real user id of the caller. */
static inline uint32_t lc_kernel_getuid(void) {
    return (uint32_t)lc_syscall0(SYS_getuid);
}

/* Set the user id of the caller. */
static inline lc_sysret lc_kernel_setuid(uint32_t uid) {
    return lc_syscall1(SYS_setuid, uid);
}

/* Set the group id of the caller. */
static inline lc_sysret lc_kernel_setgid(uint32_t gid) {
    return lc_syscall1(SYS_setgid, gid);
}

/* Set the supplementary groups of the caller. */
static inline lc_sysret lc_kernel_setgroups(size_t size, const uint32_t *list) {
    return lc_syscall2(SYS_setgroups, (int64_t)size, (int64_t)list);
}

/*
 * Set the real, effective and saved group ids of the caller. An id of
 * (uint32_t)-1 leaves that one as it is.
 *
 * Each id must be mapped in the caller's user namespace, or the call fails
 * with -EINVAL. Without CAP_SETGID, each must be one of the caller's three
 * group ids, or it fails with -EPERM. To drop privileges, call
 * lc_kernel_setgroups, then this, then lc_kernel_setresuid: the first two
 * need CAP_SETGID, which the process loses when lc_kernel_setresuid leaves
 * none of its user ids 0.
 */
[[nodiscard]] static inline lc_sysret lc_kernel_setresgid(uint32_t rgid, uint32_t egid, uint32_t sgid) {
    return lc_syscall3(SYS_setresgid, rgid, egid, sgid);
}

/*
 * Set the real, effective and saved user ids of the caller. An id of
 * (uint32_t)-1 leaves that one as it is.
 *
 * Each id must be mapped in the caller's user namespace, or the call fails
 * with -EINVAL. Without CAP_SETUID, each must be one of the caller's three
 * user ids, or it fails with -EPERM. A call that leaves none of the three
 * 0, where one was, clears the process's effective and permitted
 * capabilities, or only the effective ones after PR_SET_KEEPCAPS, so in a
 * privilege drop it comes last, after lc_kernel_setgroups and
 * lc_kernel_setresgid.
 */
[[nodiscard]] static inline lc_sysret lc_kernel_setresuid(uint32_t ruid, uint32_t euid, uint32_t suid) {
    return lc_syscall3(SYS_setresuid, ruid, euid, suid);
}

/* --- Async I/O --- */

/* Create an io_uring of `entries` entries. Returns its fd. */
static inline lc_sysret lc_kernel_io_ring_setup(uint32_t entries, void *params) {
    return lc_syscall2(SYS_io_uring_setup, entries, (int64_t)params);
}

/* Submit `to_submit` entries to the io_uring `fd`, and wait for `min_complete` completions. */
static inline lc_sysret lc_kernel_io_ring_enter(int32_t fd, uint32_t to_submit,
                                                 uint32_t min_complete, uint32_t flags) {
    return lc_syscall6(SYS_io_uring_enter, fd, to_submit, min_complete, flags,
                       0 /* sig */, 0 /* sigsz */);
}

#endif /* LIGHTC_SYSCALL_H */
