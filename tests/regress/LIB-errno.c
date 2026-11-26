/*
 * LIB-errno — lc_library_open must say why the kernel refused the file.
 *
 * It returned LC_ERR_NOENT for every open that failed and LC_ERR_IO for
 * every read, lseek, mmap or mprotect that did, so a plugin it could not
 * read, a path through a regular file and a directory all read as a
 * missing file or an I/O error. It now returns the kernel's errno: EACCES
 * and ENOTDIR from the open, and EISDIR from the read of a directory,
 * which opens. A file that ends inside its headers has no errno, and
 * fails with LC_ERR_MALFORMED_ELF.
 *
 * Root reads a file of mode 000, so run as root the check of one runs in a
 * child that has given root up. Run this from the source root, as ninja
 * regress does.
 */

#include "test.h"
#include "plugins.h"
#include <lightc/syscall.h>

#define UNREADABLE "build/regress/LIB-errno.unreadable"
#define SHORT_FILE "build/regress/LIB-errno.short"
#define NOBODY     65534

static int32_t open_error(const char *path) {
    lc_result_ptr r = lc_library_open(path);
    if (lc_ptr_is_ok(r)) lc_library_close(r.value);
    return r.error;
}

/* Writes `count` bytes of `bytes` to a new file of `mode` at `path`. */
static bool make_file(const char *path, uint32_t mode, const void *bytes, size_t count) {
    (void)lc_kernel_unlinkat(AT_FDCWD, path, 0);
    lc_sysret fd = lc_kernel_open_file(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd < 0) return false;
    bool ok = count == 0 || lc_kernel_write_bytes((int32_t)fd, bytes, count) == (lc_sysret)count;
    lc_kernel_close_file((int32_t)fd);
    return ok;
}

static void test_missing_file(void) {
    TEST_ASSERT_EQ(open_error(PLUGIN_DIR "no-such-plugin.so"), LC_ERR_NOENT);
}

static void test_path_through_a_file(void) {
    TEST_ASSERT_EQ(open_error(PLUGIN_DIR "abs.so/plugin.so"), LC_ERR_NOTDIR);   /* the defect: NOENT */
}

static void test_directory(void) {
    TEST_ASSERT_EQ(open_error(PLUGIN_DIR), LC_ERR_ISDIR);   /* the defect: IO */
}

static void test_unreadable_file(void) {
    TEST_REQUIRE(make_file(UNREADABLE, 0, NULL, 0));
    if (lc_kernel_getuid() != 0) {
        TEST_ASSERT_EQ(open_error(UNREADABLE), LC_ERR_ACCES);   /* the defect: NOENT */
    } else {
        lc_sysret pid = lc_kernel_fork();
        if (pid == 0) {
            if (lc_kernel_setgid(NOBODY) < 0 || lc_kernel_setuid(NOBODY) < 0) lc_kernel_exit(2);
            lc_kernel_exit(open_error(UNREADABLE) == LC_ERR_ACCES ? 0 : 1);
        }
        TEST_REQUIRE(pid > 0);
        int32_t status = -1;
        (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
        TEST_ASSERT_EQ(status, 0);
    }
    (void)lc_kernel_unlinkat(AT_FDCWD, UNREADABLE, 0);
}

static void test_short_file(void) {
    TEST_REQUIRE(make_file(SHORT_FILE, 0644, "\177ELF\2\1\1", 7));
    TEST_ASSERT_EQ(open_error(SHORT_FILE), LC_ERR_MALFORMED_ELF);   /* the defect: IO */
    (void)lc_kernel_unlinkat(AT_FDCWD, SHORT_FILE, 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_missing_file);
    TEST_RUN(test_path_through_a_file);
    TEST_RUN(test_directory);
    TEST_RUN(test_unreadable_file);
    TEST_RUN(test_short_file);
    return test_main();
}
