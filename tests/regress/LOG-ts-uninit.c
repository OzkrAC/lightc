/*
 * LOG-ts-uninit — a JSON log line's ts must be 0 when the clock cannot be
 * read.
 *
 * The JSON line's writer read the time into a timespec on its stack and
 * printed it whatever clock_gettime returned, so when the call failed, as
 * under a seccomp filter that denies it, ts was whatever the stack held.
 * It is now 0, as lc_time_now gives then.
 *
 * In a child, a seccomp filter fails clock_gettime with EPERM, the stack
 * is filled with 0x5a bytes, and a JSON line goes to a pipe: its ts must be
 * 0. Seccomp filters do not work under qemu-user, so there the program
 * skips.
 */

#include "test.h"
#include <lightc/log.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

/* --- seccomp and BPF, as <linux/seccomp.h> and <linux/filter.h> give them --- */

#define PR_SET_NO_NEW_PRIVS     38
#define SECCOMP_SET_MODE_FILTER 1
#define SECCOMP_RET_ERRNO       0x00050000u
#define SECCOMP_RET_ALLOW       0x7fff0000u
#define BPF_LD_W_ABS            0x20
#define BPF_JEQ_K               0x15
#define BPF_RET_K               0x06

#if defined(__x86_64__)
#define AUDIT_ARCH_THIS 0xc000003eu
#elif defined(__aarch64__)
#define AUDIT_ARCH_THIS 0xc00000b7u
#endif

typedef struct { uint16_t code; uint8_t jt, jf; uint32_t k; } bpf_insn;
typedef struct { uint16_t length; bpf_insn *filter; } bpf_program;

/* clock_gettime fails with EPERM; every other call goes through. */
static bool deny_the_clock(void) {
    static bpf_insn f[6] = {
        { BPF_LD_W_ABS, 0, 0, 4 },                     /* arch */
        { BPF_JEQ_K, 0, 3, AUDIT_ARCH_THIS },
        { BPF_LD_W_ABS, 0, 0, 0 },                     /* nr */
        { BPF_JEQ_K, 0, 1, (uint32_t)SYS_clock_gettime },
        { BPF_RET_K, 0, 0, SECCOMP_RET_ERRNO | LC_ERR_PERM },
        { BPF_RET_K, 0, 0, SECCOMP_RET_ALLOW },
    };
    static bpf_program program = { 6, f };
    (void)lc_kernel_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    return lc_kernel_seccomp(SECCOMP_SET_MODE_FILTER, 0, &program) == 0;
}

/* Whether a seccomp filter can be installed: a child tries one that lets
 * every call through, as qemu-user refuses a guest's filter. */
static bool seccomp_works(void) {
    static bpf_insn allow[1] = { { BPF_RET_K, 0, 0, SECCOMP_RET_ALLOW } };
    static bpf_program all = { 1, allow };
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        (void)lc_kernel_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
        lc_kernel_exit(lc_kernel_seccomp(SECCOMP_SET_MODE_FILTER, 0, &all) == 0 ? 0 : 1);
    }
    int32_t status = 1;
    if (pid > 0) (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    return status == 0;
}

[[gnu::noinline]] static void dirty_the_stack(void) {
    volatile uint8_t junk[4096];
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = 0x5a;
}

static int32_t log_without_a_clock(int32_t fd) {
    if (!deny_the_clock()) return 10;
    lc_timespec ts;
    if (lc_kernel_get_clock(LC_CLOCK_REALTIME, &ts) != -LC_ERR_PERM) return 11;
    lc_log_set_output(fd);
    lc_log_set_format(LC_LOG_FORMAT_JSON);
    dirty_the_stack();
    lc_log(LC_LOG_INFO, "no clock");
    return 0;
}

static void test_ts_is_0_without_a_clock(void) {
    if (!seccomp_works()) TEST_SKIP("seccomp filters do not work here");
    int32_t fds[2];
    TEST_REQUIRE(lc_kernel_create_pipe(fds, 0) == 0);
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) lc_kernel_exit(log_without_a_clock(fds[1]));
    TEST_REQUIRE(pid > 0);
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_REQUIRE_EQ(status, 0);

    char line[256];
    lc_sysret got = lc_kernel_read_bytes(fds[0], line, sizeof line - 1);
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
    TEST_REQUIRE(got > 7);
    const char prefix[] = "{\"ts\":";
    TEST_REQUIRE(lc_bytes_compare(line, prefix, sizeof prefix - 1) == 0);
    const char *ts = line + sizeof prefix - 1;
    size_t ts_len = 0;
    while (ts + ts_len < line + got && ts[ts_len] != ',') ts_len++;
    _test_print_text("  ts: ");
    lc_print_string(TEST_FD, ts, ts_len);
    _test_print_text("\n");
    TEST_ASSERT(ts_len == 1 && ts[0] == '0');   /* the defect: the stack's bytes */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_ts_is_0_without_a_clock);
    return test_main();
}
