/*
 * abi_check.c — compile-time check of lightc's kernel ABI against the Linux
 * UAPI headers.
 *
 * Compiling this file is the whole check: it has no main and is never linked.
 * Each _Static_assert compares a value from <lightc/syscall.h> with the one
 * the kernel headers give for the target arch, so compile it once per arch,
 * with that arch's kernel headers installed:
 *
 *   gcc -std=c2x -ffreestanding -Wall -Wextra -Wpedantic -Werror \
 *       -Iinclude -fsyntax-only tests/abi_check.c
 *   aarch64-linux-gnu-gcc (same flags)
 *
 * syscall.h defines many of the names the UAPI headers define (O_*, MAP_*,
 * SIG*, MS_*, ...). Each such UAPI value is captured as an enum constant
 * UAPI_<name>, the UAPI macro is #undef'd, and only then is syscall.h
 * included, so every assert compares lightc's definition with the kernel's.
 * SYS_* numbers are compared with __NR_* directly; those names do not clash.
 *
 * Not checked, because no UAPI header defines them: STDIN, STDOUT, STDERR,
 * MAP_FAILED, F_OK, R_OK, W_OK, X_OK, MNT_DETACH, and the pointer constants
 * SIG_DFL and SIG_IGN (#undef'd below only so syscall.h can define them).
 */

#include <stddef.h>          /* offsetof */

#include <asm/unistd.h>      /* __NR_* */
#include <asm/signal.h>      /* SIG*, SIG_BLOCK..., struct sigaction */
#include <linux/fcntl.h>     /* O_*, AT_* */
#include <linux/fs.h>        /* SEEK_* */
#include <linux/futex.h>     /* FUTEX_* */
#include <linux/mman.h>      /* PROT_*, MAP_*, MADV_* */
#include <linux/mount.h>     /* MS_* */
#include <linux/sched.h>     /* CLONE_NEW* */
#include <linux/signalfd.h>  /* SFD_* */
#include <linux/time.h>      /* CLOCK_* */
#include <linux/wait.h>      /* WNOHANG */

/* --- Capture every UAPI value that syscall.h redefines --- */

#define UAPI_CONSTANTS(X)                                                   \
    X(AT_FDCWD) X(AT_REMOVEDIR) X(AT_EACCESS) X(AT_SYMLINK_NOFOLLOW)        \
    X(O_RDONLY) X(O_WRONLY) X(O_RDWR) X(O_CREAT) X(O_TRUNC) X(O_APPEND)     \
    X(O_NONBLOCK) X(O_CLOEXEC) X(O_DIRECT) X(O_DIRECTORY) X(O_NOFOLLOW)     \
    X(PROT_NONE) X(PROT_READ) X(PROT_WRITE) X(PROT_EXEC)                    \
    X(MAP_SHARED) X(MAP_PRIVATE) X(MAP_FIXED) X(MAP_ANONYMOUS)              \
    X(MADV_DONTNEED)                                                        \
    X(SEEK_SET) X(SEEK_CUR) X(SEEK_END)                                     \
    X(WNOHANG)                                                              \
    X(SIGHUP) X(SIGINT) X(SIGQUIT) X(SIGKILL) X(SIGUSR1) X(SIGUSR2)         \
    X(SIGTERM) X(SIGCHLD)                                                   \
    X(SFD_CLOEXEC) X(SFD_NONBLOCK)                                          \
    X(FUTEX_WAIT) X(FUTEX_WAKE)                                             \
    X(MS_RDONLY) X(MS_NOSUID) X(MS_NODEV) X(MS_NOEXEC) X(MS_REMOUNT)        \
    X(MS_BIND) X(MS_REC) X(MS_PRIVATE) X(MS_SLAVE)                          \
    X(CLONE_NEWNS) X(CLONE_NEWUTS) X(CLONE_NEWIPC) X(CLONE_NEWUSER)         \
    X(CLONE_NEWPID) X(CLONE_NEWNET)

#define CAPTURE(name) UAPI_##name = (name),
enum { UAPI_CONSTANTS(CAPTURE) };
#undef CAPTURE

#undef AT_FDCWD
#undef AT_REMOVEDIR
#undef AT_EACCESS
#undef AT_SYMLINK_NOFOLLOW
#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_CREAT
#undef O_TRUNC
#undef O_APPEND
#undef O_NONBLOCK
#undef O_CLOEXEC
#undef O_DIRECT
#undef O_DIRECTORY
#undef O_NOFOLLOW
#undef PROT_NONE
#undef PROT_READ
#undef PROT_WRITE
#undef PROT_EXEC
#undef MAP_SHARED
#undef MAP_PRIVATE
#undef MAP_FIXED
#undef MAP_ANONYMOUS
#undef MADV_DONTNEED
#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#undef WNOHANG
#undef SIGHUP
#undef SIGINT
#undef SIGQUIT
#undef SIGKILL
#undef SIGUSR1
#undef SIGUSR2
#undef SIGTERM
#undef SIGCHLD
#undef SIG_DFL
#undef SIG_IGN
#undef SFD_CLOEXEC
#undef SFD_NONBLOCK
#undef FUTEX_WAIT
#undef FUTEX_WAKE
#undef MS_RDONLY
#undef MS_NOSUID
#undef MS_NODEV
#undef MS_NOEXEC
#undef MS_REMOUNT
#undef MS_BIND
#undef MS_REC
#undef MS_PRIVATE
#undef MS_SLAVE
#undef CLONE_NEWNS
#undef CLONE_NEWUTS
#undef CLONE_NEWIPC
#undef CLONE_NEWUSER
#undef CLONE_NEWPID
#undef CLONE_NEWNET

#include <lightc/syscall.h>

/* --- Constants --- */

#define CHECK(name) \
    _Static_assert((name) == UAPI_##name, #name " differs from the UAPI value");
UAPI_CONSTANTS(CHECK)

_Static_assert(LC_SIG_BLOCK == SIG_BLOCK, "LC_SIG_BLOCK differs from SIG_BLOCK");
_Static_assert(LC_SIG_UNBLOCK == SIG_UNBLOCK, "LC_SIG_UNBLOCK differs from SIG_UNBLOCK");
_Static_assert(LC_SIG_SETMASK == SIG_SETMASK, "LC_SIG_SETMASK differs from SIG_SETMASK");
_Static_assert(LC_CLOCK_REALTIME == CLOCK_REALTIME, "LC_CLOCK_REALTIME differs from CLOCK_REALTIME");
_Static_assert(LC_CLOCK_MONOTONIC == CLOCK_MONOTONIC, "LC_CLOCK_MONOTONIC differs from CLOCK_MONOTONIC");

/* --- Syscall numbers --- */

#define SYSCALLS(X)                                                         \
    X(read) X(write) X(close) X(lseek) X(mmap) X(mprotect) X(munmap)        \
    X(brk) X(ioctl) X(writev) X(openat) X(getpid) X(futex) X(madvise)       \
    X(exit) X(exit_group) X(clone) X(gettid) X(getdents64)                  \
    X(socket) X(bind) X(listen) X(connect) X(sendto) X(recvfrom)            \
    X(setsockopt) X(shutdown) X(accept4)                                    \
    X(clock_gettime) X(nanosleep)                                           \
    X(execve) X(wait4) X(pipe2) X(dup3) X(setsid)                           \
    X(rt_sigaction) X(rt_sigprocmask) X(kill) X(signalfd4)                  \
    X(mount) X(umount2) X(pivot_root) X(chdir) X(fstat) X(statx)            \
    X(mknodat) X(mkdirat) X(symlinkat) X(unlinkat) X(readlinkat)            \
    X(fchownat) X(fchmodat) X(fchmodat2) X(faccessat2)                      \
    X(unshare) X(setns) X(sethostname) X(prctl) X(seccomp)                  \
    X(getuid) X(setuid) X(setgid) X(setgroups)                              \
    X(io_uring_setup) X(io_uring_enter) X(io_uring_register)

#define CHECK_NR(name) \
    _Static_assert(SYS_##name == __NR_##name, "SYS_" #name " differs from __NR_" #name);
SYSCALLS(CHECK_NR)

#if defined(__x86_64__)
CHECK_NR(open)
CHECK_NR(arch_prctl)
#endif

/* --- struct sigaction: rt_sigaction copies the whole struct in and out --- */

_Static_assert(sizeof(lc_kernel_sigaction) == 32,
               "lc_kernel_sigaction must be 32 bytes");
_Static_assert(offsetof(lc_kernel_sigaction, mask) == 24,
               "lc_kernel_sigaction.mask must be at offset 24");
_Static_assert(sizeof(lc_kernel_sigaction) == sizeof(struct sigaction),
               "lc_kernel_sigaction size differs from struct sigaction");
_Static_assert(offsetof(lc_kernel_sigaction, handler) == offsetof(struct sigaction, sa_handler),
               "lc_kernel_sigaction.handler offset differs from sa_handler");
_Static_assert(offsetof(lc_kernel_sigaction, flags) == offsetof(struct sigaction, sa_flags),
               "lc_kernel_sigaction.flags offset differs from sa_flags");
_Static_assert(offsetof(lc_kernel_sigaction, restorer) == offsetof(struct sigaction, sa_restorer),
               "lc_kernel_sigaction.restorer offset differs from sa_restorer");
_Static_assert(offsetof(lc_kernel_sigaction, mask) == offsetof(struct sigaction, sa_mask),
               "lc_kernel_sigaction.mask offset differs from sa_mask");
