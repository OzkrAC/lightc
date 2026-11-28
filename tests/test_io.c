/*
 * test_io.c — tests for lightc buffered I/O (writer + reader).
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/signal.h>
#include <lightc/time.h>
#include <lightc/heap.h>
#include <lightc/format.h>
#include <lightc/thread.h>
#include <lightc/socket.h>

/* Helper: create a pipe pair, returning 0 on success. */
static int pipe_create(int32_t *read_fd, int32_t *write_fd) {
    int32_t fds[2];
    lc_sysret ret = lc_kernel_create_pipe(fds, 0);
    if (ret < 0) return -1;
    *read_fd  = fds[0];
    *write_fd = fds[1];
    return 0;
}

/* Helper: read exactly `count` bytes from a raw fd into buf. */
static size_t raw_read(int32_t fd, char *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        lc_sysret ret = lc_kernel_read_bytes(fd, buf + total, count - total);
        if (ret <= 0) break;
        total += (size_t)ret;
    }
    return total;
}

/* ===== Writer lifecycle ===== */

static void test_writer_create_destroy(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, 256);
    TEST_ASSERT_NOT_NULL(w.buffer);
    TEST_ASSERT_EQ(w.fd, wfd);
    TEST_ASSERT_EQ(w.capacity, 256);
    TEST_ASSERT_EQ(w.used, 0);
    TEST_ASSERT_EQ(w.error, 0);

    TEST_ASSERT_OK(lc_writer_destroy(&w));
    TEST_ASSERT_NULL(w.buffer);
    TEST_ASSERT_EQ(w.capacity, 0);
    TEST_ASSERT_EQ(w.used, 0);

    lc_kernel_close_file(rfd);
}

/* ===== Writer: put_string + flush ===== */

static void test_writer_put_string_flush(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, 256);
    lc_result put = lc_writer_put_string(&w, "hello", 5);
    TEST_ASSERT_OK(put);
    TEST_ASSERT_EQ(put.value, 5);
    lc_result flushed = lc_writer_flush(&w);
    TEST_ASSERT_OK(flushed);
    TEST_ASSERT_EQ(flushed.value, 5);
    lc_kernel_close_file(wfd);

    char buf[64];
    size_t n = raw_read(rfd, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, 5);
    TEST_ASSERT_STR_EQ(buf, 5, "hello", 5);

    lc_kernel_close_file(rfd);
    /* buffer already freed by close; free manually since we flushed ourselves */
    TEST_ASSERT_OK(lc_writer_destroy(&w));
}

/* ===== Writer: put_byte ===== */

static void test_writer_put_byte(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, 256);
    lc_writer_put_byte(&w, 0x41); /* 'A' */
    lc_writer_put_byte(&w, 0x42); /* 'B' */
    lc_writer_put_byte(&w, 0x43); /* 'C' */
    TEST_ASSERT_OK(lc_writer_flush(&w));
    lc_kernel_close_file(wfd);

    char buf[64];
    size_t n = raw_read(rfd, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, 3);
    TEST_ASSERT_STR_EQ(buf, 3, "ABC", 3);

    lc_kernel_close_file(rfd);
    TEST_ASSERT_OK(lc_writer_destroy(&w));
}

/* ===== Writer: put_char ===== */

static void test_writer_put_char(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, 256);
    lc_writer_put_char(&w, 'X');
    lc_writer_put_char(&w, 'Y');
    TEST_ASSERT_OK(lc_writer_flush(&w));
    lc_kernel_close_file(wfd);

    char buf[64];
    size_t n = raw_read(rfd, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, 2);
    TEST_ASSERT_STR_EQ(buf, 2, "XY", 2);

    lc_kernel_close_file(rfd);
    TEST_ASSERT_OK(lc_writer_destroy(&w));
}

/* ===== Writer: put_line ===== */

static void test_writer_put_line(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, 256);
    lc_result put = lc_writer_put_line(&w, "hello", 5);
    TEST_ASSERT_OK(put);
    TEST_ASSERT_EQ(put.value, 6);      /* the newline counts */
    TEST_ASSERT_OK(lc_writer_flush(&w));
    lc_kernel_close_file(wfd);

    char buf[64];
    size_t n = raw_read(rfd, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, 6);
    TEST_ASSERT_STR_EQ(buf, 6, "hello\n", 6);

    lc_kernel_close_file(rfd);
    TEST_ASSERT_OK(lc_writer_destroy(&w));
}

/* ===== Writer: put_signed ===== */

static void test_writer_put_signed(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, 256);

    /* positive */
    lc_writer_put_signed(&w, 42);
    lc_writer_put_char(&w, ',');

    /* negative: the sign counts */
    lc_result put = lc_writer_put_signed(&w, -99);
    TEST_ASSERT_OK(put);
    TEST_ASSERT_EQ(put.value, 3);
    lc_writer_put_char(&w, ',');

    /* zero */
    lc_writer_put_signed(&w, 0);

    TEST_ASSERT_OK(lc_writer_flush(&w));
    lc_kernel_close_file(wfd);

    char buf[64];
    size_t n = raw_read(rfd, buf, sizeof(buf));
    TEST_ASSERT_STR_EQ(buf, n, "42,-99,0", 8);

    lc_kernel_close_file(rfd);
    TEST_ASSERT_OK(lc_writer_destroy(&w));
}

/* ===== Writer: put_unsigned ===== */

static void test_writer_put_unsigned(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, 256);

    lc_writer_put_unsigned(&w, 0);
    lc_writer_put_char(&w, ',');
    lc_writer_put_unsigned(&w, 12345);
    lc_writer_put_char(&w, ',');
    lc_writer_put_unsigned(&w, 18446744073709551615ULL);

    TEST_ASSERT_OK(lc_writer_flush(&w));
    lc_kernel_close_file(wfd);

    char buf[64];
    size_t n = raw_read(rfd, buf, sizeof(buf));
    TEST_ASSERT_STR_EQ(buf, n, "0,12345,18446744073709551615", 28);

    lc_kernel_close_file(rfd);
    TEST_ASSERT_OK(lc_writer_destroy(&w));
}

/* ===== Reader lifecycle ===== */

static void test_reader_create_destroy(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_reader r = lc_reader_create(rfd, 128);
    TEST_ASSERT_NOT_NULL(r.buffer);
    TEST_ASSERT_EQ(r.fd, rfd);
    TEST_ASSERT_EQ(r.capacity, 128);
    TEST_ASSERT_EQ(r.filled, 0);
    TEST_ASSERT_EQ(r.position, 0);
    TEST_ASSERT(!r.end_of_file);

    lc_reader_destroy(&r);
    TEST_ASSERT_NULL(r.buffer);

    lc_kernel_close_file(rfd);
    lc_kernel_close_file(wfd);
}

/* ===== Reader: read_byte ===== */

static void test_reader_read_byte(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    /* Write raw bytes into pipe */
    lc_kernel_write_bytes(wfd, "ABC", 3);
    lc_kernel_close_file(wfd);

    lc_reader r = lc_reader_create(rfd, 128);
    TEST_ASSERT_EQ(lc_reader_read_byte(&r), 'A');
    TEST_ASSERT_EQ(lc_reader_read_byte(&r), 'B');
    TEST_ASSERT_EQ(lc_reader_read_byte(&r), 'C');
    TEST_ASSERT_EQ(lc_reader_read_byte(&r), -1); /* EOF */

    lc_reader_destroy(&r);
    lc_kernel_close_file(rfd);
}

/* ===== Reader: read_line ===== */

static void test_reader_read_line(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_kernel_write_bytes(wfd, "first\nsecond\n", 13);
    lc_kernel_close_file(wfd);

    lc_reader r = lc_reader_create(rfd, 128);
    char line[64];

    int64_t len1 = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(len1, 5);
    TEST_ASSERT_STR_EQ(line, 5, "first", 5);

    int64_t len2 = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(len2, 6);
    TEST_ASSERT_STR_EQ(line, 6, "second", 6);

    /* EOF — no more data */
    int64_t len3 = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(len3, -1);

    lc_reader_destroy(&r);
    lc_kernel_close_file(rfd);
}

/* ===== Writer+Reader round-trip ===== */

static void test_writer_reader_roundtrip(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    /* Write via buffered writer */
    lc_writer w = lc_writer_create(wfd, 64);
    lc_writer_put_string(&w, "line1\n", 6);
    lc_writer_put_string(&w, "line2\n", 6);
    lc_writer_put_signed(&w, -42);
    lc_writer_put_char(&w, '\n');
    TEST_ASSERT_OK(lc_writer_flush(&w));
    lc_kernel_close_file(wfd);
    /* Destroy after closing fd to avoid double-flush to closed fd */
    w.fd = -1;
    TEST_ASSERT_OK(lc_writer_destroy(&w));

    /* Read via buffered reader */
    lc_reader r = lc_reader_create(rfd, 64);
    char line[64];

    int64_t n1 = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(n1, 5);
    TEST_ASSERT_STR_EQ(line, 5, "line1", 5);

    int64_t n2 = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(n2, 5);
    TEST_ASSERT_STR_EQ(line, 5, "line2", 5);

    int64_t n3 = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(n3, 3);
    TEST_ASSERT_STR_EQ(line, 3, "-42", 3);

    lc_reader_destroy(&r);
    lc_kernel_close_file(rfd);
}

/* ===== M1: writer with a failed buffer allocation must not spin ===== */

/* A writer whose buffer allocation failed has capacity 0. put_string's chunking
 * loop makes no progress at capacity 0 and, unguarded, spins forever. Such a
 * writer is unbuffered instead: each put writes straight to the fd. */
static void test_writer_put_string_no_buffer(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    lc_writer w = lc_writer_create(wfd, (size_t)-1);  /* SIZE_MAX -> allocation fails */
    TEST_ASSERT_NULL(w.buffer);
    TEST_ASSERT_EQ(w.capacity, (size_t)0);

    lc_result put = lc_writer_put_string(&w, "data", 4);  /* must return, not hang */
    TEST_ASSERT_OK(put);
    TEST_ASSERT_EQ(put.value, 4);
    TEST_ASSERT_OK(lc_writer_put_byte(&w, 'x'));
    TEST_ASSERT_OK(lc_writer_destroy(&w));
    lc_kernel_close_file(wfd);

    char buf[16];
    size_t n = raw_read(rfd, buf, sizeof(buf));
    TEST_ASSERT_STR_EQ(buf, n, "datax", 5);
    lc_kernel_close_file(rfd);
}

/* ===== M1: lc_file_write_all / lc_file_read_all round-trip ===== */

static void test_file_write_read_all(void) {
    const char *path = "build/m1_io_roundtrip.tmp";
    const char payload[] = "lightc file I/O round-trip \x01\x02\x03 payload";
    const size_t len = sizeof(payload) - 1;

    TEST_ASSERT_OK(lc_file_write_all(path, payload, len));

    uint8_t *data = NULL;
    size_t size = 0;
    TEST_ASSERT_OK(lc_file_read_all(path, &data, &size));
    TEST_ASSERT_EQ(size, len);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_STR_EQ((const char *)data, size, payload, len);

    lc_heap_free(data);
    (void)lc_kernel_unlinkat(AT_FDCWD, path, 0);
}

/* ===== M1: a signal must not truncate a buffered read (EINTR retry) ===== */

static volatile int32_t eintr_sig_count;
static void eintr_sig_handler(int signo) { (void)signo; eintr_sig_count++; }

/*
 * The reader blocks in a refill on an empty pipe; a child interrupts it with
 * SIGUSR1 (the handler is installed without SA_RESTART, so the read returns
 * EINTR), then delivers the data a moment later. With the EINTR-retry fix the
 * refill resumes and returns "PING"; without it, EINTR is mistaken for
 * end-of-file and the stream data is lost (read returns 0 bytes).
 */
static void test_io_reader_eintr(void) {
    int32_t rfd, wfd;
    TEST_ASSERT_EQ(pipe_create(&rfd, &wfd), 0);

    eintr_sig_count = 0;
    TEST_ASSERT_OK(lc_signal_handle(SIGUSR1, eintr_sig_handler));

    int32_t parent = lc_kernel_get_process_id();
    lc_sysret pid = lc_kernel_fork();
    TEST_ASSERT(pid >= 0);
    if (pid == 0) {
        /* Child: interrupt the parent's blocking read, then feed it the data. */
        lc_time_sleep_milliseconds(40);
        lc_kernel_send_signal(parent, SIGUSR1);
        lc_time_sleep_milliseconds(40);
        (void)lc_kernel_write_bytes(wfd, "PING", 4);
        lc_time_sleep_milliseconds(20);
        lc_kernel_exit(0);
    }

    lc_reader r = lc_reader_create(rfd, 64);
    char buf[8];
    lc_bytes_fill(buf, 0, sizeof(buf));
    size_t n = lc_reader_read_bytes(&r, buf, 4);

    int32_t status = 0;
    lc_kernel_wait_for_child((int32_t)pid, &status, 0);

    TEST_ASSERT_EQ(n, (size_t)4);            /* data survived the signal */
    TEST_ASSERT_STR_EQ(buf, n, "PING", 4);
    TEST_ASSERT(eintr_sig_count >= 1);       /* a signal really did interrupt */

    lc_reader_destroy(&r);
    (void)lc_signal_reset(SIGUSR1);
    lc_kernel_close_file(rfd);
    lc_kernel_close_file(wfd);
}

/* ===== main ===== */

/* directory_open must reject a non-directory (O_DIRECTORY) rather than "open"
 * a regular file and list it as empty. */
static void test_directory_open_notdir(void) {
    const char *path = "build/m_notdir.tmp";
    TEST_ASSERT_OK(lc_file_write_all(path, "x", 1));

    lc_directory dir;
    TEST_ASSERT_ERR(lc_directory_open(&dir, path));   /* a regular file: rejected */
    (void)lc_kernel_unlinkat(AT_FDCWD, path, 0);

    TEST_ASSERT_OK(lc_directory_open(&dir, "."));      /* a real directory: ok */
    lc_directory_close(&dir);
}

/* lc_directory's error: 0 after a listing that reached its end, the errno
 * of a failed open, which leaves no fd and nothing to list, and EBADF from
 * a next whose fd was closed underneath it. */
static void test_directory_error(void) {
    lc_directory dir;
    lc_directory_entry entry;
    TEST_REQUIRE_OK(lc_directory_open(&dir, "."));
    size_t entries = 0;
    while (entries < 100000 && lc_directory_next(&dir, &entry)) entries++;
    TEST_ASSERT(entries >= 2);   /* . and .. */
    TEST_ASSERT_EQ(dir.error, 0);
    lc_directory_close(&dir);

    TEST_ASSERT_ERR(lc_directory_open(&dir, "/nonexistent/lightc/test_io"));
    TEST_ASSERT_EQ(dir.error, LC_ERR_NOENT);
    TEST_ASSERT_EQ(dir.fd, -1);
    TEST_ASSERT(!lc_directory_next(&dir, &entry));
    lc_directory_close(&dir);

    TEST_REQUIRE_OK(lc_directory_open(&dir, "."));
    lc_kernel_close_file(dir.fd);   /* closed underneath the listing */
    TEST_ASSERT(!lc_directory_next(&dir, &entry));
    TEST_ASSERT_EQ(dir.error, LC_ERR_BADF);
    dir.fd = -1;                    /* closed already */
    lc_directory_close(&dir);
}

/* ===== Close-on-exec of the fds the io module opens ===== */

#define F_GETFD     1
#define F_GETFL     3
#define F_SETFL     4
#define FD_CLOEXEC  1
#define S_IFIFO     0010000
#define CLOEXEC_FIFO "build/test_io_cloexec.fifo"

static void test_directory_fd_close_on_exec(void) {
    lc_directory dir;
    TEST_REQUIRE_OK(lc_directory_open(&dir, "."));
    TEST_ASSERT((lc_syscall3(SYS_fcntl, dir.fd, F_GETFD, 0) & FD_CLOEXEC) != 0);
    lc_directory_close(&dir);
}

/* The octal flags in /proc/self/fdinfo of the fd of this process on the
 * FIFO other than `mine`, waiting up to 2 s for one; -1 when none comes. */
static int64_t fifo_fd_flags(int32_t mine) {
    for (int tries = 0; tries < 200; tries++) {
        for (int64_t fd = 0; fd < 1024; fd++) {
            if (fd == mine) continue;
            char path[64], target[256];
            lc_format f = lc_format_start(path, sizeof path);
            lc_format_add_text(&f, "/proc/self/fd/");
            lc_format_add_signed(&f, fd);
            lc_format_finish(&f);
            lc_sysret n = lc_kernel_readlinkat(AT_FDCWD, path, target, sizeof target);
            if (n <= 0 || !lc_string_ends_with(target, (size_t)n, "/test_io_cloexec.fifo", 21)) continue;

            f = lc_format_start(path, sizeof path);
            lc_format_add_text(&f, "/proc/self/fdinfo/");
            lc_format_add_signed(&f, fd);
            lc_format_finish(&f);
            uint8_t *info = NULL;
            size_t size = 0;
            if (lc_file_read_all(path, &info, &size).error) return -1;
            int64_t flags = -1;
            int64_t at = lc_string_find_substring((const char *)info, size, "flags:", 6);
            if (at >= 0) {
                size_t i = (size_t)at + 6;
                while (i < size && (info[i] == ' ' || info[i] == '\t')) i++;
                flags = 0;
                while (i < size && info[i] >= '0' && info[i] <= '7') flags = flags * 8 + (info[i++] - '0');
            }
            lc_heap_free(info);
            return flags;
        }
        lc_time_sleep_milliseconds(10);
    }
    return -1;
}

static int32_t read_all_of_fifo(void *arg) {
    (void)arg;
    uint8_t *data = NULL;
    size_t size = 0;
    if (lc_is_ok(lc_file_read_all(CLOEXEC_FIFO, &data, &size))) lc_heap_free(data);
    return 0;
}

static int32_t write_all_to_fifo(void *arg) {
    (void)arg;
    static char big[256 * 1024];   /* more than a pipe holds: the write waits */
    (void)lc_file_write_all(CLOEXEC_FIFO, big, sizeof big);
    return 0;
}

/* lc_file_read_all and lc_file_write_all hold their fd only during the
 * call, but a program another thread execs meanwhile would inherit it if
 * it were not close-on-exec. On a FIFO each call waits, in its read or its
 * write, while /proc/self/fdinfo shows the fd's flags. */
static void test_file_functions_close_on_exec(void) {
    (void)lc_kernel_unlinkat(AT_FDCWD, CLOEXEC_FIFO, 0);
    TEST_REQUIRE_EQ(lc_kernel_mknodat(AT_FDCWD, CLOEXEC_FIFO, S_IFIFO | 0600, 0), 0);

    lc_thread reader;
    TEST_REQUIRE_OK(lc_thread_create(&reader, read_all_of_fifo, NULL));
    lc_sysret writer_fd = lc_kernel_open_file(CLOEXEC_FIFO, O_WRONLY, 0);   /* meets the reader's open */
    TEST_REQUIRE(writer_fd >= 0);
    int64_t read_flags = fifo_fd_flags((int32_t)writer_fd);
    lc_kernel_close_file((int32_t)writer_fd);   /* the reader reaches the end */
    lc_thread_join(&reader);

    lc_thread writer;
    TEST_REQUIRE_OK(lc_thread_create(&writer, write_all_to_fifo, NULL));
    lc_sysret reader_fd = lc_kernel_open_file(CLOEXEC_FIFO, O_RDONLY, 0);
    TEST_REQUIRE(reader_fd >= 0);
    int64_t write_flags = fifo_fd_flags((int32_t)reader_fd);
    static char sink[65536];
    while (lc_kernel_read_bytes((int32_t)reader_fd, sink, sizeof sink) > 0) {}   /* until the writer closes */
    lc_kernel_close_file((int32_t)reader_fd);
    lc_thread_join(&writer);
    (void)lc_kernel_unlinkat(AT_FDCWD, CLOEXEC_FIFO, 0);

    TEST_ASSERT(read_flags >= 0 && (read_flags & O_CLOEXEC) != 0);
    TEST_ASSERT(write_flags >= 0 && (write_flags & O_CLOEXEC) != 0);
}

/* ===== lc_writer's flush goes on after EINTR ===== */

static volatile int32_t writer_alarms;
static void count_writer_alarm(int signo) { (void)signo; writer_alarms++; }

/* The flush waits for room in a full pipe; a SIGALRM interrupts it at
 * 20 ms, and a child makes room at 60 ms. The flush must go on and write
 * every byte. */
static void test_writer_flush_retries_eintr(void) {
    TEST_REQUIRE_OK(lc_signal_handle(LC_SIGALRM, count_writer_alarm));
    int32_t rfd, wfd;
    TEST_REQUIRE_EQ(pipe_create(&rfd, &wfd), 0);

    static char block[4096];
    int64_t flags = lc_syscall3(SYS_fcntl, wfd, F_GETFL, 0);
    TEST_REQUIRE(flags >= 0);
    TEST_REQUIRE_EQ(lc_syscall3(SYS_fcntl, wfd, F_SETFL, flags | O_NONBLOCK), 0);
    size_t filled = 0;
    for (;;) {
        lc_sysret n = lc_kernel_write_bytes(wfd, block, sizeof block);
        if (n <= 0) break;
        filled += (size_t)n;
    }
    TEST_REQUIRE_EQ(lc_syscall3(SYS_fcntl, wfd, F_SETFL, flags), 0);

    size_t total = filled + 100;
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        lc_kernel_close_file(wfd);   /* so a flush cut short ends the drain */
        lc_time_sleep_milliseconds(60);
        size_t drained = 0;
        while (drained < total) {
            lc_sysret n = lc_kernel_read_bytes(rfd, block, sizeof block);
            if (n <= 0) break;
            drained += (size_t)n;
        }
        lc_kernel_exit(drained == total ? 0 : 1);
    }
    TEST_REQUIRE(pid > 0);

    lc_writer w = lc_writer_create(wfd, 256);
    (void)lc_writer_put_string(&w, block, 100);
    writer_alarms = 0;
    int64_t timer[4] = { 0, 0, 0, 20000 };   /* struct itimerval: 20 ms, once */
    (void)lc_syscall3(SYS_setitimer, 0 /* ITIMER_REAL */, (int64_t)timer, 0);
    lc_result r = lc_writer_flush(&w);
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(writer_alarms, 1);
    (void)lc_writer_destroy(&w);
    lc_kernel_close_file(wfd);
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status, 0);   /* every byte arrived */
    lc_kernel_close_file(rfd);
    TEST_ASSERT_OK(lc_signal_reset(LC_SIGALRM));
}

/* /proc files report size 0 via SEEK_END; lc_file_read_all must still return
 * their contents by reading until EOF, not an empty buffer. */
static void test_file_read_all_proc(void) {
    uint8_t *data = NULL;
    size_t size = 0;
    lc_result r = lc_file_read_all("/proc/self/stat", &data, &size);
    TEST_ASSERT_OK(r);
    TEST_ASSERT(size > 0);
    TEST_ASSERT_NOT_NULL(data);
    lc_heap_free(data);
}

/* ===== lc_file_stat, lc_file_stat_at and lc_file_stat_fd ===== */

/* The tests make their files in STAT_DIR, and stat them relative to a dirfd
 * on it, as STAT_DIR "/" name from the current directory, or by an fd. */
#define STAT_DIR          "build"
#define STAT_FIFO         "test_io_stat.fifo"
#define STAT_SOCKET       "test_io_stat.sock"
#define STAT_WRITE_ONLY   "test_io_stat.write-only"
#define STAT_TARGET       "test_io_stat.target"
#define STAT_LINK         "test_io_stat.link"
#define STAT_MISSING      "test_io_stat.missing"
#define STAT_SYSFS        "/sys/devices/system/cpu/online"

/* struct stat, which fstat fills, has st_size at offset 48 on both arches,
 * and st_mode at 24 on x86_64 (asm/stat.h) and at 16 on aarch64
 * (asm-generic/stat.h). */
#define STAT_SIZE_OFFSET  48
#if defined(__x86_64__)
#define STAT_MODE_OFFSET  24
#elif defined(__aarch64__)
#define STAT_MODE_OFFSET  16
#endif

/* An AF_UNIX socket address: struct sockaddr_un of <linux/un.h>. */
typedef struct {
    uint16_t family;
    char     path[108];
} test_sockaddr_un;

/* A dirfd on STAT_DIR, or a negative errno. */
static int32_t stat_dir(void) {
    return (int32_t)lc_kernel_open_file(STAT_DIR, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
}

/* The size and the type, as an LC_DT_*, that fstat gives for the file at
 * path, opened without blocking: a system call other than statx, to check
 * lc_file_stat_at against. false when the open or the fstat fails. */
static bool fstat_size_and_type(const char *path, uint64_t *size, uint32_t *type) {
    lc_sysret fd = lc_kernel_open_file(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC, 0);
    if (fd < 0) return false;
    static uint8_t st[256];   /* more than either arch's struct stat */
    lc_sysret ret = lc_syscall2(SYS_fstat, fd, (int64_t)st);
    lc_kernel_close_file((int32_t)fd);
    if (ret != 0) return false;
    uint32_t mode;
    lc_bytes_copy(size, st + STAT_SIZE_OFFSET, sizeof *size);
    lc_bytes_copy(&mode, st + STAT_MODE_OFFSET, sizeof mode);
    *type = mode >> 12;
    return true;
}

/* A FIFO: statx opens nothing, so it returns at once, with size 0. */
static void test_file_stat_at_fifo(void) {
    int32_t dir = stat_dir();
    TEST_REQUIRE(dir >= 0);
    (void)lc_kernel_unlinkat(dir, STAT_FIFO, 0);
    TEST_REQUIRE_EQ(lc_kernel_mknodat(dir, STAT_FIFO, S_IFIFO | 0600, 0), 0);
    TEST_ASSERT_EQ(lc_kernel_fchmodat(dir, STAT_FIFO, 0640, 0), 0);   /* whatever the umask */

    lc_file_info info;
    TEST_ASSERT_OK(lc_file_stat_at(dir, STAT_FIFO, 0, &info));
    TEST_ASSERT_EQ(info.mask & (LC_STATX_TYPE | LC_STATX_MODE | LC_STATX_SIZE),
                   LC_STATX_TYPE | LC_STATX_MODE | LC_STATX_SIZE);
    TEST_ASSERT_EQ(info.type, LC_DT_FIFO);
    TEST_ASSERT_EQ(info.mode, 0640);
    TEST_ASSERT_EQ(info.size, 0);

    lc_file_info by_path;
    TEST_ASSERT_OK(lc_file_stat(STAT_DIR "/" STAT_FIFO, &by_path));
    TEST_ASSERT_EQ(by_path.type, LC_DT_FIFO);
    TEST_ASSERT_EQ(by_path.inode, info.inode);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(dir, STAT_FIFO, 0), 0);
    lc_kernel_close_file(dir);
}

/* An AF_UNIX socket bound to a path: the path is a socket of size 0, and so
 * is the file its fd is open on, an inode of sockfs. */
static void test_file_stat_at_socket(void) {
    int32_t dir = stat_dir();
    TEST_REQUIRE(dir >= 0);
    (void)lc_kernel_unlinkat(dir, STAT_SOCKET, 0);
    lc_result opened = lc_socket_open(LC_AF_UNIX, LC_SOCK_STREAM, 0);
    TEST_REQUIRE_OK(opened);
    int32_t fd = (int32_t)opened.value;
    test_sockaddr_un address = { .family = LC_AF_UNIX };
    lc_bytes_copy(address.path, STAT_DIR "/" STAT_SOCKET, sizeof(STAT_DIR "/" STAT_SOCKET));
    TEST_REQUIRE_EQ(lc_syscall3(SYS_bind, fd, (int64_t)&address, sizeof address), 0);

    lc_file_info info;
    TEST_ASSERT_OK(lc_file_stat_at(dir, STAT_SOCKET, 0, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_SOCK);
    TEST_ASSERT_EQ(info.size, 0);

    lc_file_info by_fd;
    TEST_ASSERT_OK(lc_file_stat_fd(fd, &by_fd));
    TEST_ASSERT_EQ(by_fd.type, LC_DT_SOCK);
    TEST_ASSERT_EQ(by_fd.size, 0);

    lc_socket_close(fd);
    TEST_ASSERT_EQ(lc_kernel_unlinkat(dir, STAT_SOCKET, 0), 0);
    lc_kernel_close_file(dir);
}

/* A /proc file reports size 0, whatever it holds, as fstat says too. /proc/self
 * is a symlink, which AT_SYMLINK_NOFOLLOW stats, and without it the
 * directory it points at. */
static void test_file_stat_at_proc(void) {
    lc_file_info info;
    TEST_ASSERT_OK(lc_file_stat_at(AT_FDCWD, "/proc/self/stat", 0, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_REG);
    TEST_ASSERT_EQ(info.size, 0);
    uint64_t size = 1;
    uint32_t type = 0;
    TEST_ASSERT(fstat_size_and_type("/proc/self/stat", &size, &type));
    TEST_ASSERT_EQ(info.size, size);
    TEST_ASSERT_EQ(info.type, type);

    TEST_ASSERT_OK(lc_file_stat_at(AT_FDCWD, "/proc/self", AT_SYMLINK_NOFOLLOW, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_LNK);
    TEST_ASSERT_OK(lc_file_stat_at(AT_FDCWD, "/proc/self", 0, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_DIR);
}

/* A sysfs attribute reports a page, whatever it holds: the size fstat
 * gives, not that of what a read returns. */
static void test_file_stat_at_sysfs(void) {
    lc_file_info info;
    lc_result r = lc_file_stat_at(AT_FDCWD, STAT_SYSFS, 0, &info);
    if (r.error == LC_ERR_NOENT) TEST_SKIP("no sysfs at /sys");
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(info.type, LC_DT_REG);
    TEST_ASSERT(info.size > 0);
    uint64_t size = 0;
    uint32_t type = 0;
    TEST_REQUIRE(fstat_size_and_type(STAT_SYSFS, &size, &type));
    TEST_ASSERT_EQ(info.size, size);
    TEST_ASSERT_EQ(info.type, type);
    TEST_ASSERT_EQ(lc_file_get_size(STAT_SYSFS).value, (int64_t)size);
}

/* A file of mode 0200, which its owner may write but not read: statx needs
 * no read permission, and an fd opened to write gives the same file. */
static void test_file_stat_at_write_only(void) {
    int32_t dir = stat_dir();
    TEST_REQUIRE(dir >= 0);
    (void)lc_kernel_unlinkat(dir, STAT_WRITE_ONLY, 0);
    TEST_REQUIRE_OK(lc_file_write_all(STAT_DIR "/" STAT_WRITE_ONLY, "seventeen bytes!\n", 17));
    TEST_REQUIRE_EQ(lc_kernel_fchmodat(dir, STAT_WRITE_ONLY, 0200, 0), 0);

    lc_file_info info;
    TEST_ASSERT_OK(lc_file_stat_at(dir, STAT_WRITE_ONLY, 0, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_REG);
    TEST_ASSERT_EQ(info.mode, 0200);
    TEST_ASSERT_EQ(info.size, 17);

    lc_sysret fd = lc_kernel_open_file_at(dir, STAT_WRITE_ONLY, O_WRONLY | O_CLOEXEC, 0);
    TEST_ASSERT(fd >= 0);
    if (fd >= 0) {
        lc_file_info by_fd;
        TEST_ASSERT_OK(lc_file_stat_fd((int32_t)fd, &by_fd));
        TEST_ASSERT_EQ(by_fd.type, LC_DT_REG);
        TEST_ASSERT_EQ(by_fd.size, 17);
        TEST_ASSERT_EQ(by_fd.inode, info.inode);
        TEST_ASSERT_EQ(by_fd.dev_major, info.dev_major);
        TEST_ASSERT_EQ(by_fd.dev_minor, info.dev_minor);
        lc_kernel_close_file((int32_t)fd);
    }

    TEST_ASSERT_EQ(lc_kernel_unlinkat(dir, STAT_WRITE_ONLY, 0), 0);
    lc_kernel_close_file(dir);
}

/* lc_file_stat follows a symlink, and lc_file_stat_at with
 * AT_SYMLINK_NOFOLLOW gives the symlink itself, whose size is the length
 * of its target. */
static void test_file_stat_symlink(void) {
    int32_t dir = stat_dir();
    TEST_REQUIRE(dir >= 0);
    (void)lc_kernel_unlinkat(dir, STAT_LINK, 0);
    (void)lc_kernel_unlinkat(dir, STAT_TARGET, 0);
    TEST_REQUIRE_OK(lc_file_write_all(STAT_DIR "/" STAT_TARGET, "five!", 5));
    TEST_REQUIRE_EQ(lc_kernel_symlinkat(STAT_TARGET, dir, STAT_LINK), 0);

    lc_file_info info;
    TEST_ASSERT_OK(lc_file_stat(STAT_DIR "/" STAT_LINK, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_REG);
    TEST_ASSERT_EQ(info.size, 5);
    TEST_ASSERT_OK(lc_file_stat_at(dir, STAT_LINK, 0, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_REG);
    TEST_ASSERT_OK(lc_file_stat_at(dir, STAT_LINK, AT_SYMLINK_NOFOLLOW, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_LNK);
    TEST_ASSERT_EQ(info.size, sizeof(STAT_TARGET) - 1);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(dir, STAT_LINK, 0), 0);
    TEST_ASSERT_EQ(lc_kernel_unlinkat(dir, STAT_TARGET, 0), 0);
    lc_kernel_close_file(dir);
}

/* The other fields of a new file: its owner, one link, its times, and the
 * device and mount it shares with the directory that holds it. */
static void test_file_stat_fields(void) {
    int32_t dir = stat_dir();
    TEST_REQUIRE(dir >= 0);
    (void)lc_kernel_unlinkat(dir, STAT_TARGET, 0);
    int64_t before = lc_time_now_unix();
    TEST_REQUIRE_OK(lc_file_write_all(STAT_DIR "/" STAT_TARGET, "five!", 5));

    lc_file_info info;
    TEST_ASSERT_OK(lc_file_stat_at(dir, STAT_TARGET, 0, &info));
    TEST_ASSERT(info.mask & LC_STATX_MNT_ID);
    TEST_ASSERT_EQ(info.uid, lc_kernel_getuid());
    TEST_ASSERT_EQ(info.links, 1);
    TEST_ASSERT(info.inode != 0);
    TEST_ASSERT(info.block_size > 0);
    TEST_ASSERT_EQ(info.rdev_major, 0);
    TEST_ASSERT_EQ(info.rdev_minor, 0);
    /* A day either way: the filesystem's clock need not be this one. */
    TEST_ASSERT(info.modified.seconds > before - 86400 && info.modified.seconds < before + 86400);
    TEST_ASSERT(info.modified.nanoseconds >= 0 && info.modified.nanoseconds < 1000000000);
    TEST_ASSERT(info.changed.nanoseconds >= 0 && info.changed.nanoseconds < 1000000000);
    TEST_ASSERT(info.accessed.nanoseconds >= 0 && info.accessed.nanoseconds < 1000000000);

    lc_file_info directory;
    TEST_ASSERT_OK(lc_file_stat_fd(dir, &directory));
    TEST_ASSERT_EQ(directory.type, LC_DT_DIR);
    TEST_ASSERT_EQ(directory.dev_major, info.dev_major);
    TEST_ASSERT_EQ(directory.dev_minor, info.dev_minor);
    TEST_ASSERT_EQ(directory.mount_id, info.mount_id);

    TEST_ASSERT_EQ(lc_kernel_unlinkat(dir, STAT_TARGET, 0), 0);
    lc_kernel_close_file(dir);
}

/* attributes: /proc is the root of a mount, and /proc/self, which is a
 * directory of that mount, is not. Linux 5.8 and later report the bit for
 * every file, so attributes_mask has it. */
static void test_file_stat_attributes(void) {
    lc_file_info root, self;
    TEST_REQUIRE_OK(lc_file_stat("/proc", &root));
    TEST_REQUIRE_OK(lc_file_stat("/proc/self", &self));
    TEST_ASSERT(root.attributes_mask & LC_STATX_ATTR_MOUNT_ROOT);
    TEST_ASSERT(self.attributes_mask & LC_STATX_ATTR_MOUNT_ROOT);
    TEST_ASSERT(root.attributes & LC_STATX_ATTR_MOUNT_ROOT);
    TEST_ASSERT(!(self.attributes & LC_STATX_ATTR_MOUNT_ROOT));
    TEST_ASSERT_EQ(root.mount_id, self.mount_id);
}

/* A pipe's fd is a FIFO. */
static void test_file_stat_fd_pipe(void) {
    int32_t rfd, wfd;
    TEST_REQUIRE_EQ(pipe_create(&rfd, &wfd), 0);
    lc_file_info info;
    TEST_ASSERT_OK(lc_file_stat_fd(rfd, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_FIFO);
    TEST_ASSERT_OK(lc_file_stat_fd(wfd, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_FIFO);
    lc_kernel_close_file(rfd);
    lc_kernel_close_file(wfd);
}

/* On an error *info is all zero, whatever it held: type LC_DT_UNKNOWN and
 * mask 0. A flag statx does not take, such as AT_REMOVEDIR, fails with
 * EINVAL from the kernel. */
static void test_file_stat_errors(void) {
    lc_file_info info;
    lc_bytes_fill(&info, 0xa5, sizeof info);
    lc_result r = lc_file_stat(STAT_DIR "/" STAT_MISSING, &info);
    TEST_ASSERT_EQ(r.error, LC_ERR_NOENT);
    TEST_ASSERT_EQ(info.type, LC_DT_UNKNOWN);
    TEST_ASSERT_EQ(info.mask, 0);
    TEST_ASSERT_EQ(info.size, 0);
    TEST_ASSERT_EQ(info.inode, 0);

    lc_bytes_fill(&info, 0xa5, sizeof info);
    TEST_ASSERT_EQ(lc_file_stat_at(AT_FDCWD, "/", AT_REMOVEDIR, &info).error, LC_ERR_INVAL);
    TEST_ASSERT_EQ(info.mask, 0);
    TEST_ASSERT_EQ(lc_file_stat_at(-1, "relative", 0, &info).error, LC_ERR_BADF);
    TEST_ASSERT_EQ(lc_file_stat_fd(-1, &info).error, LC_ERR_BADF);
    TEST_ASSERT_EQ(info.type, LC_DT_UNKNOWN);
}

/* True when every word of info's reserved space is 0. */
static bool reserved_is_zero(const lc_file_info *info) {
    for (size_t i = 0; i < sizeof info->reserved / sizeof info->reserved[0]; i++)
        if (info->reserved[i] != 0) return false;
    return true;
}

/* The reserved space is 0 after each of the three calls, whatever the
 * caller's struct held before, and after one that fails. */
static void test_file_stat_reserved_is_zero(void) {
    int32_t dir = stat_dir();
    TEST_REQUIRE(dir >= 0);
    (void)lc_kernel_unlinkat(dir, STAT_TARGET, 0);
    TEST_REQUIRE_OK(lc_file_write_all(STAT_DIR "/" STAT_TARGET, "five!", 5));

    lc_file_info info;
    lc_bytes_fill(&info, 0xa5, sizeof info);
    TEST_ASSERT_OK(lc_file_stat(STAT_DIR "/" STAT_TARGET, &info));
    TEST_ASSERT(reserved_is_zero(&info));

    lc_bytes_fill(&info, 0xa5, sizeof info);
    TEST_ASSERT_OK(lc_file_stat_at(dir, STAT_TARGET, AT_SYMLINK_NOFOLLOW, &info));
    TEST_ASSERT(reserved_is_zero(&info));

    lc_bytes_fill(&info, 0xa5, sizeof info);
    TEST_ASSERT_OK(lc_file_stat_fd(dir, &info));
    TEST_ASSERT(reserved_is_zero(&info));

    lc_bytes_fill(&info, 0xa5, sizeof info);
    TEST_ASSERT_EQ(lc_file_stat_at(dir, STAT_MISSING, 0, &info).error, LC_ERR_NOENT);
    TEST_ASSERT(reserved_is_zero(&info));

    TEST_ASSERT_EQ(lc_kernel_unlinkat(dir, STAT_TARGET, 0), 0);
    lc_kernel_close_file(dir);
}

/* lc_file_get_size is lc_file_stat's size: a directory's, and a device
 * node's st_size, 0 for /dev/null. */
static void test_file_get_size_is_stat_size(void) {
    lc_file_info info;
    TEST_REQUIRE_OK(lc_file_stat(STAT_DIR, &info));
    TEST_ASSERT_EQ(info.type, LC_DT_DIR);
    TEST_ASSERT_EQ(lc_file_get_size(STAT_DIR).value, (int64_t)info.size);
    TEST_REQUIRE_OK(lc_file_stat("/dev/null", &info));
    TEST_ASSERT_EQ(info.type, LC_DT_CHR);
    TEST_ASSERT_EQ(info.rdev_major, 1);   /* /dev/null is char device 1:3 */
    TEST_ASSERT_EQ(info.rdev_minor, 3);
    lc_result r = lc_file_get_size("/dev/null");
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(r.value, 0);
    TEST_ASSERT_EQ(lc_file_get_size(STAT_DIR "/" STAT_MISSING).error, LC_ERR_NOENT);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    /* Writer lifecycle */
    TEST_RUN(test_writer_create_destroy);

    /* Writer output */
    TEST_RUN(test_writer_put_string_flush);
    TEST_RUN(test_writer_put_byte);
    TEST_RUN(test_writer_put_char);
    TEST_RUN(test_writer_put_line);
    TEST_RUN(test_writer_put_signed);
    TEST_RUN(test_writer_put_unsigned);

    /* Reader lifecycle */
    TEST_RUN(test_reader_create_destroy);

    /* Reader input */
    TEST_RUN(test_reader_read_byte);
    TEST_RUN(test_reader_read_line);

    /* Round-trip */
    TEST_RUN(test_writer_reader_roundtrip);

    /* M1: EINTR / partial-I/O handling */
    TEST_RUN(test_writer_put_string_no_buffer);
    TEST_RUN(test_file_write_read_all);
    TEST_RUN(test_io_reader_eintr);
    TEST_RUN(test_directory_open_notdir);
    TEST_RUN(test_directory_error);
    TEST_RUN(test_directory_fd_close_on_exec);
    TEST_RUN(test_file_functions_close_on_exec);
    TEST_RUN(test_writer_flush_retries_eintr);
    TEST_RUN(test_file_read_all_proc);

    /* File status */
    TEST_RUN(test_file_stat_at_fifo);
    TEST_RUN(test_file_stat_at_socket);
    TEST_RUN(test_file_stat_at_proc);
    TEST_RUN(test_file_stat_at_sysfs);
    TEST_RUN(test_file_stat_at_write_only);
    TEST_RUN(test_file_stat_symlink);
    TEST_RUN(test_file_stat_fields);
    TEST_RUN(test_file_stat_attributes);
    TEST_RUN(test_file_stat_fd_pipe);
    TEST_RUN(test_file_stat_errors);
    TEST_RUN(test_file_stat_reserved_is_zero);
    TEST_RUN(test_file_get_size_is_stat_size);

    return test_main();
}
