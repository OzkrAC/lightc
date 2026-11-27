#ifndef LIGHTC_SYSCALL_NR_X86_64_H
#define LIGHTC_SYSCALL_NR_X86_64_H

/*
 * x86_64 syscall numbers, from arch/x86/entry/syscalls/syscall_64.tbl,
 * in number order, each with the number of arguments it takes.
 *
 * Generated from syscalls.def by tools/gensyscalls.sh: edit syscalls.def,
 * not this file, then run sh tools/gensyscalls.sh.
 */

#define SYS_read              0    /* 3 args */
#define SYS_write             1    /* 3 args */
#define SYS_open              2    /* 3 args */
#define SYS_close             3    /* 1 arg */
#define SYS_fstat             5    /* 2 args */
#define SYS_lseek             8    /* 3 args */
#define SYS_mmap              9    /* 6 args */
#define SYS_mprotect          10   /* 3 args */
#define SYS_munmap            11   /* 2 args */
#define SYS_brk               12   /* 1 arg */
#define SYS_rt_sigaction      13   /* 4 args */
#define SYS_rt_sigprocmask    14   /* 4 args */
#define SYS_ioctl             16   /* 3 args */
#define SYS_writev            20   /* 3 args */
#define SYS_sched_yield       24   /* 0 args */
#define SYS_madvise           28   /* 3 args */
#define SYS_nanosleep         35   /* 2 args */
#define SYS_setitimer         38   /* 3 args */
#define SYS_getpid            39   /* 0 args */
#define SYS_socket            41   /* 3 args */
#define SYS_connect           42   /* 3 args */
#define SYS_sendto            44   /* 6 args */
#define SYS_recvfrom          45   /* 6 args */
#define SYS_shutdown          48   /* 2 args */
#define SYS_bind              49   /* 3 args */
#define SYS_listen            50   /* 2 args */
#define SYS_getsockname       51   /* 3 args */
#define SYS_setsockopt        54   /* 5 args */
#define SYS_getsockopt        55   /* 5 args */
#define SYS_clone             56   /* 5 args */
#define SYS_execve            59   /* 3 args */
#define SYS_exit              60   /* 1 arg */
#define SYS_wait4             61   /* 4 args */
#define SYS_kill              62   /* 2 args */
#define SYS_fcntl             72   /* 3 args */
#define SYS_chdir             80   /* 1 arg */
#define SYS_getuid            102  /* 0 args */
#define SYS_setuid            105  /* 1 arg */
#define SYS_setgid            106  /* 1 arg */
#define SYS_setsid            112  /* 0 args */
#define SYS_setgroups         116  /* 2 args */
#define SYS_pivot_root        155  /* 2 args */
#define SYS_prctl             157  /* 5 args */
#define SYS_arch_prctl        158  /* 2 args */
#define SYS_mount             165  /* 5 args */
#define SYS_umount2           166  /* 2 args */
#define SYS_sethostname       170  /* 2 args */
#define SYS_gettid            186  /* 0 args */
#define SYS_futex             202  /* 6 args */
#define SYS_getdents64        217  /* 3 args */
#define SYS_clock_gettime     228  /* 2 args */
#define SYS_clock_nanosleep   230  /* 4 args */
#define SYS_exit_group        231  /* 1 arg */
#define SYS_openat            257  /* 4 args */
#define SYS_mkdirat           258  /* 3 args */
#define SYS_mknodat           259  /* 4 args */
#define SYS_fchownat          260  /* 5 args */
#define SYS_unlinkat          263  /* 3 args */
#define SYS_symlinkat         266  /* 3 args */
#define SYS_readlinkat        267  /* 4 args */
#define SYS_fchmodat          268  /* 3 args */
#define SYS_unshare           272  /* 1 arg */
#define SYS_accept4           288  /* 4 args */
#define SYS_signalfd4         289  /* 4 args */
#define SYS_dup3              292  /* 3 args */
#define SYS_pipe2             293  /* 2 args */
#define SYS_prlimit64         302  /* 4 args */
#define SYS_setns             308  /* 2 args */
#define SYS_seccomp           317  /* 3 args */
#define SYS_statx             332  /* 5 args */
#define SYS_io_uring_setup    425  /* 2 args */
#define SYS_io_uring_enter    426  /* 6 args */
#define SYS_io_uring_register 427  /* 4 args */
#define SYS_open_tree         428  /* 3 args */
#define SYS_move_mount        429  /* 5 args */
#define SYS_fsopen            430  /* 2 args */
#define SYS_fsconfig          431  /* 5 args */
#define SYS_fsmount           432  /* 3 args */
#define SYS_fspick            433  /* 3 args */
#define SYS_pidfd_open        434  /* 2 args */
#define SYS_clone3            435  /* 2 args */
#define SYS_close_range       436  /* 3 args */
#define SYS_openat2           437  /* 4 args */
#define SYS_faccessat2        439  /* 4 args */
#define SYS_mount_setattr     442  /* 5 args */
#define SYS_fchmodat2         452  /* 4 args */

#endif /* LIGHTC_SYSCALL_NR_X86_64_H */
