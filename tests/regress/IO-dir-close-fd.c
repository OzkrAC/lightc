/*
 * IO-dir-close-fd — lc_directory_close after a failed lc_directory_open
 * must close nothing, and lc_directory_next after a close must list
 * nothing.
 *
 * A failed lc_directory_open left *dir as it was, so a close of it, as the
 * cleanup of a caller that opens and closes in pairs makes, closed whatever
 * fd the field held: garbage from the stack, 0 to 2 among it. A close also
 * left the buffer's position where it was, so a next after it gave back the
 * entries still in the buffer. The open now empties *dir first, with no
 * fd, and the close empties the buffer.
 *
 * For each of fds 0 to 2, a child sets a dir's fd to it, fails an open of
 * a path that does not exist, closes the dir, and checks that the fd is
 * still open; a fd the test runner left closed is first pointed at
 * /dev/null. Then "/" is listed by one entry and closed, and next must
 * return false.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/string.h>
#include <lightc/syscall.h>

#define F_GETFD 1

static bool fd_is_open(int32_t fd) {
    return lc_syscall3(SYS_fcntl, fd, F_GETFD, 0) >= 0;
}

static int32_t close_after_a_failed_open(void) {
    lc_sysret null = lc_kernel_open_file("/dev/null", O_RDWR, 0);
    if (null < 0) return 10;
    for (int32_t fd = 0; fd <= 2; fd++) {
        if (!fd_is_open(fd) && lc_kernel_duplicate_fd((int32_t)null, fd, 0) < 0) return 11;
        lc_directory dir;
        lc_bytes_fill(&dir, 0, sizeof dir);
        dir.fd = fd;   /* what the caller's dir held before the open */
        if (lc_is_ok(lc_directory_open(&dir, "/nonexistent/lightc/IO-dir-close-fd"))) return 12;
        lc_directory_close(&dir);
        if (!fd_is_open(fd)) return 20 + fd;   /* the defect: the close closed it */
    }
    return 0;
}

static void test_close_after_a_failed_open_closes_nothing(void) {
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) lc_kernel_exit(close_after_a_failed_open());
    TEST_REQUIRE(pid > 0);
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status, 0);   /* the defect: 20 << 8, fd 0 closed */
}

static void test_next_after_close_lists_nothing(void) {
    lc_directory dir;
    TEST_REQUIRE_OK(lc_directory_open(&dir, "/"));
    lc_directory_entry entry;
    TEST_REQUIRE(lc_directory_next(&dir, &entry));   /* "/" has . and .. at least */
    lc_directory_close(&dir);
    TEST_ASSERT(!lc_directory_next(&dir, &entry));   /* the defect: the next entry in the buffer */
    lc_directory_close(&dir);                        /* a second close closes nothing */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_close_after_a_failed_open_closes_nothing);
    TEST_RUN(test_next_after_close_lists_nothing);
    return test_main();
}
