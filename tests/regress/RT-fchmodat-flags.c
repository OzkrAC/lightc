/*
 * RT-fchmodat-flags — lc_kernel_fchmodat with AT_SYMLINK_NOFOLLOW must not
 * change the mode of a symlink's target.
 *
 * The kernel's fchmodat has no flags argument, and the wrapper passed its
 * flags to it anyway, so AT_SYMLINK_NOFOLLOW was dropped: called on a
 * symlink, the wrapper followed it, gave the target the new mode and
 * returned 0. Linux has no symlink modes, so the call must leave the target
 * alone and return -EOPNOTSUPP, on every kernel.
 *
 * The target is created 0644 (less umask), so MODE, which has an execute
 * bit, always differs from its starting mode. The files go in build/regress,
 * so run this from the source root, as ninja regress does.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/syscall.h>

/* lightc defines AT_SYMLINK_NOFOLLOW from 0.1.1 on. This copy lets the test
 * build against 0.1.0, which has the defect. */
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif
#define ERRNO_OPNOTSUPP 95      /* EOPNOTSUPP: operation not supported */

#define TARGET      "build/regress/RT-fchmodat-flags.target"
#define LINK        "build/regress/RT-fchmodat-flags.link"
#define LINK_TARGET "RT-fchmodat-flags.target"  /* relative to the link's directory */
#define MODE        0700

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

static void test_nofollow_leaves_the_target_alone(void) {
    /* Start clean: O_TRUNC keeps the mode of a target an aborted run left. */
    (void)lc_kernel_unlinkat(AT_FDCWD, LINK, 0);
    (void)lc_kernel_unlinkat(AT_FDCWD, TARGET, 0);
    TEST_REQUIRE_OK(lc_file_write_all(TARGET, "x", 1));
    TEST_REQUIRE_EQ(lc_kernel_symlinkat(LINK_TARGET, AT_FDCWD, LINK), 0);

    lc_sysret before = file_mode(TARGET);
    TEST_REQUIRE(before >= 0);
    TEST_REQUIRE_NEQ(before, MODE);

    lc_sysret ret = lc_kernel_fchmodat(AT_FDCWD, LINK, MODE, AT_SYMLINK_NOFOLLOW);
    _test_print_text("  lc_kernel_fchmodat(link, 0700, AT_SYMLINK_NOFOLLOW) = ");
    lc_print_signed(TEST_FD, ret);
    lc_print_newline(TEST_FD);
    TEST_ASSERT_EQ(file_mode(TARGET), before);      /* the target kept its mode */
    TEST_ASSERT_EQ(ret, -ERRNO_OPNOTSUPP);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, LINK, 0), 0);
    TEST_ASSERT_EQ(lc_kernel_unlinkat(AT_FDCWD, TARGET, 0), 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_nofollow_leaves_the_target_alone);
    return test_main();
}
