#ifndef LIGHTC_SYSCALL_NR_AARCH64_H
#define LIGHTC_SYSCALL_NR_AARCH64_H

/*
 * aarch64 syscall numbers, from include/uapi/asm-generic/unistd.h,
 * in number order, each with the number of arguments it takes.
 * aarch64 has no syscall for SYS_open or SYS_arch_prctl.
 *
 * Generated from syscalls.def by tools/gensyscalls.sh: edit syscalls.def,
 * not this file, then run sh tools/gensyscalls.sh.
 */

#define SYS_dup3              24   /* 3 args */
#define SYS_ioctl             29   /* 3 args */
#define SYS_mknodat           33   /* 4 args */
#define SYS_mkdirat           34   /* 3 args */
#define SYS_unlinkat          35   /* 3 args */
#define SYS_symlinkat         36   /* 3 args */
#define SYS_umount2           39   /* 2 args */
#define SYS_mount             40   /* 5 args */
#define SYS_pivot_root        41   /* 2 args */
#define SYS_chdir             49   /* 1 arg */
#define SYS_fchmodat          53   /* 3 args */
#define SYS_fchownat          54   /* 5 args */
#define SYS_openat            56   /* 4 args */
#define SYS_close             57   /* 1 arg */
#define SYS_pipe2             59   /* 2 args */
#define SYS_getdents64        61   /* 3 args */
#define SYS_lseek             62   /* 3 args */
#define SYS_read              63   /* 3 args */
#define SYS_write             64   /* 3 args */
#define SYS_writev            66   /* 3 args */
#define SYS_signalfd4         74   /* 4 args */
#define SYS_readlinkat        78   /* 4 args */
#define SYS_fstat             80   /* 2 args */
#define SYS_exit              93   /* 1 arg */
#define SYS_exit_group        94   /* 1 arg */
#define SYS_unshare           97   /* 1 arg */
#define SYS_futex             98   /* 6 args */
#define SYS_nanosleep         101  /* 2 args */
#define SYS_setitimer         103  /* 3 args */
#define SYS_clock_gettime     113  /* 2 args */
#define SYS_kill              129  /* 2 args */
#define SYS_rt_sigaction      134  /* 4 args */
#define SYS_rt_sigprocmask    135  /* 4 args */
#define SYS_setgid            144  /* 1 arg */
#define SYS_setuid            146  /* 1 arg */
#define SYS_setsid            157  /* 0 args */
#define SYS_setgroups         159  /* 2 args */
#define SYS_sethostname       161  /* 2 args */
#define SYS_prctl             167  /* 5 args */
#define SYS_getpid            172  /* 0 args */
#define SYS_getuid            174  /* 0 args */
#define SYS_gettid            178  /* 0 args */
#define SYS_socket            198  /* 3 args */
#define SYS_bind              200  /* 3 args */
#define SYS_listen            201  /* 2 args */
#define SYS_connect           203  /* 3 args */
#define SYS_getsockname       204  /* 3 args */
#define SYS_sendto            206  /* 6 args */
#define SYS_recvfrom          207  /* 6 args */
#define SYS_setsockopt        208  /* 5 args */
#define SYS_shutdown          210  /* 2 args */
#define SYS_brk               214  /* 1 arg */
#define SYS_munmap            215  /* 2 args */
#define SYS_clone             220  /* 5 args */
#define SYS_execve            221  /* 3 args */
#define SYS_mmap              222  /* 6 args */
#define SYS_mprotect          226  /* 3 args */
#define SYS_madvise           233  /* 3 args */
#define SYS_accept4           242  /* 4 args */
#define SYS_wait4             260  /* 4 args */
#define SYS_setns             268  /* 2 args */
#define SYS_seccomp           277  /* 3 args */
#define SYS_statx             291  /* 5 args */
#define SYS_io_uring_setup    425  /* 2 args */
#define SYS_io_uring_enter    426  /* 6 args */
#define SYS_io_uring_register 427  /* 4 args */
#define SYS_faccessat2        439  /* 4 args */
#define SYS_fchmodat2         452  /* 4 args */

#endif /* LIGHTC_SYSCALL_NR_AARCH64_H */
