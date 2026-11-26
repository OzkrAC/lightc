/*
 * uring_fault.h — make io_uring_enter fail as the kernel does under memory
 * pressure, for the tests of lightc's async ring.
 *
 * A seccomp filter fails each submit-only io_uring_enter of a given count
 * of SQEs with EAGAIN, the kernel's "could not allocate a request, reap
 * some completions and try again", and lets every other call through: a
 * wait, whose min_complete is 1, gets past. The filter stays for the life of
 * the process, so a test installs it in a child: in_child runs one there
 * under a watchdog. Neither works under qemu-user, which runs no guest's
 * filter and has no io_uring; uring_fault_works says whether they do.
 */

#ifndef LIGHTC_TESTS_URING_FAULT_H
#define LIGHTC_TESTS_URING_FAULT_H

#include <lightc/async.h>
#include <lightc/syscall.h>

/* --- seccomp and BPF, as <linux/seccomp.h> and <linux/filter.h> give them --- */

#define UF_PR_SET_NO_NEW_PRIVS     38
#define UF_SECCOMP_SET_MODE_FILTER 1
#define UF_SECCOMP_RET_ERRNO       0x00050000u
#define UF_SECCOMP_RET_ALLOW       0x7fff0000u
#define UF_BPF_LD_W_ABS            0x20    /* BPF_LD | BPF_W | BPF_ABS */
#define UF_BPF_JEQ_K               0x15    /* BPF_JMP | BPF_JEQ | BPF_K */
#define UF_BPF_RET_K               0x06    /* BPF_RET | BPF_K */
#define UF_ITIMER_REAL             0

#if defined(__x86_64__)
#define UF_AUDIT_ARCH 0xc000003eu
#elif defined(__aarch64__)
#define UF_AUDIT_ARCH 0xc00000b7u
#endif

typedef struct {
    uint16_t code;
    uint8_t  jt;
    uint8_t  jf;
    uint32_t k;
} uf_bpf_insn;

typedef struct {
    uint16_t     length;
    uf_bpf_insn *filter;
} uf_bpf_program;

/* Fails with EAGAIN each io_uring_enter of `to_submit` SQEs that waits for
 * nothing (min_complete 0), as a flush makes. The offsets are those of
 * struct seccomp_data: nr at 0, arch at 4, args[1] at 24 and args[2] at 32,
 * the low half of each on these little-endian arches. True if installed. */
static inline bool refuse_flushes_of(uint32_t to_submit) {
    static uf_bpf_insn filter[10];
    static uf_bpf_program program = { 10, filter };
    uf_bpf_insn f[10] = {
        { UF_BPF_LD_W_ABS, 0, 0, 4 },
        { UF_BPF_JEQ_K, 0, 7, UF_AUDIT_ARCH },
        { UF_BPF_LD_W_ABS, 0, 0, 0 },
        { UF_BPF_JEQ_K, 0, 5, (uint32_t)SYS_io_uring_enter },
        { UF_BPF_LD_W_ABS, 0, 0, 24 },
        { UF_BPF_JEQ_K, 0, 3, to_submit },
        { UF_BPF_LD_W_ABS, 0, 0, 32 },
        { UF_BPF_JEQ_K, 0, 1, 0 },
        { UF_BPF_RET_K, 0, 0, UF_SECCOMP_RET_ERRNO | LC_ERR_AGAIN },
        { UF_BPF_RET_K, 0, 0, UF_SECCOMP_RET_ALLOW },
    };
    for (int i = 0; i < 10; i++) filter[i] = f[i];
    (void)lc_kernel_prctl(UF_PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    return lc_kernel_seccomp(UF_SECCOMP_SET_MODE_FILTER, 0, &program) == 0;
}

/* Runs `scenario` in a child under a watchdog of `seconds`. Returns its
 * exit code, or 128 plus the signal that killed it: 142 is the watchdog's
 * SIGALRM. -1 if the fork failed. */
static inline int32_t in_child(int32_t (*scenario)(void), int64_t seconds) {
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, seconds, 0 };   /* struct itimerval, once */
        (void)lc_syscall3(SYS_setitimer, UF_ITIMER_REAL, (int64_t)watchdog, 0);
        lc_kernel_exit(scenario());
    }
    if (pid < 0) return -1;
    int32_t status = 0;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    return (status & 0x7f) ? 128 + (status & 0x7f) : (status >> 8) & 0xff;
}

/* A child installs a filter that lets every call through, so an emulator
 * that cannot run a guest's filter refuses it. */
static inline int32_t uf_install_allow_all(void) {
    static uf_bpf_insn allow_all[1] = { { UF_BPF_RET_K, 0, 0, UF_SECCOMP_RET_ALLOW } };
    static uf_bpf_program all = { 1, allow_all };
    (void)lc_kernel_prctl(UF_PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    return lc_kernel_seccomp(UF_SECCOMP_SET_MODE_FILTER, 0, &all) == 0 ? 0 : 1;
}

/* Whether io_uring and seccomp filters both work here. */
static inline bool uring_fault_works(void) {
    lc_result_ptr probe = lc_async_ring_create(4);
    if (lc_ptr_is_err(probe)) return false;
    lc_async_ring_destroy(probe.value);
    return in_child(uf_install_allow_all, 3) == 0;
}

#endif /* LIGHTC_TESTS_URING_FAULT_H */
