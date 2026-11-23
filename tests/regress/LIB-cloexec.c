/*
 * LIB-cloexec — lc_library_open must open the plugin close-on-exec.
 *
 * It opened the file with O_RDONLY alone, and holds it open while it reads
 * the ELF header, the program headers and each PT_LOAD. A program that
 * another thread started meanwhile, with fork and execve, inherited the
 * descriptor: a file of the host, open in the new program. It now opens
 * with O_CLOEXEC, as lightc's other opens do.
 *
 * The plugin here is a FIFO, so the open can be looked at while it lasts:
 * a child process calls lc_library_open on it, which waits for the ELF
 * header there. The parent finds the child's descriptor of the FIFO in
 * /proc/PID/fd and reads its flags in /proc/PID/fdinfo: O_CLOEXEC must be
 * set. It then closes its end, and the child's lc_library_open fails at
 * end of file. Each wait is bounded. The FIFO goes in build/regress, so
 * run this from the source root, as ninja regress does.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/heap.h>
#include <lightc/io.h>
#include <lightc/library.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

#define FIFO        "build/regress/LIB-cloexec.fifo"
#define FIFO_NAME   "/LIB-cloexec.fifo"
#define S_IFIFO     0010000
#define WAIT_MS     5000
#define MAX_FD      1024

/* The child's exit statuses. */
#define CHILD_REFUSED   0       /* lc_library_open failed, as it must at EOF */
#define CHILD_LOADED    1

/* /proc/PID/fd/FD into `out`, with FD left out when it is negative. */
static void proc_fd_path(char *out, size_t size, const char *dir, int64_t pid, int64_t fd) {
    lc_format path = lc_format_start(out, size);
    lc_format_add_text(&path, "/proc/");
    lc_format_add_signed(&path, pid);
    lc_format_add_text(&path, dir);
    if (fd >= 0) lc_format_add_signed(&path, fd);
    (void)lc_format_finish(&path);
}

/* The child's descriptor of the FIFO, or -1 if it has none. */
static int64_t find_fifo_fd(int64_t pid) {
    char path[64];
    char target[4096];
    for (int64_t fd = 0; fd < MAX_FD; fd++) {
        proc_fd_path(path, sizeof path, "/fd/", pid, fd);
        lc_sysret n = lc_kernel_readlinkat(AT_FDCWD, path, target, sizeof target);
        if (n > 0 && lc_string_ends_with(target, (size_t)n, FIFO_NAME, sizeof(FIFO_NAME) - 1))
            return fd;
    }
    return -1;
}

/* The octal number after "flags:" in /proc/PID/fdinfo/FD, or -1. */
static int64_t fd_flags(int64_t pid, int64_t fd) {
    char path[64];
    proc_fd_path(path, sizeof path, "/fdinfo/", pid, fd);
    uint8_t *data = NULL;
    size_t size = 0;
    if (lc_file_read_all(path, &data, &size).error) return -1;

    int64_t flags = -1;
    const char *text = (const char *)data;
    int64_t at = lc_string_find_substring(text, size, "flags:", 6);
    if (at >= 0) {
        size_t i = (size_t)at + 6;
        while (i < size && (text[i] == ' ' || text[i] == '\t')) i++;
        flags = 0;
        while (i < size && text[i] >= '0' && text[i] <= '7') flags = flags * 8 + (text[i++] - '0');
    }
    lc_heap_free(data);
    return flags;
}

static void test_plugin_fd_is_close_on_exec(void) {
    (void)lc_kernel_unlinkat(AT_FDCWD, FIFO, 0);
    TEST_REQUIRE_EQ(lc_kernel_mknodat(AT_FDCWD, FIFO, S_IFIFO | 0600, 0), 0);

    lc_sysret pid = lc_kernel_fork();
    TEST_REQUIRE(pid >= 0);
    if (pid == 0) {
        lc_result_ptr r = lc_library_open(FIFO);
        lc_kernel_exit(lc_ptr_is_err(r) ? CHILD_REFUSED : CHILD_LOADED);
    }

    /* A non-blocking open for writing fails until the child has the FIFO
     * open for reading, and then lets the child's open return. */
    int64_t start = lc_time_start_timer();
    lc_sysret writer;
    while ((writer = lc_kernel_open_file(FIFO, O_WRONLY | O_NONBLOCK | O_CLOEXEC, 0)) < 0
           && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);

    int64_t fd = -1;
    while (writer >= 0 && (fd = find_fifo_fd(pid)) < 0
           && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);
    int64_t flags = fd >= 0 ? fd_flags(pid, fd) : -1;

    _test_print_text("  the child's descriptor of the FIFO: ");
    lc_print_signed(TEST_FD, fd);
    _test_print_text(", flags ");
    lc_print_hex(TEST_FD, (uint64_t)flags);
    _test_print_text("\n");

    if (writer >= 0) lc_kernel_close_file((int32_t)writer);
    int32_t status = 0;
    lc_sysret reaped;
    while ((reaped = lc_kernel_wait_for_child((int32_t)pid, &status, WNOHANG)) == 0
           && lc_time_elapsed_milliseconds(start) < 2 * WAIT_MS)
        lc_time_sleep_milliseconds(5);
    if (reaped == 0) {
        _test_print_text("  the child's lc_library_open has not returned; killing it\n");
        lc_kernel_send_signal((int32_t)pid, SIGKILL);
        lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    }
    (void)lc_kernel_unlinkat(AT_FDCWD, FIFO, 0);

    TEST_ASSERT(writer >= 0);                   /* the child opened the FIFO */
    TEST_ASSERT(fd >= 0);                       /* and its descriptor was found */
    TEST_ASSERT(flags >= 0);
    TEST_ASSERT((flags & O_CLOEXEC) != 0);
    TEST_ASSERT_EQ(reaped, pid);
    TEST_ASSERT_EQ(status, CHILD_REFUSED << 8);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_plugin_fd_is_close_on_exec);
    return test_main();
}
