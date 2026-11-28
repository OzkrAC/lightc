/*
 * test_probe — lc_probe_kernel, and what lightc does without each feature
 * it probes.
 *
 * CI runs kernels that have every feature, so a seccomp filter stands in
 * for a kernel without one: in a forked child it makes the feature's calls
 * fail with ENOSYS, as a kernel without them does, or with EPERM, as some
 * container profiles do. The child then probes, and takes each path of
 * lightc that depends on the feature, which must take the fallback that
 * docs/contracts.html#kernel gives.
 *
 * lc_probe_kernel keeps its answers for the life of the process, and a fork
 * copies them, so this suite never probes a feature in its own process:
 * every probe is made in a child forked for it, which starts with no
 * answer. What a
 * child without a filter finds is the reference the others compare with.
 * The files go in build/, so run this from the source root, as ninja test
 * does.
 *
 * qemu-user runs no guest's seccomp filter, so there the six tests that
 * need one skip, as ci/qemu-skips.txt counts. The other three run, and the
 * probes then see what qemu-user implements, which has no io_uring: those
 * tests check that each answer agrees with a real use of the feature, and
 * that threads which probe at once agree.
 */

#include "test.h"
#include <lightc/async.h>
#include <lightc/io.h>
#include <lightc/probe.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightio/lightio.h>

/* --- seccomp and BPF, as <linux/seccomp.h> and <linux/filter.h> give them --- */

#define PR_SET_NO_NEW_PRIVS      38
#define SECCOMP_SET_MODE_FILTER  1
#define SECCOMP_RET_KILL_PROCESS 0x80000000u
#define SECCOMP_RET_ERRNO        0x00050000u
#define SECCOMP_RET_ALLOW        0x7fff0000u
#define BPF_LD_W_ABS             0x20    /* BPF_LD | BPF_W | BPF_ABS */
#define BPF_JEQ_K                0x15    /* BPF_JMP | BPF_JEQ | BPF_K */
#define BPF_RET_K                0x06    /* BPF_RET | BPF_K */
#define ITIMER_REAL              0

#if defined(__x86_64__)
#define AUDIT_ARCH_THIS 0xc000003eu
#elif defined(__aarch64__)
#define AUDIT_ARCH_THIS 0xc00000b7u
#endif

/* struct sock_filter and struct sock_fprog */
typedef struct {
    uint16_t code;
    uint8_t  jt;
    uint8_t  jf;
    uint32_t k;
} bpf_insn;

typedef struct {
    uint16_t        length;
    const bpf_insn *filter;
} bpf_program;

/* --- What the real uses of the features take, as the UAPI headers give it --- */

#define O_PATH            010000000   /* <asm-generic/fcntl.h>, both arches */
#define AT_EMPTY_PATH     0x1000      /* <linux/fcntl.h> */
#define STATX_TYPE        0x1U        /* <linux/stat.h> */
#define STATX_MODE        0x2U
#define S_IFMT            0170000
#define S_IFLNK           0120000
#define OPEN_TREE_CLOEXEC O_CLOEXEC   /* <linux/mount.h> */
#define FSOPEN_CLOEXEC    0x1
#define FSPICK_CLOEXEC    0x1
#define FSMOUNT_CLOEXEC   0x1
#define FSCONFIG_CMD_CREATE 6

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

/* struct open_how, of <linux/openat2.h> */
typedef struct {
    uint64_t flags;
    uint64_t mode;
    uint64_t resolve;
} test_open_how;
_Static_assert(sizeof(test_open_how) == 24, "test_open_how must be 24 bytes (struct open_how)");

/* struct clone_args of <linux/sched.h>, as Linux 5.3 has it (CLONE_ARGS_SIZE_VER0) */
typedef struct {
    uint64_t flags;
    uint64_t pidfd;
    uint64_t child_tid;
    uint64_t parent_tid;
    uint64_t exit_signal;
    uint64_t stack;
    uint64_t stack_size;
    uint64_t tls;
} test_clone_args;
_Static_assert(sizeof(test_clone_args) == 64, "test_clone_args must be 64 bytes (CLONE_ARGS_SIZE_VER0)");

/* struct mount_attr, of <linux/mount.h> (MOUNT_ATTR_SIZE_VER0) */
typedef struct {
    uint64_t attr_set;
    uint64_t attr_clr;
    uint64_t propagation;
    uint64_t userns_fd;
} test_mount_attr;
_Static_assert(sizeof(test_mount_attr) == 32, "test_mount_attr must be 32 bytes (MOUNT_ATTR_SIZE_VER0)");

/* --- The features --- */

#define MAX_CALLS 6

typedef struct {
    lc_kernel_feature feature;
    const char       *name;
    const char       *since;     /* the Linux release that added it */
    uint32_t          count;     /* how many of calls the probe makes */
    int64_t           calls[MAX_CALLS];
    const char       *call_names[MAX_CALLS];
    /* The calls of the feature that answer a caller without CAP_SYS_ADMIN
     * with EPERM, so that an EPERM from them is no sign of a filter. */
    uint32_t          eperm_is_the_kernels;   /* a bit per call */
} feature_case;

static const feature_case features[] = {
    { LC_KFEAT_FCHMODAT2,     "LC_KFEAT_FCHMODAT2",     "6.6",  1, { SYS_fchmodat2 }, { "fchmodat2" }, 0 },
    { LC_KFEAT_FACCESSAT2,    "LC_KFEAT_FACCESSAT2",    "5.8",  1, { SYS_faccessat2 }, { "faccessat2" }, 0 },
    { LC_KFEAT_OPENAT2,       "LC_KFEAT_OPENAT2",       "5.6",  1, { SYS_openat2 }, { "openat2" }, 0 },
    { LC_KFEAT_CLOSE_RANGE,   "LC_KFEAT_CLOSE_RANGE",   "5.9",  1, { SYS_close_range }, { "close_range" }, 0 },
    { LC_KFEAT_CLONE3,        "LC_KFEAT_CLONE3",        "5.3",  1, { SYS_clone3 }, { "clone3" }, 0 },
    { LC_KFEAT_PIDFD,         "LC_KFEAT_PIDFD",         "5.3",  1, { SYS_pidfd_open }, { "pidfd_open" }, 0 },
    { LC_KFEAT_MOUNT_API,     "LC_KFEAT_MOUNT_API",     "5.2",  6,
      { SYS_open_tree, SYS_move_mount, SYS_fsopen, SYS_fsconfig, SYS_fsmount, SYS_fspick },
      { "open_tree", "move_mount", "fsopen", "fsconfig", "fsmount", "fspick" },
      (1u << 1) | (1u << 2) | (1u << 4) | (1u << 5) },
    { LC_KFEAT_MOUNT_SETATTR, "LC_KFEAT_MOUNT_SETATTR", "5.12", 1, { SYS_mount_setattr }, { "mount_setattr" }, 0 },
    { LC_KFEAT_IO_URING,      "LC_KFEAT_IO_URING",      "5.1",  1, { SYS_io_uring_setup }, { "io_uring_setup" }, 0 },
};

#define FEATURE_COUNT (sizeof features / sizeof features[0])
#define ANSWERS       (LC_KFEAT_IO_URING + 1)   /* indexed by feature */

/* Every probed call of every feature, for a filter that refuses them all. */
#define ALL_CALLS 14

/* --- Children and filters --- */

#define CHILD_SECONDS 10

/* Runs body(arg) in a forked child, under a watchdog of CHILD_SECONDS.
 * Returns the child's exit status: 0 when every check of body passed, 1
 * when one failed, or 128 plus the signal that killed it: 142 is the
 * watchdog's SIGALRM. -1 if the fork failed. */
static int32_t in_child(void (*body)(const void *arg), const void *arg) {
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, CHILD_SECONDS, 0 };   /* struct itimerval, once */
        (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)watchdog, 0);
        _test_current_failed = 0;
        body(arg);
        lc_kernel_exit(_test_current_failed > 0 ? 1 : 0);
    }
    if (pid < 0) return -1;
    int32_t status = 0;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    return (status & 0x7f) ? 128 + (status & 0x7f) : (status >> 8) & 0xff;
}

/* Makes each of the `count` system calls of `calls` fail with `error` in
 * this process from now on, and lets every other call through. True if the
 * filter is in place. The offsets are those of struct seccomp_data: nr at
 * 0, arch at 4. */
static bool refuse(const int64_t *calls, uint32_t count, int32_t error) {
    bpf_insn filter[3 + ALL_CALLS + 2];
    if (count > ALL_CALLS) return false;
    uint32_t k = 0;
    filter[k++] = (bpf_insn){ BPF_LD_W_ABS, 0, 0, 4 };
    filter[k++] = (bpf_insn){ BPF_JEQ_K, 0, (uint8_t)(count + 1), AUDIT_ARCH_THIS };
    filter[k++] = (bpf_insn){ BPF_LD_W_ABS, 0, 0, 0 };
    for (uint32_t i = 0; i < count; i++)
        filter[k++] = (bpf_insn){ BPF_JEQ_K, (uint8_t)(count - i), 0, (uint32_t)calls[i] };
    filter[k++] = (bpf_insn){ BPF_RET_K, 0, 0, SECCOMP_RET_ALLOW };
    filter[k++] = (bpf_insn){ BPF_RET_K, 0, 0, SECCOMP_RET_ERRNO | (uint32_t)error };
    bpf_program program = { (uint16_t)k, filter };
    if (lc_kernel_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) return false;
    return lc_kernel_seccomp(SECCOMP_SET_MODE_FILTER, 0, &program) == 0;
}

/* Exits 0 if it installs a filter, 1 if not, printing nothing. */
static void install_allow_all(const void *arg) {
    (void)arg;
    static const bpf_insn allow_all[4] = {
        { BPF_LD_W_ABS, 0, 0, 4 },
        { BPF_JEQ_K, 1, 0, AUDIT_ARCH_THIS },
        { BPF_RET_K, 0, 0, SECCOMP_RET_KILL_PROCESS },
        { BPF_RET_K, 0, 0, SECCOMP_RET_ALLOW },
    };
    bpf_program all = { 4, allow_all };
    (void)lc_kernel_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    lc_kernel_exit(lc_kernel_seccomp(SECCOMP_SET_MODE_FILTER, 0, &all) == 0 ? 0 : 1);
}

/* Does a seccomp filter work here? A child installs one that allows every
 * call of this arch and kills the process on any other: an emulator that
 * refuses the filter, or hands it to its host process, makes the child
 * exit 1 or die. */
static bool seccomp_works(void) {
    static int answer = -1;
    if (answer < 0) answer = in_child(install_allow_all, NULL) == 0;
    return answer;
}

#define REQUIRE_SECCOMP()                                                    \
    do {                                                                     \
        if (!seccomp_works())                                                \
            TEST_SKIP("seccomp filters do not work here: qemu-user does not implement them"); \
    } while (0)

/* --- The reference: what a child without a filter finds --- */

static int32_t *reference;   /* a shared page, by feature */

static void probe_all(int32_t *out) {
    for (size_t i = 0; i < FEATURE_COUNT; i++)
        out[features[i].feature] = lc_probe_kernel(features[i].feature).error;
}

static void probe_all_into(const void *arg) {
    probe_all((int32_t *)(uintptr_t)arg);
}

/* Fills `reference` once; false if it could not. */
static bool find_reference(void) {
    static bool found = false;
    if (found) return true;
    if (!reference) {
        void *page = lc_kernel_map_memory(NULL, 4096, PROT_READ | PROT_WRITE,
                                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (page == MAP_FAILED) return false;
        reference = page;
    }
    found = in_child(probe_all_into, reference) == 0;
    return found;
}

static void print_answer(int32_t error) {
    if (error == 0) {
        _test_print_text("present");
    } else {
        _test_print_text("absent, ");
        _test_print_text(lc_error_name(error));
    }
}

/* Checks that each feature but `except` has the reference's answer. */
static void check_others(const int32_t *answers, lc_kernel_feature except) {
    for (size_t i = 0; i < FEATURE_COUNT; i++) {
        if (features[i].feature == except) continue;
        TEST_ASSERT_EQ(answers[features[i].feature], reference[features[i].feature]);
    }
}

/* ===== The features lc_probe_kernel knows ===== */

static void test_unknown_features_are_invalid(void) {
    /* Each LC_KFEAT_* value has a case here, and the values run from 1. */
    for (int32_t f = LC_KFEAT_FCHMODAT2; f <= (int32_t)LC_KFEAT_IO_URING; f++) {
        size_t cases = 0;
        for (size_t i = 0; i < FEATURE_COUNT; i++)
            if ((int32_t)features[i].feature == f) cases++;
        TEST_ASSERT_EQ(cases, (size_t)1);
    }
    TEST_ASSERT_EQ(FEATURE_COUNT, (size_t)LC_KFEAT_IO_URING);

    /* A value that is no feature probes nothing, so this process still has
     * no answer kept. */
    TEST_ASSERT_EQ(lc_probe_kernel((lc_kernel_feature)0).error, LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_probe_kernel((lc_kernel_feature)(LC_KFEAT_IO_URING + 1)).error, LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_probe_kernel((lc_kernel_feature)-1).error, LC_ERR_INVAL);
}

/* ===== Each answer agrees with a real use of the feature =====
 *
 * A real use makes the feature's calls as a program would, with arguments
 * the kernel takes, and undoes what it did. It gives 0 when the feature
 * works, or the error that stopped it. The probe's arguments are ones the
 * kernel refuses, so a probe that read the kernel's answer wrong, or that
 * the kernel answered other than this suite expects, would disagree. */

#define USE_FILE "build/test_probe_use"

static int32_t error_of(lc_sysret ret) {
    return ret < 0 ? (int32_t)-ret : 0;
}

/* The answer of a mount API call that a caller without CAP_SYS_ADMIN gets
 * EPERM from, made with arguments the kernel takes: only ENOSYS counts. */
static int32_t mount_call(lc_sysret ret) {
    if (ret >= 0) (void)lc_kernel_close_file((int32_t)ret);
    return ret == -LC_ERR_NOSYS ? LC_ERR_NOSYS : 0;
}

static int32_t use(lc_kernel_feature feature) {
    switch (feature) {
    case LC_KFEAT_FCHMODAT2: {
        lc_sysret fd = lc_kernel_open_file(USE_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        if (fd < 0) return error_of(fd);
        (void)lc_kernel_close_file((int32_t)fd);
        lc_sysret ret = lc_syscall4(SYS_fchmodat2, AT_FDCWD, (int64_t)USE_FILE, 0600, AT_SYMLINK_NOFOLLOW);
        (void)lc_kernel_unlinkat(AT_FDCWD, USE_FILE, 0);
        return error_of(ret);
    }
    case LC_KFEAT_FACCESSAT2:
        return error_of(lc_kernel_faccessat2(AT_FDCWD, "/", F_OK, AT_EACCESS));
    case LC_KFEAT_OPENAT2: {
        test_open_how how = { O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0, 0 };
        lc_sysret fd = lc_syscall4(SYS_openat2, AT_FDCWD, (int64_t)"/", (int64_t)&how, sizeof how);
        if (fd >= 0) (void)lc_kernel_close_file((int32_t)fd);
        return error_of(fd);
    }
    case LC_KFEAT_CLOSE_RANGE: {
        lc_sysret fd = lc_kernel_open_file("/dev/null", O_RDONLY | O_CLOEXEC, 0);
        if (fd < 0) return error_of(fd);
        lc_sysret ret = lc_syscall3(SYS_close_range, fd, fd, 0);
        if (ret == 0) {
            /* It closed fd, and only fd. */
            TEST_ASSERT_EQ(lc_kernel_close_file((int32_t)fd), -LC_ERR_BADF);
        } else {
            (void)lc_kernel_close_file((int32_t)fd);
        }
        return error_of(ret);
    }
    case LC_KFEAT_CLONE3: {
        /* A fork: no CLONE_VM and no stack, so the child runs on a copy of
         * this one, and leaves at once. */
        test_clone_args args = { .exit_signal = SIGCHLD };
        lc_sysret pid = lc_syscall2(SYS_clone3, (int64_t)&args, sizeof args);
        if (pid == 0) lc_kernel_exit(0);
        if (pid < 0) return error_of(pid);
        int32_t status = -1;
        TEST_ASSERT_EQ(lc_kernel_wait_for_child((int32_t)pid, &status, 0), pid);
        TEST_ASSERT_EQ(status, 0);
        return 0;
    }
    case LC_KFEAT_PIDFD: {
        lc_sysret fd = lc_syscall2(SYS_pidfd_open, lc_kernel_get_process_id(), 0);
        if (fd >= 0) (void)lc_kernel_close_file((int32_t)fd);
        return error_of(fd);
    }
    case LC_KFEAT_MOUNT_API: {
        /* open_tree without OPEN_TREE_CLONE needs no privilege: it gives an
         * O_PATH fd of the mount. fsconfig refuses fd -1 with EINVAL. The
         * other four refuse such a caller with EPERM, and a privileged one
         * gets an fd, or an error for the fd -1 and empty paths. */
        lc_sysret fd = lc_syscall3(SYS_open_tree, AT_FDCWD, (int64_t)"/", OPEN_TREE_CLOEXEC);
        if (fd < 0) return error_of(fd);
        (void)lc_kernel_close_file((int32_t)fd);
        lc_sysret ret = lc_syscall5(SYS_fsconfig, -1, FSCONFIG_CMD_CREATE, 0, 0, 0);
        if (ret == -LC_ERR_NOSYS || ret == -LC_ERR_PERM) return error_of(ret);
        int32_t error = mount_call(lc_syscall5(SYS_move_mount, -1, (int64_t)"", -1, (int64_t)"", 0));
        if (error == 0) error = mount_call(lc_syscall2(SYS_fsopen, (int64_t)"tmpfs", FSOPEN_CLOEXEC));
        if (error == 0) error = mount_call(lc_syscall3(SYS_fsmount, -1, FSMOUNT_CLOEXEC, 0));
        if (error == 0) error = mount_call(lc_syscall3(SYS_fspick, AT_FDCWD, (int64_t)"/", FSPICK_CLOEXEC));
        return error;
    }
    case LC_KFEAT_MOUNT_SETATTR: {
        /* Changes nothing: no attribute to set or clear. A caller without
         * CAP_SYS_ADMIN may get EPERM, which the check below allows. */
        test_mount_attr attr = {0};
        return error_of(lc_syscall5(SYS_mount_setattr, AT_FDCWD, (int64_t)"/", 0, (int64_t)&attr, sizeof attr));
    }
    case LC_KFEAT_IO_URING: {
        lc_result_ptr ring = lc_async_ring_create(4);
        if (lc_ptr_is_ok(ring)) lc_async_ring_destroy(ring.value);
        return ring.error;
    }
    }
    return LC_ERR_INVAL;
}

static void probes_agree_with_uses(const void *arg) {
    (void)arg;
    for (size_t i = 0; i < FEATURE_COUNT; i++) {
        const feature_case *c = &features[i];
        int32_t answer = lc_probe_kernel(c->feature).error;
        int32_t used = use(c->feature);
        _test_print_text("  ");
        _test_print_text(c->name);
        _test_print_text(" (Linux ");
        _test_print_text(c->since);
        _test_print_text("): ");
        print_answer(answer);
        _test_print_text("; a real use gives ");
        _test_print_text(lc_error_name(used));
        lc_print_newline(TEST_FD);
        if (answer != 0) {
            TEST_ASSERT_EQ(used, answer);
        } else if (c->feature == LC_KFEAT_MOUNT_SETATTR) {
            TEST_ASSERT(used == 0 || used == LC_ERR_PERM);
        } else {
            TEST_ASSERT_EQ(used, 0);
        }
    }
}

static void test_probes_agree_with_real_uses(void) {
    TEST_ASSERT_EQ(in_child(probes_agree_with_uses, NULL), 0);
}

/* ===== Threads that probe at once all get the right answer ===== */

#define PROBE_THREADS 8

typedef struct {
    lc_barrier *start;
    uint32_t    first;               /* which feature it probes first */
    int32_t     answers[ANSWERS];
} prober;

static int32_t probe_from_thread(void *arg) {
    prober *p = arg;
    lc_barrier_wait(p->start);
    for (size_t i = 0; i < FEATURE_COUNT; i++) {
        lc_kernel_feature f = features[(p->first + i) % FEATURE_COUNT].feature;
        p->answers[f] = lc_probe_kernel(f).error;
    }
    return 0;
}

static void threads_probe_at_once(const void *arg) {
    (void)arg;
    static lc_barrier start;
    static prober probers[PROBE_THREADS];
    static lc_thread threads[PROBE_THREADS];
    lc_barrier_init(&start, PROBE_THREADS);
    for (uint32_t t = 0; t < PROBE_THREADS; t++) {
        probers[t].start = &start;
        /* Half start with the same feature, so two threads probe one at
         * once; the rest each start with another. */
        probers[t].first = t < PROBE_THREADS / 2 ? 0 : t;
        for (size_t f = 0; f < ANSWERS; f++) probers[t].answers[f] = -1;
        TEST_REQUIRE_OK(lc_thread_create(&threads[t], probe_from_thread, &probers[t]));
    }
    for (uint32_t t = 0; t < PROBE_THREADS; t++) lc_thread_join(&threads[t]);
    for (uint32_t t = 0; t < PROBE_THREADS; t++) {
        check_others(probers[t].answers, (lc_kernel_feature)0);
    }
    /* And what they kept is what a later call gets. */
    int32_t again[ANSWERS];
    probe_all(again);
    check_others(again, (lc_kernel_feature)0);
}

static void test_threads_probing_at_once_agree(void) {
    TEST_REQUIRE(find_reference());
    TEST_ASSERT_EQ(in_child(threads_probe_at_once, NULL), 0);
}

/* ===== Under a filter, each probe reports its feature absent ===== */

typedef struct {
    const feature_case *feature;
    uint32_t            first, count;  /* which of its calls the filter refuses */
    int32_t             error;          /* and with what */
    int32_t             expected;       /* what the probe must give, or UNCHECKED */
} refusal;

/* A filter's errno replaces the one an older filter gives the same call:
 * the newest filter's wins. So where the reference finds a feature absent,
 * as a sandbox around this suite can make it, an EPERM from a call whose
 * EPERM the probe counts as the kernel's can hide what made it absent, and
 * the probe may find the feature present. */
#define UNCHECKED (-1)

static void probe_under_filter(const void *arg) {
    const refusal *r = arg;
    TEST_REQUIRE(refuse(r->feature->calls + r->first, r->count, r->error));
    int32_t answers[ANSWERS];
    probe_all(answers);
    _test_print_text("  ");
    _test_print_text(r->feature->name);
    _test_print_text(", with ");
    for (uint32_t i = 0; i < r->count; i++) {
        if (i > 0) _test_print_text(" and ");
        _test_print_text(r->feature->call_names[r->first + i]);
    }
    _test_print_text(" refused with ");
    _test_print_text(lc_error_name(r->error));
    _test_print_text(": ");
    print_answer(answers[r->feature->feature]);
    lc_print_newline(TEST_FD);
    if (r->expected == UNCHECKED) {
        /* See UNCHECKED. */
    } else if (r->expected != 0 && reference[r->feature->feature] != 0) {
        /* Absent already, so an earlier call of the feature may have
         * stopped the probe before the refused one. */
        TEST_ASSERT_NEQ(answers[r->feature->feature], 0);
    } else {
        TEST_ASSERT_EQ(answers[r->feature->feature], r->expected);
    }
    check_others(answers, r->feature->feature);
}

static void check_refusal(const feature_case *c, uint32_t first, uint32_t count,
                          int32_t error, int32_t expected) {
    refusal r = { c, first, count, error, expected };
    TEST_ASSERT_EQ(in_child(probe_under_filter, &r), 0);
}

static void test_each_probe_is_absent_under_enosys(void) {
    REQUIRE_SECCOMP();
    TEST_REQUIRE(find_reference());
    for (size_t i = 0; i < FEATURE_COUNT; i++) {
        const feature_case *c = &features[i];
        check_refusal(c, 0, c->count, LC_ERR_NOSYS, LC_ERR_NOSYS);
        /* One call of the mount API that is absent makes it all absent. */
        for (uint32_t k = 0; c->count > 1 && k < c->count; k++)
            check_refusal(c, k, 1, LC_ERR_NOSYS, LC_ERR_NOSYS);
    }
}

static void test_an_eperm_filter_makes_a_feature_absent(void) {
    REQUIRE_SECCOMP();
    TEST_REQUIRE(find_reference());
    for (size_t i = 0; i < FEATURE_COUNT; i++) {
        const feature_case *c = &features[i];
        for (uint32_t k = 0; k < c->count; k++) {
            /* An EPERM that the kernel gives a caller without CAP_SYS_ADMIN
             * changes nothing. */
            bool kernels = (c->eperm_is_the_kernels >> k) & 1u;
            int32_t expected = LC_ERR_PERM;
            if (kernels) expected = reference[c->feature] == 0 ? 0 : UNCHECKED;
            check_refusal(c, k, 1, LC_ERR_PERM, expected);
        }
    }
}

/* ===== The answer is kept: a probe tests its feature once ===== */

static int64_t all_calls[ALL_CALLS];

static void same_as(const void *arg) {
    const int32_t *before = arg;
    int32_t answers[ANSWERS];
    probe_all(answers);
    for (size_t i = 0; i < FEATURE_COUNT; i++)
        TEST_ASSERT_EQ(answers[features[i].feature], before[features[i].feature]);
}

static void answers_are_kept(const void *arg) {
    (void)arg;
    int32_t before[ANSWERS];
    probe_all(before);
    check_others(before, (lc_kernel_feature)0);

    /* Every probed call now fails with ENOSYS ... */
    TEST_REQUIRE(refuse(all_calls, ALL_CALLS, LC_ERR_NOSYS));
    TEST_ASSERT_EQ(lc_syscall2(SYS_pidfd_open, 0, 0), -LC_ERR_NOSYS);
    /* ... and the answers stay those of the first probes, in this process
     * and in a child of fork, which inherits them. */
    same_as(before);
    TEST_ASSERT_EQ(in_child(same_as, before), 0);
}

static void test_an_answer_is_kept(void) {
    REQUIRE_SECCOMP();
    TEST_REQUIRE(find_reference());
    uint32_t n = 0;
    for (size_t i = 0; i < FEATURE_COUNT; i++)
        for (uint32_t k = 0; k < features[i].count; k++) all_calls[n++] = features[i].calls[k];
    TEST_REQUIRE_EQ(n, (uint32_t)ALL_CALLS);
    TEST_ASSERT_EQ(in_child(answers_are_kept, NULL), 0);
}

/* ===== lc_kernel_fchmodat without fchmodat2 =====
 *
 * With flags, lc_kernel_fchmodat needs fchmodat2 (Linux 6.6); without it,
 * it returns -EOPNOTSUPP and changes nothing. The fallback that
 * docs/contracts.html#kernel gives a caller is chmod_nofollow below: it
 * changes the mode of anything but a symlink, which has no mode to change.
 */

#define CHMOD_TARGET "build/test_probe_chmod_target"
#define CHMOD_LINK   "build/test_probe_chmod_link"

/* Permission bits of path, following symlinks, or a negative errno. */
static lc_sysret file_mode(const char *path) {
    test_statx stx = {0};
    lc_sysret ret = lc_syscall5(SYS_statx, AT_FDCWD, (int64_t)path, 0, STATX_MODE, (int64_t)&stx);
    if (ret < 0) return ret;
    return stx.mode & 07777;
}

/* "/proc/self/fd/<fd>" into buf, which has room for 32 bytes. */
static void proc_fd_path(char *buf, int32_t fd) {
    static const char prefix[] = "/proc/self/fd/";
    size_t n = sizeof prefix - 1;
    lc_bytes_copy(buf, prefix, n);
    char digits[12];
    size_t d = 0;
    uint32_t v = (uint32_t)fd;
    do {
        digits[d++] = (char)('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (d > 0) buf[n++] = digits[--d];
    buf[n] = '\0';
}

/* The fallback for fchmodat2 with AT_SYMLINK_NOFOLLOW: changes the mode of
 * path, relative to dirfd, unless it is a symlink. An O_PATH fd holds the
 * file without opening it, and fstat (here statx with AT_EMPTY_PATH) works
 * on it; fchmod does not. A chmod of its /proc/self/fd link changes the
 * mode of the file the fd holds, which a rename of path cannot swap for
 * another. */
static lc_sysret chmod_nofollow(int32_t dirfd, const char *path, int32_t mode) {
    lc_sysret fd = lc_kernel_open_file_at(dirfd, path, O_PATH | O_NOFOLLOW | O_CLOEXEC, 0);
    if (fd < 0) return fd;
    test_statx stx = {0};
    lc_sysret ret = lc_syscall5(SYS_statx, fd, (int64_t)"", AT_EMPTY_PATH, STATX_TYPE, (int64_t)&stx);
    if (ret == 0 && (stx.mode & S_IFMT) == S_IFLNK) ret = -LC_ERR_OPNOTSUPP;
    if (ret == 0) {
        char proc[32];
        proc_fd_path(proc, (int32_t)fd);
        ret = lc_kernel_fchmodat(AT_FDCWD, proc, mode, 0);
    }
    (void)lc_kernel_close_file((int32_t)fd);
    return ret;
}

static void fchmodat_without_fchmodat2(const void *arg) {
    (void)arg;
    const int64_t fchmodat2[1] = { SYS_fchmodat2 };
    TEST_REQUIRE(refuse(fchmodat2, 1, LC_ERR_NOSYS));
    TEST_ASSERT_EQ(lc_probe_kernel(LC_KFEAT_FCHMODAT2).error, LC_ERR_NOSYS);

    lc_sysret before = file_mode(CHMOD_TARGET);
    TEST_REQUIRE_EQ(before, 0644);

    /* The wrapper refuses, and leaves the link's target alone. */
    TEST_ASSERT_EQ(lc_kernel_fchmodat(AT_FDCWD, CHMOD_LINK, 0700, AT_SYMLINK_NOFOLLOW), -LC_ERR_OPNOTSUPP);
    TEST_ASSERT_EQ(lc_kernel_fchmodat(AT_FDCWD, CHMOD_TARGET, 0700, AT_SYMLINK_NOFOLLOW), -LC_ERR_OPNOTSUPP);
    TEST_ASSERT_EQ(file_mode(CHMOD_TARGET), before);

    /* The fallback refuses the symlink, and changes the file. */
    TEST_ASSERT_EQ(chmod_nofollow(AT_FDCWD, CHMOD_LINK, 0700), -LC_ERR_OPNOTSUPP);
    TEST_ASSERT_EQ(file_mode(CHMOD_TARGET), before);
    TEST_ASSERT_EQ(chmod_nofollow(AT_FDCWD, CHMOD_TARGET, 0700), 0);
    TEST_ASSERT_EQ(file_mode(CHMOD_TARGET), 0700);

    /* Without flags it is fchmodat, which every kernel has. */
    TEST_ASSERT_EQ(lc_kernel_fchmodat(AT_FDCWD, CHMOD_TARGET, 0600, 0), 0);
    TEST_ASSERT_EQ(file_mode(CHMOD_TARGET), 0600);
}

static void test_fchmodat_without_fchmodat2(void) {
    REQUIRE_SECCOMP();
    (void)lc_kernel_unlinkat(AT_FDCWD, CHMOD_LINK, 0);
    (void)lc_kernel_unlinkat(AT_FDCWD, CHMOD_TARGET, 0);
    lc_sysret fd = lc_kernel_open_file(CHMOD_TARGET, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    TEST_REQUIRE(fd >= 0);
    (void)lc_kernel_close_file((int32_t)fd);
    /* Whatever the umask took away. */
    TEST_REQUIRE_EQ(lc_kernel_fchmodat(AT_FDCWD, CHMOD_TARGET, 0644, 0), 0);
    /* A relative symlink target resolves from the link's directory, build/. */
    TEST_REQUIRE_EQ(lc_kernel_symlinkat("test_probe_chmod_target", AT_FDCWD, CHMOD_LINK), 0);

    TEST_ASSERT_EQ(in_child(fchmodat_without_fchmodat2, NULL), 0);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, CHMOD_LINK, 0), 0);
    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, CHMOD_TARGET, 0), 0);
}

/* ===== lc_kernel_faccessat2 without faccessat2 =====
 *
 * Every kernel lightc runs on has faccessat2 (Linux 5.8), but a seccomp
 * filter can take it away. lightc has no faccessat to fall back to, so the
 * wrapper returns what the kernel does: -ENOSYS.
 */

static void faccessat2_without_faccessat2(const void *arg) {
    (void)arg;
    const int64_t faccessat2[1] = { SYS_faccessat2 };
    TEST_REQUIRE(refuse(faccessat2, 1, LC_ERR_NOSYS));
    TEST_ASSERT_EQ(lc_probe_kernel(LC_KFEAT_FACCESSAT2).error, LC_ERR_NOSYS);
    TEST_ASSERT_EQ(lc_kernel_faccessat2(AT_FDCWD, "/", F_OK, 0), -LC_ERR_NOSYS);
    TEST_ASSERT_EQ(lc_kernel_faccessat2(AT_FDCWD, "/", F_OK, AT_EACCESS), -LC_ERR_NOSYS);
}

static void test_faccessat2_without_faccessat2(void) {
    REQUIRE_SECCOMP();
    TEST_ASSERT_EQ(in_child(faccessat2_without_faccessat2, NULL), 0);
}

/* ===== Rings without io_uring =====
 *
 * io_uring_setup fails with ENOSYS where the kernel lacks io_uring or a
 * filter takes it away, and with EPERM where kernel.io_uring_disabled
 * forbids it, which a filter stands in for here. lc_async_ring_create
 * returns that errno, and lio_loop_create returns NULL: there is no other
 * ring to fall back to.
 */

static void rings_without_io_uring(const void *arg) {
    int32_t error = *(const int32_t *)arg;
    const int64_t setup[1] = { SYS_io_uring_setup };
    TEST_REQUIRE(refuse(setup, 1, error));
    TEST_ASSERT_EQ(lc_probe_kernel(LC_KFEAT_IO_URING).error, error);
    lc_result_ptr ring = lc_async_ring_create(4);
    TEST_ASSERT_EQ(ring.error, error);
    TEST_ASSERT_NULL(ring.value);
    TEST_ASSERT_NULL(lio_loop_create());
}

static void test_rings_without_io_uring(void) {
    REQUIRE_SECCOMP();
    static const int32_t errors[2] = { LC_ERR_NOSYS, LC_ERR_PERM };
    for (size_t i = 0; i < 2; i++)
        TEST_ASSERT_EQ(in_child(rings_without_io_uring, &errors[i]), 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_unknown_features_are_invalid);
    TEST_RUN(test_probes_agree_with_real_uses);
    TEST_RUN(test_threads_probing_at_once_agree);
    TEST_RUN(test_each_probe_is_absent_under_enosys);
    TEST_RUN(test_an_eperm_filter_makes_a_feature_absent);
    TEST_RUN(test_an_answer_is_kept);
    TEST_RUN(test_fchmodat_without_fchmodat2);
    TEST_RUN(test_faccessat2_without_faccessat2);
    TEST_RUN(test_rings_without_io_uring);
    return test_main();
}
