/*
 * LOG-utf8-truncation — a JSON log line must stay valid UTF-8 when its
 * message is cut.
 *
 * A JSON line holds about 1000 bytes, and the message of a longer one was
 * cut at whatever byte the room ran out, which could be inside a multi-byte
 * UTF-8 sequence. The line was then not UTF-8, which JSON requires, and a
 * strict parser rejected it. The cut now drops a sequence it would split.
 *
 * Messages of 666 U+20AC, 3 bytes each, after 0, 1 and 2 ASCII bytes, put
 * the cut at each place in a sequence. Each line, read from a pipe, must be
 * strict UTF-8 and end as a JSON line ends.
 */

#include "test.h"
#include <lightc/log.h>
#include <lightc/syscall.h>

/* Whether s[0 .. n) is UTF-8 as RFC 3629 has it: no overlong form, no
 * surrogate, nothing past U+10FFFF, no sequence cut short. */
static bool strict_utf8(const uint8_t *s, size_t n) {
    for (size_t i = 0; i < n; ) {
        uint32_t c = s[i];
        size_t len;
        uint32_t min;
        if (c < 0x80)                { i++; continue; }
        else if ((c & 0xE0) == 0xC0) { len = 2; min = 0x80;    c &= 0x1F; }
        else if ((c & 0xF0) == 0xE0) { len = 3; min = 0x800;   c &= 0x0F; }
        else if ((c & 0xF8) == 0xF0) { len = 4; min = 0x10000; c &= 0x07; }
        else return false;
        if (len > n - i) return false;
        for (size_t k = 1; k < len; k++) {
            if ((s[i + k] & 0xC0) != 0x80) return false;
            c = (c << 6) | (s[i + k] & 0x3F);
        }
        if (c < min || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) return false;
        i += len;
    }
    return true;
}

static void test_cut_message_stays_utf8(void) {
    int32_t fds[2];
    TEST_REQUIRE(lc_kernel_create_pipe(fds, 0) == 0);
    lc_log_set_output(fds[1]);
    lc_log_set_format(LC_LOG_FORMAT_JSON);

    static char message[2 + 3 * 666];
    for (int shift = 0; shift < 3; shift++) {
        size_t n = 0;
        for (int i = 0; i < shift; i++) message[n++] = 'a';
        for (int i = 0; i < 666; i++) {
            message[n++] = (char)0xE2;
            message[n++] = (char)0x82;
            message[n++] = (char)0xAC;
        }
        lc_log_string(LC_LOG_INFO, message, n);

        uint8_t line[2048];
        lc_sysret got = lc_kernel_read_bytes(fds[0], line, sizeof line);
        TEST_REQUIRE(got > 0);
        size_t len = (size_t)got;
        bool valid = strict_utf8(line, len);
        bool closed = len >= 3 && line[len - 2] == '}' && line[len - 1] == '\n';
        if (!valid) {
            _test_print_text("  shift ");
            lc_print_signed(TEST_FD, shift);
            _test_print_text(": the line is not UTF-8\n");
        }
        TEST_ASSERT(valid);   /* the defect: a sequence cut short before the closing quote */
        TEST_ASSERT(closed);
    }
    lc_log_set_output(STDERR);
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_cut_message_stays_utf8);
    return test_main();
}
