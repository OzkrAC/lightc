/*
 * IO-writer-drops-data — lc_writer must not drop bytes when a write fails,
 * and must say that it failed.
 *
 * lc_writer_flush ignored what lc_kernel_write_all returned and emptied
 * the buffer whatever happened, and the puts, flush and destroy returned
 * nothing. So on a non-blocking fd the first EAGAIN dropped every byte
 * still buffered, a hard error such as ENOSPC dropped them too, and the
 * caller could not tell. Now each put returns how many bytes it accepted,
 * with LC_ERR_AGAIN when the fd would block, flush keeps the bytes the fd
 * did not take, and a hard error stays in the writer's error.
 *
 * The pipe tests write into a non-blocking pipe that starts full, so that
 * the fd blocks however much a pipe holds. Each time a put or flush returns
 * LC_ERR_AGAIN, the test drains the pipe and puts the bytes not accepted
 * again. The pipe is then empty, so every byte accepted must have come out
 * of it or still be buffered, and in the end every byte must have come
 * out, in order.
 *   - A flush on the full pipe writes nothing and keeps its 10 bytes, and
 *     the next flush writes them. A put of 10000 bytes onto 10 buffered
 *     ones fills the buffer and stops at the flush that would block.
 *   - 200000 bytes through a 4096-byte writer in one put_string call per
 *     try: the writer writes these straight from the caller's memory.
 *   - The same bytes in 100-byte put_string calls and in put_byte calls:
 *     each flush then runs from inside a put, on a full buffer. With a
 *     10000-byte buffer, larger than PIPE_BUF, a flush also writes only
 *     part of the buffer and keeps the rest.
 * The last test writes to /dev/full, where every write fails with ENOSPC.
 * All use the results that lightc 0.2.0 added, so this does not build
 * against an earlier lightc.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/syscall.h>

#define TOTAL       200000
#define BUFFER_SIZE 4096
#define MAX_BLOCKED 10000    /* bound on the tries that meet LC_ERR_AGAIN */

static uint8_t sent[TOTAL];
static uint8_t received[TOTAL];
static size_t  received_count;
static size_t  prefill_left;     /* bytes of the prefill not drained yet */
static int32_t pipe_fds[2];
static int32_t blocked;          /* tries that met LC_ERR_AGAIN */

static void print_result(const char *label, lc_result r) {
    _test_print_text("  ");
    _test_print_text(label);
    _test_print_text(": value ");
    lc_print_signed(TEST_FD, r.value);
    _test_print_text(", error ");
    _test_print_text(lc_error_name(r.error));
    _test_print_text("\n");
}

/* Fill the pipe until a write would block; returns the bytes written. */
static size_t fill_pipe(int32_t fd) {
    static const uint8_t junk[BUFFER_SIZE];
    size_t total = 0;
    for (;;) {
        lc_sysret n = lc_kernel_write_bytes(fd, junk, sizeof(junk));
        if (n == -LC_ERR_INTR) continue;
        if (n <= 0) return total;
        total += (size_t)n;
    }
}

/* Create the non-blocking pipe, fill it, and start counting. */
static bool open_full_pipe(void) {
    for (size_t i = 0; i < TOTAL; i++) sent[i] = (uint8_t)(i % 251);
    received_count = 0;
    blocked = 0;
    if (lc_kernel_create_pipe(pipe_fds, O_NONBLOCK | O_CLOEXEC) != 0) return false;
    prefill_left = fill_pipe(pipe_fds[1]);
    return prefill_left > 0;
}

static void close_pipe(void) {
    lc_kernel_close_file(pipe_fds[0]);
    lc_kernel_close_file(pipe_fds[1]);
}

/* Read what the pipe holds now: first the rest of the prefill, which is
 * thrown away, then the writer's bytes. False on an error or on more bytes
 * than were sent. */
static bool drain(void) {
    uint8_t chunk[BUFFER_SIZE];
    for (;;) {
        lc_sysret n = lc_kernel_read_bytes(pipe_fds[0], chunk, sizeof(chunk));
        if (n == -LC_ERR_AGAIN) return true;
        if (n == -LC_ERR_INTR) continue;
        if (n <= 0) return false;
        size_t got = (size_t)n, skip = got < prefill_left ? got : prefill_left;
        prefill_left -= skip;
        got -= skip;
        if (got > TOTAL - received_count) return false;
        lc_bytes_copy(received + received_count, chunk + skip, got);
        received_count += got;
    }
}

/*
 * Put the bytes of `sent` from `accepted` on through `w`, `piece` bytes per
 * put_string call, or with put_byte when `piece` is 0, then flush until the
 * buffer is written. Each time the writer would block, drain the pipe and
 * check that no accepted byte went missing.
 */
static void send_rest(lc_writer *w, size_t accepted, size_t piece) {
    while (accepted < TOTAL) {
        size_t left = TOTAL - accepted;
        size_t want = piece == 0 ? 1 : left < piece ? left : piece;
        lc_result r = piece == 0 ? lc_writer_put_byte(w, sent[accepted])
                                 : lc_writer_put_string(w, (const char *)sent + accepted, want);
        bool counted = r.value >= 0 && (size_t)r.value <= want;
        if (!counted || (r.error != 0 && r.error != LC_ERR_AGAIN)) print_result("put", r);
        TEST_REQUIRE(counted);
        accepted += (size_t)r.value;
        if (r.error == 0) {
            TEST_REQUIRE_EQ((size_t)r.value, want);
            continue;
        }
        TEST_REQUIRE_EQ(r.error, LC_ERR_AGAIN);
        TEST_REQUIRE(++blocked <= MAX_BLOCKED);
        TEST_REQUIRE(drain());
        TEST_REQUIRE_EQ(received_count + w->used, accepted);
    }

    while (w->used > 0) {
        lc_result r = lc_writer_flush(w);
        if (r.error == 0) {
            TEST_REQUIRE_EQ(w->used, (size_t)0);
            break;
        }
        if (r.error != LC_ERR_AGAIN) print_result("flush", r);
        TEST_REQUIRE_EQ(r.error, LC_ERR_AGAIN);
        TEST_REQUIRE(++blocked <= MAX_BLOCKED);
        TEST_REQUIRE(drain());
        TEST_REQUIRE_EQ(received_count + w->used, accepted);
    }
    TEST_REQUIRE(drain());
}

/* Check that every byte came out of the pipe once, in order. */
static void check_received(const char *label, const lc_writer *w) {
    _test_print_text("  ");
    _test_print_text(label);
    _test_print_text(": received ");
    lc_print_unsigned(TEST_FD, received_count);
    _test_print_text(" of 200000 bytes; LC_ERR_AGAIN ");
    lc_print_signed(TEST_FD, blocked);
    _test_print_text(" times\n");

    TEST_ASSERT(blocked > 0);                   /* the pipe did block */
    TEST_ASSERT_EQ(w->used, (size_t)0);
    TEST_ASSERT_EQ(w->error, 0);                /* EAGAIN is not sticky */
    TEST_ASSERT_EQ(received_count, (size_t)TOTAL);
    TEST_ASSERT_EQ(lc_bytes_compare(received, sent, received_count), 0);
}

/* Send all TOTAL bytes through a writer of `buffer_size` bytes. */
static void send_all(const char *label, size_t buffer_size, size_t piece) {
    TEST_REQUIRE(open_full_pipe());
    lc_writer w = lc_writer_create(pipe_fds[1], buffer_size);
    TEST_REQUIRE_NOT_NULL(w.buffer);
    send_rest(&w, 0, piece);
    check_received(label, &w);
    TEST_ASSERT_OK(lc_writer_destroy(&w));
    close_pipe();
}

static void test_flush_keeps_bytes_on_again(void) {
    TEST_REQUIRE(open_full_pipe());
    lc_writer w = lc_writer_create(pipe_fds[1], BUFFER_SIZE);
    TEST_REQUIRE_NOT_NULL(w.buffer);

    /* The 10 bytes fit in the buffer, and the pipe takes none of them. */
    lc_result r = lc_writer_put_string(&w, (const char *)sent, 10);
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(r.value, 10);
    r = lc_writer_flush(&w);
    print_result("flush of 10 bytes, pipe full", r);
    if (r.error == LC_ERR_AGAIN) blocked++;
    TEST_ASSERT_EQ(r.error, LC_ERR_AGAIN);
    TEST_ASSERT_EQ(r.value, 0);
    TEST_ASSERT_EQ(w.used, (size_t)10);
    TEST_ASSERT_EQ(w.error, 0);

    /* Once the pipe has room, the next flush writes them. */
    TEST_REQUIRE(drain());
    r = lc_writer_flush(&w);
    TEST_ASSERT_OK(r);
    TEST_ASSERT_EQ(r.value, 10);
    TEST_ASSERT_EQ(w.used, (size_t)0);
    TEST_REQUIRE(drain());
    TEST_ASSERT_EQ(received_count, (size_t)10);

    /* With 10 bytes buffered and the pipe full again, a put of 10000 fills
     * the buffer and stops at the flush that would block. */
    prefill_left += fill_pipe(pipe_fds[1]);
    r = lc_writer_put_string(&w, (const char *)sent + 10, 10);
    TEST_ASSERT_OK(r);
    r = lc_writer_put_string(&w, (const char *)sent + 20, 10000);
    print_result("put_string of 10000 bytes onto 10, pipe full", r);
    if (r.error == LC_ERR_AGAIN) blocked++;
    TEST_ASSERT_EQ(r.error, LC_ERR_AGAIN);
    TEST_ASSERT_EQ(r.value, BUFFER_SIZE - 10);
    TEST_ASSERT_EQ(w.used, (size_t)BUFFER_SIZE);
    TEST_ASSERT_EQ(w.error, 0);
    TEST_REQUIRE(r.value >= 0 && r.value <= 10000);

    /* Then the bytes not accepted, and the rest, go after them. */
    TEST_REQUIRE(drain());
    send_rest(&w, 20 + (size_t)r.value, TOTAL);
    check_received("the rest, one put_string per try", &w);
    TEST_ASSERT_OK(lc_writer_destroy(&w));
    close_pipe();
}

static void test_put_string_keeps_bytes_on_again(void) {
    send_all("one put_string per try", BUFFER_SIZE, TOTAL);
}

static void test_small_puts_keep_bytes_on_again(void) {
    send_all("100-byte put_string calls", BUFFER_SIZE, 100);
    send_all("put_byte calls", BUFFER_SIZE, 0);
    send_all("100-byte put_string calls, 10000-byte buffer", 10000, 100);
}

static void test_dev_full_error_is_sticky(void) {
    lc_sysret fd = lc_kernel_open_file("/dev/full", O_WRONLY | O_CLOEXEC, 0);
    if (fd < 0) TEST_SKIP("/dev/full cannot be opened");

    /* The bytes fit in the buffer, so only flush meets the error, and it
     * keeps them. */
    static const char data[] = "important data that must not vanish";
    lc_writer w = lc_writer_create((int32_t)fd, 64);
    lc_result put = lc_writer_put_string(&w, data, sizeof(data) - 1);
    TEST_ASSERT_OK(put);
    TEST_ASSERT_EQ(put.value, (int64_t)(sizeof(data) - 1));

    lc_result flushed = lc_writer_flush(&w);
    print_result("flush to /dev/full", flushed);
    TEST_ASSERT_EQ(flushed.error, LC_ERR_NOSPC);
    TEST_ASSERT_EQ(w.error, LC_ERR_NOSPC);
    TEST_ASSERT_EQ(w.used, sizeof(data) - 1);

    /* From then on every call returns it and writes nothing. */
    TEST_ASSERT_EQ(lc_writer_put_byte(&w, 'x').error, LC_ERR_NOSPC);
    TEST_ASSERT_EQ(lc_writer_put_string(&w, data, sizeof(data) - 1).error, LC_ERR_NOSPC);
    TEST_ASSERT_EQ(lc_writer_flush(&w).error, LC_ERR_NOSPC);
    TEST_ASSERT_EQ(w.used, sizeof(data) - 1);
    TEST_ASSERT_EQ(lc_writer_destroy(&w).error, LC_ERR_NOSPC);

    /* A put larger than the buffer, which writes straight to the fd. */
    lc_writer big = lc_writer_create((int32_t)fd, 8);
    put = lc_writer_put_string(&big, data, sizeof(data) - 1);
    print_result("put_string larger than the buffer to /dev/full", put);
    TEST_ASSERT_EQ(put.error, LC_ERR_NOSPC);
    TEST_ASSERT_EQ(put.value, 0);
    TEST_ASSERT_EQ(big.error, LC_ERR_NOSPC);
    TEST_ASSERT_EQ(lc_writer_destroy(&big).error, LC_ERR_NOSPC);

    lc_kernel_close_file((int32_t)fd);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_flush_keeps_bytes_on_again);
    TEST_RUN(test_put_string_keeps_bytes_on_again);
    TEST_RUN(test_small_puts_keep_bytes_on_again);
    TEST_RUN(test_dev_full_error_is_sticky);
    return test_main();
}
