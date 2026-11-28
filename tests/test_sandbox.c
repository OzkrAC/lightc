/*
 * test_sandbox.c — the container calls of <lightc/syscall.h> as an
 * unprivileged user makes them: tmpfs, bind and propagation mounts,
 * umount2, pivot_root, sethostname, mknodat and a privilege drop.
 *
 * Each test runs its body in a sandbox of tests/sandbox.h: new user,
 * mount, pid and UTS namespaces, which need no privilege and keep what the
 * body changes from the machine. The first tests check the sandbox itself.
 * The bodies work in SCRATCH, mostly on a tmpfs they mount there first, so
 * what they find does not depend on how the machine mounted build/. Where
 * a user namespace cannot be made, as under qemu-user, each test prints
 * SKIP: with the reason, so there the whole suite skips.
 */

#include "test.h"
#include "sandbox.h"
#include <lightc/heap.h>
#include <lightc/io.h>
#include <lightc/syscall.h>
#include <lightc/time.h>
#include <stddef.h>

#define SCRATCH         "build/test_sandbox_scratch"
#define SCRATCH_MARKER  SCRATCH "/marker"

/* --- Kernel values <lightc/syscall.h> does not name, from <linux/stat.h>,
 *     <linux/mount.h> and <linux/kdev_t.h> --- */

#define SB_S_IFMT        0170000
#define SB_S_IFDIR       0040000
#define SB_S_IFIFO       0010000
#define SB_S_IFCHR       0020000
#define SB_S_IFBLK       0060000
#define SB_MS_SHARED     (1 << 20)
#define SB_STATX_TYPE    0x0001u
#define SB_STATX_MODE    0x0002u
#define SB_STATX_INO     0x0100u
#define SB_STATX_MNT_ID  0x1000u
/* A device number as mknodat takes it (new_encode_dev), for a major below
 * 4096 and a minor below 256. */
#define SB_DEV(major, minor) ((uint64_t)(((major) << 8) | (minor)))

/* struct statx, all 256 bytes, which the kernel fills. */
typedef struct {
    uint32_t mask;
    uint32_t blksize;
    uint64_t attributes;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint16_t mode;
    uint16_t _spare0;
    uint64_t ino;
    uint64_t size;
    uint64_t blocks;
    uint64_t attributes_mask;
    uint8_t  times[64];          /* stx_atime, stx_btime, stx_ctime, stx_mtime */
    uint32_t rdev_major;
    uint32_t rdev_minor;
    uint32_t dev_major;
    uint32_t dev_minor;
    uint64_t mnt_id;
    uint8_t  _rest[104];
} sb_statx;
_Static_assert(sizeof(sb_statx) == 256, "sb_statx must be 256 bytes (struct statx)");
_Static_assert(offsetof(sb_statx, ino) == 32, "stx_ino is at 32");
_Static_assert(offsetof(sb_statx, rdev_major) == 128, "stx_rdev_major is at 128");
_Static_assert(offsetof(sb_statx, dev_major) == 136, "stx_dev_major is at 136");
_Static_assert(offsetof(sb_statx, mnt_id) == 144, "stx_mnt_id is at 144");

/* --- Helpers --- */

/* statx of path, not following a last symlink. 0 or a negative errno. */
static lc_sysret file_stat(const char *path, sb_statx *st) {
    lc_bytes_fill(st, 0, sizeof *st);
    return lc_syscall5(SYS_statx, AT_FDCWD, (int64_t)path, AT_SYMLINK_NOFOLLOW,
                       SB_STATX_TYPE | SB_STATX_MODE | SB_STATX_INO | SB_STATX_MNT_ID, (int64_t)st);
}

/* Whether a and b are the same file: device and inode. */
static bool same_file(const sb_statx *a, const sb_statx *b) {
    return a->dev_major == b->dev_major && a->dev_minor == b->dev_minor && a->ino == b->ino;
}

/* Whether path exists, as a file of any type. */
static bool exists(const char *path) {
    sb_statx st;
    return file_stat(path, &st) == 0;
}

/* Reads up to size - 1 bytes of path into buf and ends them with a NUL.
 * The byte count, or a negative errno. */
static lc_sysret read_text(const char *path, char *buf, size_t size) {
    lc_sysret fd = lc_kernel_open_file(path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return fd;
    lc_sysret n = lc_kernel_read_all((int32_t)fd, buf, size - 1);
    lc_kernel_close_file((int32_t)fd);
    buf[n > 0 ? n : 0] = '\0';
    return n;
}

/* Whether path holds exactly `text`. */
static bool holds(const char *path, const char *text) {
    char buf[256];
    lc_sysret n = read_text(path, buf, sizeof buf);
    return n >= 0 && lc_string_equal(buf, (size_t)n, text, lc_string_length(text));
}

/* Writes `text` to a new file at path, mode 0600. 0 or a negative errno. */
static lc_sysret make_file(const char *path, const char *text) {
    lc_sysret fd = lc_kernel_open_file(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0) return fd;
    lc_sysret n = lc_kernel_write_all((int32_t)fd, text, lc_string_length(text));
    lc_kernel_close_file((int32_t)fd);
    return n < 0 ? n : 0;
}

/* Reads the numbers of an id map file, such as /proc/self/uid_map, three
 * per line. How many it read, or -1 if the file cannot be read. */
static int32_t read_numbers(const char *path, uint32_t *numbers, int32_t max) {
    char buf[512];
    lc_sysret n = read_text(path, buf, sizeof buf);
    if (n < 0) return -1;
    int32_t count = 0;
    for (lc_sysret i = 0; i < n && count < max; ) {
        if (buf[i] < '0' || buf[i] > '9') {
            i++;
            continue;
        }
        uint64_t value = 0;
        while (i < n && buf[i] >= '0' && buf[i] <= '9') value = value * 10 + (uint64_t)(buf[i++] - '0');
        numbers[count++] = (uint32_t)value;
    }
    return count;
}

/* The host name, from /proc/sys/kernel/hostname, which shows the caller's
 * UTS namespace's, without its newline. Its length, or a negative errno. */
static lc_sysret host_name(char *buf, size_t size) {
    lc_sysret n = read_text("/proc/sys/kernel/hostname", buf, size);
    if (n > 0 && buf[n - 1] == '\n') buf[--n] = '\0';
    return n;
}

/* Mounts a tmpfs on SCRATCH and makes its root the current directory, so
 * the body's paths are on a filesystem of its own. False, with a failed
 * check, if it could not. */
static bool scratch(void) {
    lc_sysret ret = lc_kernel_mount("tmpfs", SCRATCH, "tmpfs", 0, "mode=0755");
    TEST_ASSERT_EQ(ret, 0);
    if (ret != 0) return false;
    ret = lc_kernel_chdir(SCRATCH);
    TEST_ASSERT_EQ(ret, 0);
    return ret == 0;
}

/* The line of /proc/self/mountinfo of the mount that path is on, by its
 * mount id, copied into line. False if there is none. */
static bool mount_line(const char *path, char *line, size_t size) {
    sb_statx st;
    if (file_stat(path, &st) != 0 || !(st.mask & SB_STATX_MNT_ID)) return false;
    uint8_t *text = NULL;
    size_t length = 0;
    if (lc_file_read_all("/proc/self/mountinfo", &text, &length).error != 0) return false;
    char id[24];
    lc_format f = lc_format_start(id, sizeof id);
    lc_format_add_unsigned(&f, st.mnt_id);
    lc_format_add_char(&f, ' ');
    lc_format_finish(&f);
    size_t id_length = lc_format_length(&f);
    bool found = false;
    for (size_t at = 0; at < length && !found; ) {
        size_t end = at;
        while (end < length && text[end] != '\n') end++;
        if (end - at > id_length && lc_bytes_compare(text + at, id, id_length) == 0) {
            size_t n = end - at < size - 1 ? end - at : size - 1;
            lc_bytes_copy(line, text + at, n);
            line[n] = '\0';
            found = true;
        }
        at = end + 1;
    }
    lc_heap_free(text);
    return found;
}

/* The propagation of the mount that path is on, as its line of
 * /proc/self/mountinfo gives it in its optional fields: "shared:N",
 * "master:N", or "" for a private mount. Copied into out; "?" if the
 * line cannot be found. */
static const char *propagation(const char *path, char *out, size_t size) {
    static char line[1024];
    out[0] = '?';
    out[1] = '\0';
    if (!mount_line(path, line, sizeof line)) return out;
    size_t length = lc_string_length(line);
    int64_t dash = lc_string_find_substring(line, length, " - ", 3);
    if (dash < 0) return out;
    /* The optional fields start after the sixth field, the mount options. */
    size_t at = 0;
    for (int field = 0; field < 6 && at < (size_t)dash; field++) {
        while (at < (size_t)dash && line[at] != ' ') at++;
        while (at < (size_t)dash && line[at] == ' ') at++;
    }
    size_t n = (size_t)dash - at < size - 1 ? (size_t)dash - at : size - 1;
    lc_bytes_copy(out, line + at, n);
    out[n] = '\0';
    return out;
}

/* The line of /proc/self/status that starts with `key`, such as "Groups:",
 * without its newline, copied into out. False if there is none. */
static bool status_line(const char *key, char *out, size_t size) {
    static char text[8192];
    lc_sysret n = read_text("/proc/self/status", text, sizeof text);
    if (n <= 0) return false;
    size_t length = (size_t)n, key_length = lc_string_length(key);
    for (size_t at = 0; at < length; ) {
        size_t end = at;
        while (end < length && text[end] != '\n') end++;
        if (end - at >= key_length && lc_bytes_compare(text + at, key, key_length) == 0) {
            size_t copy = end - at < size - 1 ? end - at : size - 1;
            lc_bytes_copy(out, text + at, copy);
            out[copy] = '\0';
            return true;
        }
        at = end + 1;
    }
    return false;
}

static bool starts_with(const char *text, const char *prefix) {
    return lc_string_starts_with(text, lc_string_length(text), prefix, lc_string_length(prefix));
}

/* What the tests set before a sandbox runs, which its body reads. */
static uint32_t caller_uid, caller_gid;
static char     caller_host[128];
static char     caller_ns[4][64];
static const char *const ns_links[4] = {
    "/proc/self/ns/user", "/proc/self/ns/mnt", "/proc/self/ns/pid", "/proc/self/ns/uts",
};

static bool note_caller(void) {
    if (!sandbox_caller_ids(&caller_uid, &caller_gid)) return false;
    if (host_name(caller_host, sizeof caller_host) < 0) return false;
    for (int i = 0; i < 4; i++) {
        lc_sysret n = lc_kernel_readlinkat(AT_FDCWD, ns_links[i], caller_ns[i], sizeof caller_ns[i] - 1);
        if (n < 0) return false;
        caller_ns[i][n] = '\0';
    }
    return true;
}

/* ===== The sandbox =====
 *
 * The body is pid 1 of a new pid namespace and uid 0 and gid 0 of a new
 * user namespace, which the default maps give the caller's euid and egid.
 * Its user, mount, pid and UTS namespaces are not the caller's, and its
 * host name starts as the caller's. SANDBOX_DENY_SETGROUPS wrote "deny" to
 * its setgroups, so setgroups fails with EPERM.
 */
static void namespaces_body(void *arg) {
    (void)arg;
    TEST_ASSERT_EQ(lc_kernel_get_process_id(), 1);
    TEST_ASSERT_EQ(lc_kernel_getuid(), 0u);
    uint32_t ids[4];
    TEST_ASSERT(sandbox_status_ids("Gid:", ids));
    TEST_ASSERT_EQ(ids[1], 0u);

    uint32_t map[6];
    TEST_ASSERT_EQ(read_numbers("/proc/self/uid_map", map, 6), 3);
    TEST_ASSERT_EQ(map[0], 0u);
    TEST_ASSERT_EQ(map[1], caller_uid);
    TEST_ASSERT_EQ(map[2], 1u);
    TEST_ASSERT_EQ(read_numbers("/proc/self/gid_map", map, 6), 3);
    TEST_ASSERT_EQ(map[0], 0u);
    TEST_ASSERT_EQ(map[1], caller_gid);
    TEST_ASSERT_EQ(map[2], 1u);
    TEST_ASSERT(holds("/proc/self/setgroups", "deny\n"));
    TEST_ASSERT_EQ(lc_kernel_setgroups(0, NULL), -LC_ERR_PERM);

    for (int i = 0; i < 4; i++) {
        char link[64];
        lc_sysret n = lc_kernel_readlinkat(AT_FDCWD, ns_links[i], link, sizeof link - 1);
        TEST_ASSERT(n > 0);
        if (n <= 0) continue;
        link[n] = '\0';
        _test_print_text("  ");
        _test_print_text(ns_links[i]);
        _test_print_text(": ");
        _test_print_text(caller_ns[i]);
        _test_print_text(" outside, ");
        _test_print_text(link);
        _test_print_text(" inside\n");
        TEST_ASSERT(!lc_string_equal(link, (size_t)n, caller_ns[i], lc_string_length(caller_ns[i])));
    }

    char host[128];
    lc_sysret n = host_name(host, sizeof host);
    TEST_ASSERT(n >= 0);
    TEST_ASSERT_STR_EQ(host, (size_t)(n > 0 ? n : 0), caller_host, lc_string_length(caller_host));
}

static void test_sandbox_namespaces(void) {
    TEST_REQUIRE(note_caller());
    SANDBOX_RUN(namespaces_body, NULL, SANDBOX_DENY_SETGROUPS);
}

/* Without SANDBOX_DENY_SETGROUPS and with SANDBOX_NO_GID_MAP, setgroups is
 * allowed but no gid is mapped: the body's gids are the overflow gid, and
 * setgroups and setresgid to gid 0 fail. */
static void no_gid_map_body(void *arg) {
    (void)arg;
    TEST_ASSERT(holds("/proc/self/setgroups", "allow\n"));
    uint32_t map[3];
    TEST_ASSERT_EQ(read_numbers("/proc/self/gid_map", map, 3), 0);
    char overflow[16];
    lc_sysret n = read_text("/proc/sys/kernel/overflowgid", overflow, sizeof overflow);
    TEST_ASSERT(n > 0);
    uint32_t ids[4];
    TEST_ASSERT(sandbox_status_ids("Gid:", ids));
    uint32_t expected = 0;
    for (lc_sysret i = 0; i < n && overflow[i] >= '0' && overflow[i] <= '9'; i++)
        expected = expected * 10 + (uint32_t)(overflow[i] - '0');
    TEST_ASSERT_EQ(ids[1], expected);
    TEST_ASSERT_EQ(lc_kernel_setgroups(0, NULL), -LC_ERR_PERM);
    TEST_ASSERT_EQ(lc_kernel_setresgid(0, 0, 0), -LC_ERR_INVAL);
}

static void test_sandbox_without_gid_map(void) {
    SANDBOX_RUN(no_gid_map_body, NULL, SANDBOX_NO_GID_MAP);
}

/* Maps of the test's choosing: uid 7 and gid 7 are the caller's. The body
 * has every capability of its namespace all the same, as the first
 * process of a user namespace does, so it can set the host name. */
static void chosen_ids_body(void *arg) {
    (void)arg;
    TEST_ASSERT_EQ(lc_kernel_getuid(), 7u);
    uint32_t ids[4];
    TEST_ASSERT(sandbox_status_ids("Gid:", ids));
    TEST_ASSERT_EQ(ids[1], 7u);
    uint32_t map[3];
    TEST_ASSERT_EQ(read_numbers("/proc/self/uid_map", map, 3), 3);
    TEST_ASSERT_EQ(map[0], 7u);
    TEST_ASSERT_EQ(map[1], caller_uid);
    TEST_ASSERT_EQ(lc_kernel_sethostname("seven", 5), 0);
}

static void test_sandbox_chosen_ids(void) {
    TEST_REQUIRE(sandbox_caller_ids(&caller_uid, &caller_gid));
    sandbox_id_range uids[1] = { { 7, caller_uid, 1 } };
    sandbox_id_range gids[1] = { { 7, caller_gid, 1 } };
    sandbox_options options = {
        .flags = SANDBOX_DENY_SETGROUPS, .uids = uids, .uid_count = 1, .gids = gids, .gid_count = 1,
    };
    SANDBOX_RUN_WITH(chosen_ids_body, NULL, &options);
}

/* ===== How the sandbox reports =====
 *
 * Each demo runs SANDBOX_RUN in a forked child whose stdout is a pipe, as
 * test_harness runs its demos, so their failures and skips are checked
 * here instead of counted. The child's exit status says what the demo's
 * test would have counted: 1 if it failed, 2 if it skipped, and 4 if
 * SANDBOX_RUN returned true.
 */
#define DEMO_OUTPUT_MAX 4096

static char    demo_text[DEMO_OUTPUT_MAX];
static size_t  demo_length;

static void failing_body(void *arg)  { (void)arg; TEST_ASSERT_EQ(1, 2); }
static void skipping_body(void *arg) { (void)arg; TEST_SKIP("the demo skips"); }
static void passing_body(void *arg)  { (void)arg; TEST_ASSERT_EQ(lc_kernel_get_process_id(), 1); }
static void exiting_body(void *arg)  { (void)arg; lc_kernel_exit(3); }
static void hanging_body(void *arg) {
    (void)arg;
    lc_timespec second = { 1, 0 };
    for (;;) (void)lc_kernel_sleep(&second);
}

typedef struct {
    void (*body)(void *);
    sandbox_options options;
} demo;

/* Runs d's body in a sandbox in a forked child, and returns the child's
 * exit status, or -1. */
static int32_t run_demo(const demo *d) {
    demo_length = 0;
    int32_t fds[2];
    if (lc_kernel_create_pipe(fds, O_CLOEXEC) < 0) return -1;
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        lc_kernel_close_file(fds[0]);
        if (lc_kernel_duplicate_fd(fds[1], TEST_FD, 0) < 0) lc_kernel_exit(127);
        _test_current_failed = 0;
        _test_current_skipped = 0;
        bool passed = SANDBOX_RUN_WITH(d->body, NULL, &d->options);
        lc_kernel_exit((_test_current_failed > 0 ? 1 : 0) | (_test_current_skipped ? 2 : 0) | (passed ? 4 : 0));
    }
    lc_kernel_close_file(fds[1]);
    if (pid < 0) {
        lc_kernel_close_file(fds[0]);
        return -1;
    }
    char spill[256];
    for (;;) {
        size_t room = DEMO_OUTPUT_MAX - 1 - demo_length;
        lc_sysret n = room > 0 ? lc_kernel_read_retry(fds[0], demo_text + demo_length, room)
                               : lc_kernel_read_retry(fds[0], spill, sizeof spill);
        if (n <= 0) break;
        if (room > 0) demo_length += (size_t)n;
    }
    demo_text[demo_length] = '\0';
    lc_kernel_close_file(fds[0]);
    int32_t status = -1;
    if (lc_kernel_wait_for_child((int32_t)pid, &status, 0) != pid) return -1;
    return (status & 0x7f) ? -1 : (status >> 8) & 0xff;
}

static bool demo_printed(const char *text) {
    return lc_string_contains(demo_text, demo_length, text, lc_string_length(text));
}

/* Prints the demo's output behind "  | ", after a check of it failed. */
static void show_demo(void) {
    size_t at = 0;
    while (at < demo_length) {
        size_t end = at;
        while (end < demo_length && demo_text[end] != '\n') end++;
        _test_print_text("  | ");
        lc_print_string(TEST_FD, demo_text + at, end - at);
        lc_print_newline(TEST_FD);
        at = end + 1;
    }
}

#define CHECK_DEMO(cond) do { TEST_ASSERT(cond); if (!(cond)) show_demo(); } while (0)

static void test_sandbox_reports(void) {
    const char *why = sandbox_unavailable();
    if (why != NULL) TEST_SKIP(why);
    TEST_REQUIRE(sandbox_caller_ids(&caller_uid, &caller_gid));

    demo passing = { passing_body, { .flags = SANDBOX_DENY_SETGROUPS } };
    int32_t code = run_demo(&passing);
    CHECK_DEMO(code == 4 && demo_length == 0);

    /* The body's failure prints its line and fails the test. */
    demo failing = { failing_body, { .flags = SANDBOX_DENY_SETGROUPS } };
    code = run_demo(&failing);
    CHECK_DEMO(code == 1 && demo_printed("FAIL: 1 == 2 (1 vs 2)"));

    /* Its skip prints one SKIP: line and skips the test. */
    demo skipping = { skipping_body, { .flags = SANDBOX_DENY_SETGROUPS } };
    code = run_demo(&skipping);
    CHECK_DEMO(code == 2 && demo_printed("SKIP: the demo skips") && !demo_printed("FAIL"));

    /* An exit of its own with another status fails the test. */
    demo exiting = { exiting_body, { .flags = SANDBOX_DENY_SETGROUPS } };
    code = run_demo(&exiting);
    CHECK_DEMO(code == 1 && demo_printed("SANDBOX_RUN: the body's process exited with status 3"));

    /* A map the kernel refuses fails the test before the body runs: two
     * lines of one inside id overlap, which nobody may map. */
    sandbox_id_range overlapping[2] = { { 0, caller_uid, 1 }, { 0, caller_uid + 1, 1 } };
    demo refused = { passing_body, { .flags = SANDBOX_DENY_SETGROUPS, .uids = overlapping, .uid_count = 2 } };
    code = run_demo(&refused);
    CHECK_DEMO(code == 1 && demo_printed("SANDBOX_RUN: the kernel refused the uid map") &&
               demo_printed("(EINVAL)"));

    /* A body that hangs is killed when its watchdog fires, with every
     * process of its pid namespace, and the test fails. */
    int64_t start = lc_time_start_timer();
    demo hanging = { hanging_body, { .flags = SANDBOX_DENY_SETGROUPS, .seconds = 1 } };
    code = run_demo(&hanging);
    int64_t took = lc_time_elapsed_milliseconds(start);
    CHECK_DEMO(code == 1 && demo_printed("SANDBOX_RUN: the body was still running after 1 s, so it was killed"));
    TEST_ASSERT(took >= 900 && took < 5000);
}

/* ===== lc_kernel_mount: tmpfs =====
 *
 * A tmpfs mounted on SCRATCH hides the marker the test made there, takes
 * the mode its data string gives and the flags it is passed, and holds
 * what the body writes. Outside, after the sandbox, the marker is back and
 * nothing the body wrote is there. A read-only tmpfs refuses a new file.
 */
static void tmpfs_body(void *arg) {
    (void)arg;
    TEST_ASSERT(exists(SCRATCH_MARKER));
    TEST_REQUIRE_EQ(lc_kernel_mount("tmpfs", SCRATCH, "tmpfs", MS_NOSUID | MS_NODEV, "size=1m,mode=0750"), 0);
    TEST_ASSERT(!exists(SCRATCH_MARKER));
    sb_statx st;
    TEST_ASSERT_EQ(file_stat(SCRATCH, &st), 0);
    TEST_ASSERT_EQ(st.mode & SB_S_IFMT, SB_S_IFDIR);
    TEST_ASSERT_EQ(st.mode & 07777, 0750);

    char line[1024];
    TEST_ASSERT(mount_line(SCRATCH, line, sizeof line));
    size_t length = lc_string_length(line);
    TEST_ASSERT(lc_string_contains(line, length, " - tmpfs ", 9));
    TEST_ASSERT(lc_string_contains(line, length, ",nosuid,nodev", 13));

    TEST_ASSERT_EQ(make_file(SCRATCH "/inside", "written in the sandbox"), 0);
    TEST_ASSERT(holds(SCRATCH "/inside", "written in the sandbox"));

    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, SCRATCH "/ro", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mount("tmpfs", SCRATCH "/ro", "tmpfs", MS_RDONLY, NULL), 0);
    TEST_ASSERT_EQ(make_file(SCRATCH "/ro/file", "x"), -LC_ERR_ROFS);
}

static void test_mount_tmpfs(void) {
    TEST_REQUIRE_EQ(make_file(SCRATCH_MARKER, "made outside"), 0);
    if (SANDBOX_RUN(tmpfs_body, NULL, SANDBOX_DENY_SETGROUPS)) {
        TEST_ASSERT(holds(SCRATCH_MARKER, "made outside"));
        TEST_ASSERT(!exists(SCRATCH "/inside"));
        TEST_ASSERT(!exists(SCRATCH "/ro"));
    }
    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, SCRATCH_MARKER, 0), 0);
}

/* ===== lc_kernel_mount: bind mounts =====
 *
 * A bind mount shows the same files at a second place, both ways. Remounted
 * read-only, the bind refuses a write that the place it came from takes.
 * MS_BIND alone leaves out the mounts below the source; MS_BIND | MS_REC
 * brings them.
 */
static void bind_body(void *arg) {
    (void)arg;
    TEST_REQUIRE(scratch());
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "src", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "dst", 0755), 0);
    TEST_ASSERT_EQ(make_file("src/file", "source"), 0);

    TEST_REQUIRE_EQ(lc_kernel_mount("src", "dst", NULL, MS_BIND, NULL), 0);
    TEST_ASSERT(holds("dst/file", "source"));
    sb_statx a, b;
    TEST_ASSERT_EQ(file_stat("src/file", &a), 0);
    TEST_ASSERT_EQ(file_stat("dst/file", &b), 0);
    TEST_ASSERT(same_file(&a, &b));
    TEST_ASSERT_EQ(make_file("dst/back", "through the bind"), 0);
    TEST_ASSERT(holds("src/back", "through the bind"));

    TEST_ASSERT_EQ(lc_kernel_mount(NULL, "dst", NULL, MS_REMOUNT | MS_BIND | MS_RDONLY, NULL), 0);
    TEST_ASSERT_EQ(make_file("dst/refused", "x"), -LC_ERR_ROFS);
    TEST_ASSERT_EQ(make_file("src/taken", "x"), 0);
    TEST_ASSERT(exists("dst/taken"));

    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "src/sub", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mount("tmpfs", "src/sub", "tmpfs", 0, NULL), 0);
    TEST_ASSERT_EQ(make_file("src/sub/below", "below"), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "flat", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "deep", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mount("src", "flat", NULL, MS_BIND, NULL), 0);
    TEST_ASSERT_EQ(lc_kernel_mount("src", "deep", NULL, MS_BIND | MS_REC, NULL), 0);
    TEST_ASSERT(exists("flat/sub"));
    TEST_ASSERT(!exists("flat/sub/below"));
    TEST_ASSERT(holds("deep/sub/below", "below"));
}

static void test_mount_bind(void) {
    SANDBOX_RUN(bind_body, NULL, SANDBOX_DENY_SETGROUPS);
}

/* ===== lc_kernel_mount: propagation, MS_PRIVATE among it =====
 *
 * a is a tmpfs made shared; b and c are bind mounts of it, so they join
 * its peer group. Then c is made a slave and b private. A mount made below
 * a then shows below c, and not below b; one made below c does not show
 * below a. /proc/self/mountinfo shows each mount's propagation. Last,
 * MS_REC | MS_PRIVATE on /, as a container's setup makes it, leaves no
 * mount of the namespace shared or a slave.
 */
static void propagation_body(void *arg) {
    (void)arg;
    TEST_REQUIRE(scratch());
    char tag[64], tag_a[64];
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "a", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "b", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "c", 0755), 0);
    TEST_REQUIRE_EQ(lc_kernel_mount("tmpfs", "a", "tmpfs", 0, NULL), 0);
    propagation("a", tag, sizeof tag);
    TEST_ASSERT_STR_EQ(tag, lc_string_length(tag), "", 0);

    TEST_ASSERT_EQ(lc_kernel_mount(NULL, "a", NULL, SB_MS_SHARED, NULL), 0);
    propagation("a", tag_a, sizeof tag_a);
    TEST_ASSERT(starts_with(tag_a, "shared:"));
    TEST_ASSERT_EQ(lc_kernel_mount("a", "b", NULL, MS_BIND, NULL), 0);
    TEST_ASSERT_EQ(lc_kernel_mount("a", "c", NULL, MS_BIND, NULL), 0);
    propagation("b", tag, sizeof tag);
    TEST_ASSERT_STR_EQ(tag, lc_string_length(tag), tag_a, lc_string_length(tag_a));

    TEST_ASSERT_EQ(lc_kernel_mount(NULL, "c", NULL, MS_SLAVE, NULL), 0);
    propagation("c", tag, sizeof tag);
    TEST_ASSERT(starts_with(tag, "master:"));
    TEST_ASSERT_EQ(lc_kernel_mount(NULL, "b", NULL, MS_PRIVATE, NULL), 0);
    propagation("b", tag, sizeof tag);
    TEST_ASSERT_STR_EQ(tag, lc_string_length(tag), "", 0);

    /* a, b and c are one tmpfs, so a directory made in a is in b and c too. */
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "a/x", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mount("tmpfs", "a/x", "tmpfs", 0, NULL), 0);
    TEST_ASSERT_EQ(make_file("a/x/f", "propagated"), 0);
    TEST_ASSERT(holds("c/x/f", "propagated"));     /* a slave receives */
    TEST_ASSERT(exists("b/x"));
    TEST_ASSERT(!exists("b/x/f"));                 /* a private mount does not */

    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "c/y", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mount("tmpfs", "c/y", "tmpfs", 0, NULL), 0);
    TEST_ASSERT_EQ(make_file("c/y/g", "kept"), 0);
    TEST_ASSERT(exists("a/y"));
    TEST_ASSERT(!exists("a/y/g"));                 /* nor does a slave send */

    TEST_ASSERT_EQ(lc_kernel_mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL), 0);
    uint8_t *text = NULL;
    size_t length = 0;
    TEST_REQUIRE_OK(lc_file_read_all("/proc/self/mountinfo", &text, &length));
    TEST_ASSERT(length > 0);
    TEST_ASSERT(!lc_string_contains((const char *)text, length, " shared:", 8));
    TEST_ASSERT(!lc_string_contains((const char *)text, length, " master:", 8));
    lc_heap_free(text);
}

static void test_mount_propagation(void) {
    SANDBOX_RUN(propagation_body, NULL, SANDBOX_DENY_SETGROUPS);
}

/* ===== lc_kernel_umount =====
 *
 * umount2 refuses flags it does not know and a busy mount, and unmounts
 * one that is not, which shows the directory below again. MNT_DETACH
 * takes a busy mount away from the tree at once, while its open files
 * still read. A path that is not a mount point gives EINVAL, as does a
 * mount that came with the namespace's copy of the caller's mounts: the
 * kernel locks those, so a namespace cannot uncover what they hide.
 */
static void umount_body(void *arg) {
    (void)arg;
    TEST_REQUIRE(scratch());
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "m", 0755), 0);
    TEST_ASSERT_EQ(make_file("m/under", "under the mount"), 0);
    TEST_REQUIRE_EQ(lc_kernel_mount("tmpfs", "m", "tmpfs", 0, NULL), 0);
    TEST_ASSERT_EQ(make_file("m/on", "on the mount"), 0);

    TEST_ASSERT_EQ(lc_kernel_umount("m", 0x10), -LC_ERR_INVAL);   /* no such flag */
    lc_sysret fd = lc_kernel_open_file("m/on", O_RDONLY | O_CLOEXEC, 0);
    TEST_ASSERT(fd >= 0);
    TEST_ASSERT_EQ(lc_kernel_umount("m", 0), -LC_ERR_BUSY);
    if (fd >= 0) lc_kernel_close_file((int32_t)fd);
    TEST_ASSERT_EQ(lc_kernel_umount("m", 0), 0);
    TEST_ASSERT(holds("m/under", "under the mount"));
    TEST_ASSERT(!exists("m/on"));
    TEST_ASSERT_EQ(lc_kernel_umount("m", 0), -LC_ERR_INVAL);       /* not a mount point */

    TEST_REQUIRE_EQ(lc_kernel_mount("tmpfs", "m", "tmpfs", 0, NULL), 0);
    TEST_ASSERT_EQ(make_file("m/held", "held open"), 0);
    fd = lc_kernel_open_file("m/held", O_RDONLY | O_CLOEXEC, 0);
    TEST_ASSERT(fd >= 0);
    TEST_ASSERT_EQ(lc_kernel_umount("m", MNT_DETACH), 0);
    TEST_ASSERT(!exists("m/held"));
    TEST_ASSERT(holds("m/under", "under the mount"));
    if (fd >= 0) {
        char buf[16];
        TEST_ASSERT_EQ(lc_kernel_read_all((int32_t)fd, buf, sizeof buf), (lc_sysret)9);
        TEST_ASSERT_STR_EQ(buf, 9, "held open", 9);
        lc_kernel_close_file((int32_t)fd);
    }

    TEST_ASSERT_EQ(lc_kernel_umount("/proc", MNT_DETACH), -LC_ERR_INVAL);
    TEST_ASSERT(exists("/proc/self/status"));
}

static void test_umount(void) {
    SANDBOX_RUN(umount_body, NULL, SANDBOX_DENY_SETGROUPS);
}

/* ===== lc_kernel_pivot_root =====
 *
 * pivot_root makes the tmpfs on SCRATCH the root, and puts the old root
 * on its old/, as a container's setup does; then MNT_DETACH takes the old
 * root away. The paths before and after are the same files, by device and
 * inode. A new root that is not a mount point gives EINVAL.
 */
static void pivot_root_body(void *arg) {
    (void)arg;
    sb_statx old_root, new_root, st;
    TEST_REQUIRE_EQ(file_stat("/", &old_root), 0);
    TEST_REQUIRE(scratch());
    TEST_REQUIRE_EQ(file_stat(".", &new_root), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "old", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "plain", 0755), 0);
    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "plain/old", 0755), 0);
    TEST_ASSERT_EQ(make_file("marker", "in the new root"), 0);

    TEST_ASSERT_EQ(lc_kernel_pivot_root("plain", "plain/old"), -LC_ERR_INVAL);
    TEST_REQUIRE_EQ(lc_kernel_pivot_root(".", "old"), 0);
    TEST_REQUIRE_EQ(lc_kernel_chdir("/"), 0);
    TEST_ASSERT_EQ(file_stat("/", &st), 0);
    TEST_ASSERT(same_file(&st, &new_root));
    TEST_ASSERT_EQ(file_stat("/old", &st), 0);
    TEST_ASSERT(same_file(&st, &old_root));
    TEST_ASSERT(holds("/marker", "in the new root"));
    TEST_ASSERT(exists("/old/proc/self/status"));

    TEST_ASSERT_EQ(lc_kernel_umount("/old", MNT_DETACH), 0);
    TEST_ASSERT_EQ(file_stat("/old", &st), 0);
    TEST_ASSERT(st.dev_major == new_root.dev_major && st.dev_minor == new_root.dev_minor);
    TEST_ASSERT(!exists("/old/proc"));
    TEST_ASSERT(!exists("/proc"));
    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, "/old", AT_REMOVEDIR), 0);
}

static void test_pivot_root(void) {
    SANDBOX_RUN(pivot_root_body, NULL, SANDBOX_DENY_SETGROUPS);
}

/* ===== lc_kernel_sethostname =====
 *
 * The sandbox's UTS namespace starts with the caller's host name, and
 * sethostname changes only its own: it takes `len` bytes of the name, up
 * to 64, and refuses a longer one with EINVAL. Outside, the name is as it
 * was.
 */
static void sethostname_body(void *arg) {
    (void)arg;
    char host[128];
    lc_sysret n = host_name(host, sizeof host);
    TEST_ASSERT_STR_EQ(host, (size_t)(n > 0 ? n : 0), caller_host, lc_string_length(caller_host));

    TEST_ASSERT_EQ(lc_kernel_sethostname("lightc-sandbox", 14), 0);
    n = host_name(host, sizeof host);
    TEST_ASSERT_STR_EQ(host, (size_t)(n > 0 ? n : 0), "lightc-sandbox", 14);

    TEST_ASSERT_EQ(lc_kernel_sethostname("abcdef", 3), 0);
    n = host_name(host, sizeof host);
    TEST_ASSERT_STR_EQ(host, (size_t)(n > 0 ? n : 0), "abc", 3);

    static const char long_name[] =
        "0123456789012345678901234567890123456789012345678901234567890123456789";
    TEST_ASSERT_EQ(lc_kernel_sethostname(long_name, 65), -LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_kernel_sethostname(long_name, 64), 0);
    n = host_name(host, sizeof host);
    TEST_ASSERT_STR_EQ(host, (size_t)(n > 0 ? n : 0), long_name, 64);
}

static void test_sethostname(void) {
    TEST_REQUIRE(note_caller());
    if (SANDBOX_RUN(sethostname_body, NULL, SANDBOX_DENY_SETGROUPS)) {
        char host[128];
        lc_sysret n = host_name(host, sizeof host);
        TEST_ASSERT_STR_EQ(host, (size_t)(n > 0 ? n : 0), caller_host, lc_string_length(caller_host));
    }
}

/* ===== lc_kernel_mknodat =====
 *
 * In a user namespace mknodat makes FIFOs, and whiteouts, the character
 * device 0:0 that overlayfs takes for a deleted file, but any other
 * character or block device gets EPERM: making one needs CAP_MKNOD in the
 * initial user namespace. So the container tests use FIFOs and whiteouts.
 */
static void mknodat_body(void *arg) {
    (void)arg;
    TEST_REQUIRE(scratch());
    sb_statx st;

    TEST_ASSERT_EQ(lc_kernel_mknodat(AT_FDCWD, "fifo", SB_S_IFIFO | 0600, 0), 0);
    TEST_ASSERT_EQ(file_stat("fifo", &st), 0);
    TEST_ASSERT_EQ(st.mode & SB_S_IFMT, SB_S_IFIFO);
    TEST_ASSERT_EQ(st.mode & 07777 & ~0600, 0);    /* the umask may take bits away */
    lc_sysret fd = lc_kernel_open_file("fifo", O_RDWR | O_NONBLOCK | O_CLOEXEC, 0);
    TEST_ASSERT(fd >= 0);
    if (fd >= 0) {
        char byte = 0;
        TEST_ASSERT_EQ(lc_kernel_write_bytes((int32_t)fd, "f", 1), (lc_sysret)1);
        TEST_ASSERT_EQ(lc_kernel_read_bytes((int32_t)fd, &byte, 1), (lc_sysret)1);
        TEST_ASSERT_EQ(byte, 'f');
        lc_kernel_close_file((int32_t)fd);
    }

    TEST_ASSERT_EQ(lc_kernel_mkdirat(AT_FDCWD, "sub", 0755), 0);
    lc_sysret sub = lc_kernel_open_file("sub", O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    TEST_ASSERT(sub >= 0);
    TEST_ASSERT_EQ(lc_kernel_mknodat((int32_t)sub, "fifo", SB_S_IFIFO | 0600, 0), 0);
    TEST_ASSERT_EQ(file_stat("sub/fifo", &st), 0);
    TEST_ASSERT_EQ(st.mode & SB_S_IFMT, SB_S_IFIFO);
    if (sub >= 0) lc_kernel_close_file((int32_t)sub);

    TEST_ASSERT_EQ(lc_kernel_mknodat(AT_FDCWD, "whiteout", SB_S_IFCHR | 0, SB_DEV(0, 0)), 0);
    TEST_ASSERT_EQ(file_stat("whiteout", &st), 0);
    TEST_ASSERT_EQ(st.mode & SB_S_IFMT, SB_S_IFCHR);
    TEST_ASSERT_EQ(st.rdev_major, 0u);
    TEST_ASSERT_EQ(st.rdev_minor, 0u);

    TEST_ASSERT_EQ(lc_kernel_mknodat(AT_FDCWD, "null", SB_S_IFCHR | 0666, SB_DEV(1, 3)), -LC_ERR_PERM);
    TEST_ASSERT(!exists("null"));
    TEST_ASSERT_EQ(lc_kernel_mknodat(AT_FDCWD, "char01", SB_S_IFCHR | 0600, SB_DEV(0, 1)), -LC_ERR_PERM);
    TEST_ASSERT_EQ(lc_kernel_mknodat(AT_FDCWD, "loop0", SB_S_IFBLK | 0600, SB_DEV(7, 0)), -LC_ERR_PERM);
    TEST_ASSERT(!exists("loop0"));
}

static void test_mknodat(void) {
    SANDBOX_RUN(mknodat_body, NULL, SANDBOX_DENY_SETGROUPS);
}

/* ===== lc_kernel_setgroups, lc_kernel_setresgid and lc_kernel_setresuid =====
 *
 * A privilege drop calls setgroups, then setresgid, then setresuid: the
 * first two need CAP_SETGID, which setresuid takes away when it leaves no
 * user id 0. In a sandbox an unprivileged user makes, one uid and one gid
 * are mapped and setgroups is denied, so this is the drop that holds
 * there: setgroups fails with EPERM and the groups stay as they were, and
 * the real, effective and saved ids all become the mapped ones, 0. Each of
 * the three ids is passed: one that is not mapped, in any place, gets
 * EINVAL.
 */
static void drop_body(void *arg) {
    (void)arg;
    char groups_before[2048], groups_after[2048];
    TEST_ASSERT(status_line("Groups:", groups_before, sizeof groups_before));

    TEST_ASSERT_EQ(lc_kernel_setgroups(0, NULL), -LC_ERR_PERM);
    TEST_ASSERT_EQ(lc_kernel_setresgid(0, 0, 0), 0);
    TEST_ASSERT_EQ(lc_kernel_setresuid(0, 0, 0), 0);

    uint32_t ids[4];
    TEST_ASSERT(sandbox_status_ids("Uid:", ids));
    TEST_ASSERT(ids[0] == 0 && ids[1] == 0 && ids[2] == 0 && ids[3] == 0);
    TEST_ASSERT(sandbox_status_ids("Gid:", ids));
    TEST_ASSERT(ids[0] == 0 && ids[1] == 0 && ids[2] == 0 && ids[3] == 0);
    TEST_ASSERT(status_line("Groups:", groups_after, sizeof groups_after));
    _test_print_text("  ");
    _test_print_text(groups_after);
    _test_print_text(" (the caller's groups, unmapped but the one gid 0 maps)\n");
    TEST_ASSERT_STR_EQ(groups_before, lc_string_length(groups_before),
                       groups_after, lc_string_length(groups_after));

    const uint32_t keep = (uint32_t)-1;
    TEST_ASSERT_EQ(lc_kernel_setresgid(keep, keep, keep), 0);
    TEST_ASSERT_EQ(lc_kernel_setresgid(1, keep, keep), -LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_kernel_setresgid(keep, 1, keep), -LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_kernel_setresgid(keep, keep, 1), -LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_kernel_setresuid(keep, keep, keep), 0);
    TEST_ASSERT_EQ(lc_kernel_setresuid(1, keep, keep), -LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_kernel_setresuid(keep, 1, keep), -LC_ERR_INVAL);
    TEST_ASSERT_EQ(lc_kernel_setresuid(keep, keep, 1), -LC_ERR_INVAL);
}

static void test_drop_privileges(void) {
    SANDBOX_RUN(drop_body, NULL, SANDBOX_DENY_SETGROUPS);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    lc_sysret made = lc_kernel_mkdirat(AT_FDCWD, SCRATCH, 0755);
    if (made != 0 && made != -LC_ERR_EXIST) {
        _test_print_text("cannot make " SCRATCH "\n");
        return 1;
    }

    TEST_RUN(test_sandbox_namespaces);
    TEST_RUN(test_sandbox_without_gid_map);
    TEST_RUN(test_sandbox_chosen_ids);
    TEST_RUN(test_sandbox_reports);
    TEST_RUN(test_mount_tmpfs);
    TEST_RUN(test_mount_bind);
    TEST_RUN(test_mount_propagation);
    TEST_RUN(test_umount);
    TEST_RUN(test_pivot_root);
    TEST_RUN(test_sethostname);
    TEST_RUN(test_mknodat);
    TEST_RUN(test_drop_privileges);

    (void)lc_kernel_unlinkat(AT_FDCWD, SCRATCH, AT_REMOVEDIR);
    return test_main();
}
