/*
 * test_syscall.c — tests for lightc's typed kernel wrappers (syscall.h).
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/signal.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

/* ===== lc_kernel_mount =====
 *
 * A forked child unshares a user and a mount namespace, which needs no
 * privileges and keeps the mount away from the host, then mounts tmpfs over
 * a directory that holds a marker file. The mount must return 0 and hide the
 * marker. With a wrong SYS_mount (x86_64 once used 40, which is sendfile) the
 * call returns -EBADF or -EFAULT instead.
 *
 * Where unprivileged user namespaces are unavailable (some containers and
 * sandboxes, qemu-user), unshare fails: the child prints SKIP: and the test
 * passes without checking the mount. Ubuntu 23.10+ with AppArmor's
 * kernel.apparmor_restrict_unprivileged_userns=1 (its default) lets unshare
 * succeed but gives the namespace no capabilities, so the mount fails with
 * -EPERM: the child skips then too. -EPERM with that sysctl off or absent is
 * a failure, so CI (which sets it to 0) cannot quietly skip.
 */
#define MOUNT_DIR     "build/test_syscall_mnt"
#define MOUNT_MARKER  MOUNT_DIR "/marker"
#define CHILD_SKIPPED 77
#define APPARMOR_USERNS_SYSCTL "/proc/sys/kernel/apparmor_restrict_unprivileged_userns"

typedef struct {
    lc_sysret mount;    /* lc_kernel_mount of tmpfs on MOUNT_DIR */
    lc_sysret marker;   /* opening MOUNT_MARKER after the mount */
} mount_results;

/* True when APPARMOR_USERNS_SYSCTL exists and reads 1. */
static bool apparmor_restricts_userns(void) {
    lc_sysret fd = lc_kernel_open_file(APPARMOR_USERNS_SYSCTL, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char value = 0;
    lc_sysret n = lc_kernel_read_bytes((int32_t)fd, &value, 1);
    lc_kernel_close_file((int32_t)fd);
    return n == 1 && value == '1';
}

static void test_mount_tmpfs(void) {
    lc_sysret made = lc_kernel_mkdirat(AT_FDCWD, MOUNT_DIR, 0755);
    TEST_ASSERT(made == 0 || made == -LC_ERR_EXIST);
    TEST_ASSERT_OK(lc_file_write_all(MOUNT_MARKER, "x", 1));

    /* The child reports its results through a shared page. */
    void *page = lc_kernel_map_memory(NULL, 4096, PROT_READ | PROT_WRITE,
                                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(page != MAP_FAILED);
    if (page == MAP_FAILED) return;
    volatile mount_results *res = (volatile mount_results *)page;
    res->mount  = 1;
    res->marker = 1;

    lc_sysret pid = lc_kernel_fork();
    TEST_ASSERT(pid >= 0);
    if (pid == 0) {
        lc_sysret ret = lc_kernel_unshare(CLONE_NEWUSER | CLONE_NEWNS);
        if (ret < 0) {
            _test_print_text("SKIP: user namespaces not available, unshare(CLONE_NEWUSER | CLONE_NEWNS) = ");
            lc_print_signed(TEST_FD, ret);
            lc_print_newline(TEST_FD);
            lc_kernel_exit(CHILD_SKIPPED);
        }
        res->mount = lc_kernel_mount("tmpfs", MOUNT_DIR, "tmpfs", 0, NULL);
        if (res->mount == -LC_ERR_PERM && apparmor_restricts_userns()) {
            _test_print_text("SKIP: mount denied in the user namespace (-1), "
                             "kernel.apparmor_restrict_unprivileged_userns=1\n");
            lc_kernel_exit(CHILD_SKIPPED);
        }
        lc_sysret fd = lc_kernel_open_file(MOUNT_MARKER, O_RDONLY | O_CLOEXEC, 0);
        if (fd >= 0) lc_kernel_close_file((int32_t)fd);
        res->marker = fd;
        lc_kernel_exit(0);
    }

    int32_t status = 0;
    lc_kernel_wait_for_child((int32_t)pid, &status, 0);

    if (status != (CHILD_SKIPPED << 8)) {
        _test_print_text("  lc_kernel_mount(\"tmpfs\", " MOUNT_DIR ", \"tmpfs\") = ");
        lc_print_signed(TEST_FD, res->mount);
        lc_print_newline(TEST_FD);

        TEST_ASSERT_EQ(status, 0);                     /* child exited normally */
        TEST_ASSERT_EQ(res->mount, 0);
        TEST_ASSERT_EQ(res->marker, -LC_ERR_NOENT);    /* tmpfs hides the marker */
    }

    /* The mount stayed in the child's namespace: the marker is still here. */
    lc_sysret fd = lc_kernel_open_file(MOUNT_MARKER, O_RDONLY | O_CLOEXEC, 0);
    TEST_ASSERT(fd >= 0);
    if (fd >= 0) lc_kernel_close_file((int32_t)fd);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, MOUNT_MARKER, 0), 0);
    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, MOUNT_DIR, AT_REMOVEDIR), 0);
    lc_kernel_unmap_memory(page, 4096);
}

/* ===== lc_kernel_fchmodat =====
 *
 * The kernel's fchmodat takes no flags. The wrapper used to pass flags to it
 * anyway, so AT_SYMLINK_NOFOLLOW was dropped and the symlink's target got the
 * new mode. Now non-zero flags go to fchmodat2. Linux has no symlink modes, so
 * on 6.6+ fchmodat2 returns -EOPNOTSUPP for a symlink with AT_SYMLINK_NOFOLLOW,
 * and on older kernels the wrapper maps fchmodat2's -ENOSYS to -EOPNOTSUPP:
 * -95 on every kernel, target untouched. With flags 0 the call follows the
 * link and changes the target's mode.
 *
 * The target is created 0644 (less umask), so CHMOD_MODE, which has an execute
 * bit, always differs from its starting mode.
 */
#define CHMOD_TARGET "build/test_syscall_chmod_target"
#define CHMOD_LINK   "build/test_syscall_chmod_link"
#define CHMOD_MODE   0700

/* The head of struct statx, up to stx_mode; the kernel writes all 256 bytes. */
typedef struct {
    uint32_t mask;
    uint32_t blksize;
    uint64_t attributes;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint16_t mode;
    uint8_t  _pad[226];
} test_statx;
_Static_assert(sizeof(test_statx) == 256, "test_statx must be 256 bytes (struct statx)");

#define STATX_MODE 0x2

/* Permission bits of path (following symlinks), or a negative errno. */
static lc_sysret file_mode(const char *path) {
    test_statx stx = {0};
    lc_sysret ret = lc_syscall5(SYS_statx, AT_FDCWD, (int64_t)path, 0, STATX_MODE, (int64_t)&stx);
    if (ret < 0) return ret;
    return stx.mode & 07777;
}

/*
 * Kernels before 6.6 have no fchmodat2, and CI runs newer ones. So a forked
 * child stands in for an old kernel: a seccomp filter makes SYS_fchmodat2
 * fail with ENOSYS, and the child repeats the AT_SYMLINK_NOFOLLOW call. That
 * checks the -ENOSYS mapping, and that the wrapper does not fall back to
 * fchmodat, on every kernel. The child makes only native syscalls, so the
 * filter skips the usual arch check. Where seccomp filters are unavailable
 * (qemu-user refuses them), the child prints SKIP:.
 */
#define PR_SET_NO_NEW_PRIVS     38
#define SECCOMP_SET_MODE_FILTER 1
#define SECCOMP_RET_ERRNO       0x00050000u
#define SECCOMP_RET_ALLOW       0x7fff0000u
#define BPF_LD_W_ABS            0x20    /* BPF_LD | BPF_W | BPF_ABS */
#define BPF_JEQ_K               0x15    /* BPF_JMP | BPF_JEQ | BPF_K */
#define BPF_RET_K               0x06    /* BPF_RET | BPF_K */

/* struct sock_filter and struct sock_fprog (linux/filter.h). */
typedef struct {
    uint16_t code;
    uint8_t  jt;
    uint8_t  jf;
    uint32_t k;
} test_sock_filter;

typedef struct {
    uint16_t len;
    const test_sock_filter *filter;
} test_sock_fprog;
_Static_assert(sizeof(test_sock_filter) == 8, "test_sock_filter must be 8 bytes (struct sock_filter)");
_Static_assert(sizeof(test_sock_fprog) == 16, "test_sock_fprog must be 16 bytes (struct sock_fprog)");

static void fchmodat_without_fchmodat2(void) {
    lc_sysret pid = lc_kernel_fork();
    TEST_ASSERT(pid >= 0);
    if (pid < 0) return;
    if (pid == 0) {
        static const test_sock_filter filter[] = {
            { BPF_LD_W_ABS, 0, 0, 0 },                      /* seccomp_data.nr */
            { BPF_JEQ_K,    0, 1, SYS_fchmodat2 },
            { BPF_RET_K,    0, 0, SECCOMP_RET_ERRNO | LC_ERR_NOSYS },
            { BPF_RET_K,    0, 0, SECCOMP_RET_ALLOW },
        };
        test_sock_fprog prog = { sizeof filter / sizeof filter[0], filter };
        lc_sysret ret = lc_kernel_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
        if (ret == 0) ret = lc_kernel_seccomp(SECCOMP_SET_MODE_FILTER, 0, &prog);
        if (ret < 0) {
            _test_print_text("SKIP: seccomp filters not available, cannot fail fchmodat2 with ENOSYS: ");
            lc_print_signed(TEST_FD, ret);
            lc_print_newline(TEST_FD);
            lc_kernel_exit(CHILD_SKIPPED);
        }
        ret = lc_kernel_fchmodat(AT_FDCWD, CHMOD_LINK, CHMOD_MODE, AT_SYMLINK_NOFOLLOW);
        _test_print_text("  ... with fchmodat2 failing with ENOSYS (before Linux 6.6) = ");
        lc_print_signed(TEST_FD, ret);
        lc_print_newline(TEST_FD);
        lc_kernel_exit(ret == -LC_ERR_OPNOTSUPP ? 0 : 1);
    }

    int32_t status = 0;
    lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    if (status != (CHILD_SKIPPED << 8)) {
        TEST_ASSERT_EQ(status, 0);      /* the child got -EOPNOTSUPP */
    }
}

static void test_fchmodat_symlink_nofollow(void) {
    /* Start clean: O_TRUNC keeps the mode of a target left by an aborted run. */
    (void)lc_kernel_unlinkat(AT_FDCWD, CHMOD_LINK, 0);
    (void)lc_kernel_unlinkat(AT_FDCWD, CHMOD_TARGET, 0);
    TEST_ASSERT_OK(lc_file_write_all(CHMOD_TARGET, "x", 1));
    /* A relative symlink target resolves from the link's directory (build/). */
    TEST_ASSERT_EQ(lc_kernel_symlinkat("test_syscall_chmod_target", AT_FDCWD, CHMOD_LINK), 0);

    lc_sysret before = file_mode(CHMOD_TARGET);
    TEST_ASSERT(before >= 0);
    TEST_ASSERT_NEQ(before, CHMOD_MODE);

    lc_sysret ret = lc_kernel_fchmodat(AT_FDCWD, CHMOD_LINK, CHMOD_MODE, AT_SYMLINK_NOFOLLOW);
    _test_print_text("  lc_kernel_fchmodat(link, 0700, AT_SYMLINK_NOFOLLOW) = ");
    lc_print_signed(TEST_FD, ret);
    lc_print_newline(TEST_FD);
    TEST_ASSERT_EQ(ret, -LC_ERR_OPNOTSUPP);
    TEST_ASSERT_EQ(file_mode(CHMOD_TARGET), before);   /* target untouched */

    fchmodat_without_fchmodat2();
    TEST_ASSERT_EQ(file_mode(CHMOD_TARGET), before);   /* still untouched */

    TEST_ASSERT_EQ(lc_kernel_fchmodat(AT_FDCWD, CHMOD_LINK, CHMOD_MODE, 0), 0);
    TEST_ASSERT_EQ(file_mode(CHMOD_TARGET), CHMOD_MODE);   /* followed the link */

    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, CHMOD_LINK, 0), 0);
    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, CHMOD_TARGET, 0), 0);
}

/* lc_kernel_clock_sleep: a deadline that has passed returns at once, one
 * 20 ms ahead returns once it has passed, a duration sleeps, and a clock
 * that does not exist is refused. */
static void test_clock_sleep(void) {
    lc_timespec now;
    TEST_REQUIRE(lc_kernel_get_clock(LC_CLOCK_MONOTONIC, &now) == 0);
    TEST_ASSERT_EQ(lc_kernel_clock_sleep(LC_CLOCK_MONOTONIC, LC_TIMER_ABSTIME, &now, NULL), 0);

    lc_timespec deadline = now;
    deadline.nanoseconds += 20000000;
    if (deadline.nanoseconds >= 1000000000) {
        deadline.seconds++;
        deadline.nanoseconds -= 1000000000;
    }
    TEST_ASSERT_EQ(lc_kernel_clock_sleep(LC_CLOCK_MONOTONIC, LC_TIMER_ABSTIME, &deadline, NULL), 0);
    lc_timespec after;
    TEST_REQUIRE(lc_kernel_get_clock(LC_CLOCK_MONOTONIC, &after) == 0);
    TEST_ASSERT(after.seconds > deadline.seconds ||
                (after.seconds == deadline.seconds && after.nanoseconds >= deadline.nanoseconds));

    lc_timespec duration = { 0, 1000000 }, remaining = { 7, 7 };
    TEST_ASSERT_EQ(lc_kernel_clock_sleep(LC_CLOCK_MONOTONIC, 0, &duration, &remaining), 0);
    TEST_ASSERT_EQ(lc_kernel_clock_sleep(9999, 0, &duration, NULL), -(lc_sysret)LC_ERR_INVAL);
}

/* lc_kernel_read_all, lc_kernel_read_retry and lc_kernel_write_all go on
 * after a handled signal interrupts them, where the other wrappers return
 * -EINTR. Each blocks on a pipe, a SIGALRM comes at 20 ms, and a child
 * gives the data, or makes room, at 60 ms. */
#define ITIMER_REAL 0
#define F_GETFL     3
#define F_SETFL     4

static volatile int32_t eintr_alarms;
static void count_eintr_alarm(int signo) { (void)signo; eintr_alarms++; }

static void alarm_at_20_ms(void) {
    int64_t timer[4] = { 0, 0, 0, 20000 };   /* struct itimerval: 20 ms, once */
    (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)timer, 0);
}

static void test_read_write_retry_eintr(void) {
    TEST_REQUIRE_OK(lc_signal_handle(LC_SIGALRM, count_eintr_alarm));
    int32_t pipe_fds[2];
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(pipe_fds, 0), 0);

    eintr_alarms = 0;
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        lc_time_sleep_milliseconds(60);
        lc_kernel_exit(lc_kernel_write_all(pipe_fds[1], "DATA", 4) == 4 ? 0 : 1);
    }
    TEST_REQUIRE(pid > 0);
    alarm_at_20_ms();
    char got[4];
    TEST_ASSERT_EQ(lc_kernel_read_all(pipe_fds[0], got, sizeof got), (lc_sysret)4);
    TEST_ASSERT_STR_EQ(got, 4, "DATA", 4);
    TEST_ASSERT_EQ(eintr_alarms, 1);
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status, 0);

    /* lc_kernel_read_retry: one read, made again after the signal. */
    pid = lc_kernel_fork();
    if (pid == 0) {
        lc_time_sleep_milliseconds(60);
        lc_kernel_exit(lc_kernel_write_all(pipe_fds[1], "R", 1) == 1 ? 0 : 1);
    }
    TEST_REQUIRE(pid > 0);
    alarm_at_20_ms();
    TEST_ASSERT_EQ(lc_kernel_read_retry(pipe_fds[0], got, sizeof got), (lc_sysret)1);
    TEST_ASSERT_EQ(got[0], 'R');
    TEST_ASSERT_EQ(eintr_alarms, 2);
    status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status, 0);

    /* Fill the pipe, so that the next write waits for room. */
    static char block[4096];
    int64_t flags = lc_syscall3(SYS_fcntl, pipe_fds[1], F_GETFL, 0);
    TEST_REQUIRE(flags >= 0);
    TEST_REQUIRE_EQ(lc_syscall3(SYS_fcntl, pipe_fds[1], F_SETFL, flags | O_NONBLOCK), 0);
    size_t filled = 0;
    for (;;) {
        lc_sysret n = lc_kernel_write_bytes(pipe_fds[1], block, sizeof block);
        if (n <= 0) break;
        filled += (size_t)n;
    }
    TEST_REQUIRE_EQ(lc_syscall3(SYS_fcntl, pipe_fds[1], F_SETFL, flags), 0);

    size_t total = filled + sizeof block;
    pid = lc_kernel_fork();
    if (pid == 0) {
        lc_kernel_close_file(pipe_fds[1]);   /* so a write cut short ends the drain */
        lc_time_sleep_milliseconds(60);
        size_t drained = 0;
        while (drained < total) {
            lc_sysret n = lc_kernel_read_bytes(pipe_fds[0], block, sizeof block);
            if (n <= 0) break;
            drained += (size_t)n;
        }
        lc_kernel_exit(drained == total ? 0 : 1);
    }
    TEST_REQUIRE(pid > 0);
    alarm_at_20_ms();
    TEST_ASSERT_EQ(lc_kernel_write_all(pipe_fds[1], block, sizeof block), (lc_sysret)sizeof block);
    TEST_ASSERT_EQ(eintr_alarms, 3);
    lc_kernel_close_file(pipe_fds[1]);
    status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status, 0);
    lc_kernel_close_file(pipe_fds[0]);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGALRM));
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    TEST_RUN(test_mount_tmpfs);
    TEST_RUN(test_fchmodat_symlink_nofollow);
    TEST_RUN(test_clock_sleep);
    TEST_RUN(test_read_write_retry_eintr);

    return test_main();
}
