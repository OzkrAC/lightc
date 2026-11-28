/*
 * sandbox.h — run a test body where an unprivileged user's container runs:
 * in new user, mount, pid and UTS namespaces, for the rootless tests of
 * lightc's container calls.
 *
 *   static void body(void *arg) {
 *       (void)arg;
 *       TEST_ASSERT_EQ(lc_kernel_get_process_id(), 1);
 *       TEST_ASSERT_EQ(lc_kernel_sethostname("box", 3), 0);
 *   }
 *
 *   static void test_something(void) {
 *       SANDBOX_RUN(body, NULL, SANDBOX_DENY_SETGROUPS);
 *   }
 *
 * SANDBOX_RUN(body, arg, flags) runs body(arg) in a child that clone makes
 * with CLONE_NEWUSER, CLONE_NEWNS, CLONE_NEWPID and CLONE_NEWUTS, and waits
 * for it. The child is pid 1 of its pid namespace, and has every capability
 * in its user namespace. Its parent, the keeper below, writes the
 * namespace's id maps before the body starts: by default uid 0 there is
 * the caller's effective uid and gid 0 its effective gid, one id each,
 * which is all an unprivileged caller may map. The child has its own copy
 * of the caller's mounts, which the kernel locks to their places, and a
 * host name of its own, so what the body mounts or names ends with it and
 * is never seen outside. Its memory is a copy of the caller's, as after a
 * fork: the body reads what the test set up before, and what it writes
 * stays in the child. The body starts in the caller's current directory,
 * on a stack of SANDBOX_STACK_SIZE bytes.
 *
 * The flags (0, or SANDBOX_* or'd):
 *
 *   SANDBOX_DENY_SETGROUPS  writes "deny" to the child's /proc/PID/setgroups
 *       before its gid map. An unprivileged caller can write a gid map only
 *       after that, and then setgroups fails with EPERM in the sandbox, so
 *       the caller's supplementary groups stay: the sandbox shows each as
 *       the overflow gid, but for the one the map gives.
 *   SANDBOX_NO_GID_MAP  writes no gid map, so no gid is mapped in the
 *       sandbox: its own gids show as the overflow gid, setgroups fails with
 *       EPERM, and making a file on a tmpfs it mounted fails with EOVERFLOW.
 *
 * With neither, the gid map is written as it is, which needs CAP_SETGID
 * over the caller's user namespace: for an unprivileged caller the kernel
 * refuses it with EPERM, and the test fails.
 *
 * SANDBOX_RUN_WITH(body, arg, options) takes a sandbox_options for the
 * rest: more clone flags, such as CLONE_NEWNET, the id maps, and the time
 * the body may take.
 *
 * The body checks with the TEST_* macros, as a test does, and its failures
 * and skip count for the test that runs it. A body that ends the process
 * itself, with lc_kernel_exit or by an exec, gives its exit status as the
 * body would: 0 passed, 1 failed, 77 skipped. Any other status, a signal
 * that kills the child, an id map the kernel refuses, and a body still
 * running after options.seconds (SANDBOX_SECONDS by default) fail the test
 * with a line that says which. The test forks a keeper, which runs under a
 * setitimer watchdog as the other suites run their children: the keeper
 * makes the child, writes its maps, starts it and waits for it. When the
 * watchdog fires, the keeper kills the child, which takes every process of
 * the child's pid namespace with it, so a hung body does not hang the
 * suite.
 *
 * Where a user namespace cannot be made, as under qemu-user, whose clone
 * refuses the namespace flags, or where AppArmor's
 * kernel.apparmor_restrict_unprivileged_userns=1 gives one no capabilities,
 * the test prints SKIP: with the reason and is skipped. Once a test has
 * skipped, by that or by its body's TEST_SKIP, its later SANDBOX_RUNs
 * return false at once, so a test prints one SKIP: line, whatever number
 * of sandboxes it runs. sandbox_unavailable() gives that reason, or NULL
 * where sandboxes work. CI's qemu job counts those lines: a suite's count
 * in ci/qemu-skips.txt includes its tests that run a sandbox, and a suite
 * whose every test does must be in that job's LIGHTC_TEST_ALLOW_SKIP.
 *
 * SANDBOX_RUN returns true when the body ran and none of its checks failed,
 * so a test can go on to check from outside what the body did not change.
 */

#ifndef LIGHTC_TESTS_SANDBOX_H
#define LIGHTC_TESTS_SANDBOX_H

#include "test.h"
#include <lightc/format.h>
#include <lightc/result.h>
#include <lightc/signal.h>
#include <lightc/syscall.h>

/* --- The interface --- */

#define SANDBOX_DENY_SETGROUPS  0x1u
#define SANDBOX_NO_GID_MAP      0x2u

/* The seconds a body may run by default. */
#define SANDBOX_SECONDS         10

/* The most lines an id map of sandbox_options may have. */
#define SANDBOX_MAP_LINES       8

/* The size of the stack the body runs on. */
#define SANDBOX_STACK_SIZE      (1 << 20)

/* One line of an id map: count ids from inside, in the sandbox's user
 * namespace, are the ids from outside on, in the caller's. */
typedef struct {
    uint32_t inside;
    uint32_t outside;
    uint32_t count;
} sandbox_id_range;

typedef struct {
    uint32_t                flags;          /* SANDBOX_* */
    int64_t                 clone_flags;    /* more CLONE_* flags, or 0 */
    const sandbox_id_range *uids;           /* NULL: uid 0 is the caller's euid */
    size_t                  uid_count;
    const sandbox_id_range *gids;           /* NULL: gid 0 is the caller's egid */
    size_t                  gid_count;
    int64_t                 seconds;        /* 0: SANDBOX_SECONDS */
} sandbox_options;

#define SANDBOX_RUN(body, arg, flag_bits) \
    _sandbox_run((body), (arg), &(sandbox_options){ .flags = (flag_bits) }, __FILE__, __LINE__)

#define SANDBOX_RUN_WITH(body, arg, options) \
    _sandbox_run((body), (arg), (options), __FILE__, __LINE__)

/* --- Kernel values the fixture uses that <lightc/syscall.h> does not name --- */

#define SANDBOX_PR_SET_PDEATHSIG  1
#define SANDBOX_ITIMER_REAL       0
#define SANDBOX_CLONE_FLAGS       (CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS)
#define SANDBOX_CLONE_NAMES       "CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS"
#define SANDBOX_APPARMOR_SYSCTL   "/proc/sys/kernel/apparmor_restrict_unprivileged_userns"

/* --- Reading the process's ids --- */

/*
 * The four ids on the `key` line of /proc/self/status, "Uid:" or "Gid:":
 * real, effective, saved and filesystem, as the process's user namespace
 * sees them. False if the file or the line cannot be read.
 */
static inline bool sandbox_status_ids(const char *key, uint32_t ids[4]) {
    static char text[4096];
    lc_sysret fd = lc_kernel_open_file("/proc/self/status", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    lc_sysret n = lc_kernel_read_all((int32_t)fd, text, sizeof text - 1);
    lc_kernel_close_file((int32_t)fd);
    if (n <= 0) return false;
    size_t length = (size_t)n, key_length = lc_string_length(key);
    for (size_t at = 0; at + key_length <= length; ) {
        if ((at == 0 || text[at - 1] == '\n') &&
            lc_bytes_compare(text + at, key, key_length) == 0) {
            size_t i = at + key_length;
            for (int k = 0; k < 4; k++) {
                while (i < length && (text[i] == ' ' || text[i] == '\t')) i++;
                if (i >= length || text[i] < '0' || text[i] > '9') return false;
                uint64_t value = 0;
                while (i < length && text[i] >= '0' && text[i] <= '9' && value <= 0xffffffffu)
                    value = value * 10 + (uint64_t)(text[i++] - '0');
                if (value > 0xffffffffu) return false;
                ids[k] = (uint32_t)value;
            }
            return true;
        }
        while (at < length && text[at] != '\n') at++;
        at++;
    }
    return false;
}

/* The caller's effective uid and gid, which the default maps give the
 * sandbox's uid 0 and gid 0. False if /proc/self/status cannot be read. */
static inline bool sandbox_caller_ids(uint32_t *uid, uint32_t *gid) {
    uint32_t uids[4], gids[4];
    if (!sandbox_status_ids("Uid:", uids) || !sandbox_status_ids("Gid:", gids)) return false;
    *uid = uids[1];
    *gid = gids[1];
    return true;
}

/* --- Whether sandboxes work here --- */

static char        _sandbox_why[256];
static int         _sandbox_known = 0;    /* 0 not yet asked, 1 they work, -1 they do not */

static int _sandbox_probe_child(void *arg) {
    (void)arg;
    return 0;
}

/* NULL where sandboxes work, or why they do not, as SKIP: prints it. The
 * first call finds out, by making a child in the four namespaces, which
 * ends at once. */
static inline const char *sandbox_unavailable(void) {
    if (_sandbox_known != 0) return _sandbox_known > 0 ? NULL : _sandbox_why;
    lc_format f = lc_format_start(_sandbox_why, sizeof _sandbox_why);
    _sandbox_known = -1;

    uint32_t uid = 0, gid = 0;
    if (!sandbox_caller_ids(&uid, &gid)) {
        lc_format_add_text(&f, "cannot read the caller's ids from /proc/self/status");
        lc_format_finish(&f);
        return _sandbox_why;
    }
    lc_sysret fd = lc_kernel_open_file(SANDBOX_APPARMOR_SYSCTL, O_RDONLY | O_CLOEXEC, 0);
    if (fd >= 0) {
        char value = 0;
        lc_sysret n = lc_kernel_read_bytes((int32_t)fd, &value, 1);
        lc_kernel_close_file((int32_t)fd);
        if (n == 1 && value == '1' && uid != 0) {
            lc_format_add_text(&f, "kernel.apparmor_restrict_unprivileged_userns=1, so AppArmor "
                                   "gives a new user namespace no capabilities");
            lc_format_finish(&f);
            return _sandbox_why;
        }
    }

    static uint8_t stack[16384];   /* the child's copy of it, as it shares no memory */
    lc_sysret pid = lc_kernel_clone(SANDBOX_CLONE_FLAGS | SIGCHLD, stack + sizeof stack,
                                    _sandbox_probe_child, NULL);
    if (pid < 0) {
        lc_format_add_text(&f, "cannot make user namespaces here: clone(" SANDBOX_CLONE_NAMES ") = ");
        lc_format_add_signed(&f, pid);
        lc_format_add_text(&f, " (");
        lc_format_add_text(&f, lc_error_name((int32_t)-pid));
        lc_format_add_text(&f, ")");
        lc_format_finish(&f);
        return _sandbox_why;
    }
    int32_t status = -1;
    while (lc_kernel_wait_for_child((int32_t)pid, &status, 0) == -LC_ERR_INTR) {}
    if (status != 0) {
        lc_format_add_text(&f, "a child in new user, mount, pid and UTS namespaces ended with wait status ");
        lc_format_add_signed(&f, status);
        lc_format_finish(&f);
        return _sandbox_why;
    }
    _sandbox_known = 1;
    return NULL;
}

/* --- The keeper and the child --- */

/* What the keeper tells the test, through a page they share. */
enum {
    _SANDBOX_PASSED,        /* the body passed */
    _SANDBOX_FAILED,        /* the body failed and printed why */
    _SANDBOX_SKIPPED,       /* the body skipped and printed why */
    _SANDBOX_BROKEN,        /* the fixture failed, or the child ended otherwise: text says how */
};

typedef struct {
    int32_t outcome;
    char    text[500];
} _sandbox_report;

#define _SANDBOX_EXIT_FAILED    1
#define _SANDBOX_EXIT_SKIPPED   77
#define _SANDBOX_EXIT_ABANDONED 78      /* the keeper went away before the start */

static _sandbox_report *_sandbox_shared = NULL;

/* The keeper's, inherited by the child. */
static void           (*_sandbox_body)(void *);
static void            *_sandbox_arg;
static int32_t          _sandbox_go[2];
static volatile int32_t _sandbox_child_pid = 0;
static volatile int32_t _sandbox_timed_out = 0;

/* The keeper's watchdog: kill the child, so that the keeper's wait ends.
 * The init of a pid namespace, as the child is, ignores a signal it has no
 * handler for, unless it is SIGKILL or SIGSTOP from outside the namespace.
 * SIGKILL ends it and every process of the namespace. */
static void _sandbox_on_alarm(int signo) {
    (void)signo;
    _sandbox_timed_out = 1;
    if (_sandbox_child_pid > 0) (void)lc_kernel_send_signal(_sandbox_child_pid, SIGKILL);
}

/* The child: wait for the maps, then run the body. Its return value is the
 * child's exit status. */
static int _sandbox_child(void *unused) {
    (void)unused;
    lc_kernel_close_file(_sandbox_go[1]);
    /* If the keeper dies, so does the child; the read below sees a keeper
     * that died before this. */
    (void)lc_kernel_prctl(SANDBOX_PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0);
    (void)lc_kernel_reset_signal(LC_SIGALRM);
    char go = 0;
    if (lc_kernel_read_retry(_sandbox_go[0], &go, 1) != 1) return _SANDBOX_EXIT_ABANDONED;
    lc_kernel_close_file(_sandbox_go[0]);
    _test_current_failed = 0;
    _test_current_skipped = 0;
    _sandbox_body(_sandbox_arg);
    if (_test_current_failed > 0) return _SANDBOX_EXIT_FAILED;
    if (_test_current_skipped) return _SANDBOX_EXIT_SKIPPED;
    return 0;
}

/* Puts the lines of a map in `text`. False if they do not fit, or there
 * are too many. */
static inline bool _sandbox_map_text(char *text, size_t size, size_t *length,
                                     const sandbox_id_range *ranges, size_t count,
                                     uint32_t default_outside) {
    sandbox_id_range one = { 0, default_outside, 1 };
    if (ranges == NULL) {
        ranges = &one;
        count = 1;
    }
    if (count > SANDBOX_MAP_LINES) return false;
    lc_format f = lc_format_start(text, size);
    for (size_t i = 0; i < count; i++) {
        lc_format_add_unsigned(&f, ranges[i].inside);
        lc_format_add_char(&f, ' ');
        lc_format_add_unsigned(&f, ranges[i].outside);
        lc_format_add_char(&f, ' ');
        lc_format_add_unsigned(&f, ranges[i].count);
        lc_format_add_char(&f, '\n');
    }
    lc_format_finish(&f);
    *length = lc_format_length(&f);
    return !lc_format_has_overflow(&f);
}

/* Writes `length` bytes of `text` to /proc/<pid>/<name> in one write, as
 * the kernel takes an id map. 0, or a negative errno. */
static inline lc_sysret _sandbox_write_proc(int32_t pid, const char *name, const char *text, size_t length) {
    char path[64];
    lc_format f = lc_format_start(path, sizeof path);
    lc_format_add_text(&f, "/proc/");
    lc_format_add_signed(&f, pid);
    lc_format_add_char(&f, '/');
    lc_format_add_text(&f, name);
    lc_format_finish(&f);
    lc_sysret fd = lc_kernel_open_file(path, O_WRONLY | O_CLOEXEC, 0);
    if (fd < 0) return fd;
    lc_sysret n = lc_kernel_write_bytes((int32_t)fd, text, length);
    lc_kernel_close_file((int32_t)fd);
    if (n < 0) return n;
    return n == (lc_sysret)length ? 0 : -LC_ERR_IO;
}

/* Ends the report as broken, with the reason f holds, and the errno of ret
 * when it is one. */
static inline void _sandbox_broken(lc_format *f, lc_sysret ret) {
    if (ret < 0) {
        lc_format_add_text(f, ": ");
        lc_format_add_signed(f, ret);
        lc_format_add_text(f, " (");
        lc_format_add_text(f, lc_error_name((int32_t)-ret));
        lc_format_add_text(f, ")");
    }
    lc_format_finish(f);
    _sandbox_shared->outcome = _SANDBOX_BROKEN;
}

/* Adds the lines of a map to f, in quotes, with "; " between them. */
static inline void _sandbox_add_map(lc_format *f, const char *text, size_t length) {
    lc_format_add_text(f, " \"");
    for (size_t i = 0; i < length; i++) {
        if (text[i] != '\n') lc_format_add_char(f, text[i]);
        else if (i + 1 < length) lc_format_add_text(f, "; ");
    }
    lc_format_add_char(f, '"');
}

/* Writes the child's maps, or stops at the first that fails and says
 * which. True when they are all written. */
static inline bool _sandbox_write_maps(int32_t pid, const sandbox_options *options,
                                       uint32_t uid, uint32_t gid, lc_format *f) {
    static char text[SANDBOX_MAP_LINES * 36 + 1];
    size_t length = 0;
    if (!_sandbox_map_text(text, sizeof text, &length, options->uids, options->uid_count, uid)) {
        lc_format_add_text(f, "the uid map has more lines than SANDBOX_MAP_LINES");
        _sandbox_broken(f, 0);
        return false;
    }
    lc_sysret ret = _sandbox_write_proc(pid, "uid_map", text, length);
    if (ret < 0) {
        lc_format_add_text(f, "the kernel refused the uid map");
        _sandbox_add_map(f, text, length);
        _sandbox_broken(f, ret);
        return false;
    }
    if (options->flags & SANDBOX_DENY_SETGROUPS) {
        ret = _sandbox_write_proc(pid, "setgroups", "deny", 4);
        if (ret < 0) {
            lc_format_add_text(f, "cannot write deny to setgroups");
            _sandbox_broken(f, ret);
            return false;
        }
    }
    if (options->flags & SANDBOX_NO_GID_MAP) return true;
    if (!_sandbox_map_text(text, sizeof text, &length, options->gids, options->gid_count, gid)) {
        lc_format_add_text(f, "the gid map has more lines than SANDBOX_MAP_LINES");
        _sandbox_broken(f, 0);
        return false;
    }
    ret = _sandbox_write_proc(pid, "gid_map", text, length);
    if (ret < 0) {
        lc_format_add_text(f, "the kernel refused the gid map");
        _sandbox_add_map(f, text, length);
        if (ret == -LC_ERR_PERM && !(options->flags & SANDBOX_DENY_SETGROUPS))
            lc_format_add_text(f, ", which an unprivileged caller may write only after "
                                  "SANDBOX_DENY_SETGROUPS");
        _sandbox_broken(f, ret);
        return false;
    }
    return true;
}

/* The keeper, a forked child of the test: makes the sandbox's child under
 * the watchdog, maps it, starts it and reports how it ended. */
static inline void _sandbox_keep(void (*body)(void *), void *arg, const sandbox_options *options) {
    _sandbox_report *report = _sandbox_shared;
    lc_format f = lc_format_start(report->text, sizeof report->text);
    int64_t seconds = options->seconds > 0 ? options->seconds : SANDBOX_SECONDS;

    if (lc_signal_handle(LC_SIGALRM, _sandbox_on_alarm).error != 0 ||
        lc_signal_unblock(LC_SIGALRM).error != 0) {
        lc_format_add_text(&f, "cannot handle SIGALRM for the watchdog");
        _sandbox_broken(&f, 0);
        return;
    }
    /* A test that ignores SIGCHLD would have its children reaped unseen. */
    (void)lc_kernel_reset_signal(SIGCHLD);
    int64_t watchdog[4] = { 0, 0, seconds, 0 };   /* struct itimerval, once */
    lc_sysret ret = lc_syscall3(SYS_setitimer, SANDBOX_ITIMER_REAL, (int64_t)watchdog, 0);
    if (ret < 0) {
        lc_format_add_text(&f, "cannot start the watchdog");
        _sandbox_broken(&f, ret);
        return;
    }
    uint32_t uid = 0, gid = 0;
    if (!sandbox_caller_ids(&uid, &gid)) {
        lc_format_add_text(&f, "cannot read the caller's ids from /proc/self/status");
        _sandbox_broken(&f, 0);
        return;
    }
    ret = lc_kernel_create_pipe(_sandbox_go, O_CLOEXEC);
    if (ret < 0) {
        lc_format_add_text(&f, "cannot make a pipe");
        _sandbox_broken(&f, ret);
        return;
    }
    size_t stack_size = SANDBOX_STACK_SIZE;
    uint8_t *stack = lc_kernel_map_memory(NULL, stack_size, PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED) {
        lc_format_add_text(&f, "cannot map the child's stack");
        _sandbox_broken(&f, 0);
        return;
    }

    _sandbox_body = body;
    _sandbox_arg = arg;
    lc_sysret pid = lc_kernel_clone(SANDBOX_CLONE_FLAGS | options->clone_flags | SIGCHLD,
                                    stack + stack_size, _sandbox_child, NULL);
    if (pid < 0) {
        lc_format_add_text(&f, "clone(" SANDBOX_CLONE_NAMES);
        if (options->clone_flags != 0) {
            lc_format_add_text(&f, " | ");
            lc_format_add_hex(&f, (uint64_t)options->clone_flags);
        }
        lc_format_add_text(&f, ") failed");
        _sandbox_broken(&f, pid);
        return;
    }
    _sandbox_child_pid = (int32_t)pid;
    if (_sandbox_timed_out) (void)lc_kernel_send_signal((int32_t)pid, SIGKILL);
    lc_kernel_close_file(_sandbox_go[0]);

    int32_t status = 0;
    if (!_sandbox_write_maps((int32_t)pid, options, uid, gid, &f)) {
        (void)lc_kernel_send_signal((int32_t)pid, SIGKILL);
        while (lc_kernel_wait_for_child((int32_t)pid, &status, 0) == -LC_ERR_INTR) {}
        return;
    }
    (void)lc_kernel_write_all(_sandbox_go[1], "g", 1);
    lc_kernel_close_file(_sandbox_go[1]);

    while ((ret = lc_kernel_wait_for_child((int32_t)pid, &status, 0)) == -LC_ERR_INTR) {}
    if (ret < 0) {
        lc_format_add_text(&f, "cannot wait for the child");
        _sandbox_broken(&f, ret);
        return;
    }
    int32_t signal = status & 0x7f, code = (status >> 8) & 0xff;
    if (_sandbox_timed_out) {
        lc_format_add_text(&f, "the body was still running after ");
        lc_format_add_signed(&f, seconds);
        lc_format_add_text(&f, " s, so it was killed");
        _sandbox_broken(&f, 0);
    } else if (signal != 0) {
        lc_format_add_text(&f, "the body's process died of signal ");
        lc_format_add_signed(&f, signal);
        _sandbox_broken(&f, 0);
    } else if (code == 0) {
        report->outcome = _SANDBOX_PASSED;
    } else if (code == _SANDBOX_EXIT_FAILED) {
        report->outcome = _SANDBOX_FAILED;
    } else if (code == _SANDBOX_EXIT_SKIPPED) {
        report->outcome = _SANDBOX_SKIPPED;
    } else {
        lc_format_add_text(&f, "the body's process exited with status ");
        lc_format_add_signed(&f, code);
        _sandbox_broken(&f, 0);
    }
}

/* SANDBOX_RUN and SANDBOX_RUN_WITH. */
static inline bool _sandbox_run(void (*body)(void *), void *arg, const sandbox_options *options,
                                const char *file, int line) {
    if (_test_current_skipped) return false;    /* a sandbox of this test skipped already */
    const char *why = sandbox_unavailable();
    if (why != NULL) {
        _test_skip(why);
        return false;
    }
    if (_sandbox_shared == NULL) {
        void *page = lc_kernel_map_memory(NULL, 4096, PROT_READ | PROT_WRITE,
                                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (page == MAP_FAILED) {
            _test_fail(file, line, "SANDBOX_RUN: cannot map the page the keeper reports on");
            return false;
        }
        _sandbox_shared = page;
    }
    _sandbox_shared->outcome = _SANDBOX_BROKEN;
    static const char unreported[] = "the keeper ended before it reported";
    lc_bytes_copy(_sandbox_shared->text, unreported, sizeof unreported);

    lc_sysret keeper = lc_kernel_fork();
    if (keeper == 0) {
        _sandbox_keep(body, arg, options);
        lc_kernel_exit(0);
    }
    if (keeper < 0) {
        _test_fail(file, line, "SANDBOX_RUN: cannot fork the keeper");
        return false;
    }
    int32_t status = 0;
    while (lc_kernel_wait_for_child((int32_t)keeper, &status, 0) == -LC_ERR_INTR) {}

    switch (_sandbox_shared->outcome) {
    case _SANDBOX_PASSED:
        return true;
    case _SANDBOX_FAILED:
        _test_current_failed++;
        return false;
    case _SANDBOX_SKIPPED:
        _test_current_skipped = 1;
        return false;
    default:
        _test_fail_start(file, line, "SANDBOX_RUN: ");
        _test_print_text(_sandbox_shared->text);
        if (status != 0) {
            _test_print_text(" (the keeper's wait status ");
            lc_print_signed(TEST_FD, status);
            _test_print_text(")");
        }
        lc_print_newline(TEST_FD);
        return false;
    }
}

#endif /* LIGHTC_TESTS_SANDBOX_H */
