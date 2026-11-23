#include <lightc/log.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/format.h>
#include "kernel.h"          /* struct iovec */

/* Global state */
static lc_log_level  log_min_level = LC_LOG_DEBUG;
static int32_t       log_fd = 2;  /* STDERR */
static lc_log_format log_format = LC_LOG_FORMAT_TEXT;

static const char *level_tags[] = {
    "[DEBUG ",
    "[INFO  ",
    "[WARN  ",
    "[ERROR "
};

static const size_t level_tag_lens[] = { 7, 7, 7, 7 };

/* --- Configuration --- */

void lc_log_set_level(lc_log_level min_level) {
    log_min_level = min_level;
}

void lc_log_set_output(int32_t fd) {
    log_fd = fd;
}

void lc_log_set_format(lc_log_format format) {
    log_format = format;
}

bool lc_log_is_enabled(lc_log_level level) {
    return level >= log_min_level;
}

static const char *level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static const size_t level_name_lens[] = { 5, 4, 4, 5 };

/*
 * Build the log prefix into a buffer:
 *   [LEVEL tid:NNNN]
 * Returns the length written.
 */
static size_t build_prefix(char *buf, size_t buf_size, lc_log_level level) {
    lc_format fmt = lc_format_start(buf, buf_size);
    lc_format_add_string(&fmt, level_tags[level], level_tag_lens[level]);
    lc_format_add_text(&fmt, "tid:");
    lc_format_add_unsigned(&fmt, (uint64_t)(uint32_t)lc_kernel_get_thread_id());
    lc_format_add_text(&fmt, "] ");
    return lc_format_finish(&fmt);
}

/*
 * Add a JSON-escaped version of the message to the format builder.
 * Escapes " \ \n \r \t specially, and every other control byte (< 0x20) as
 * \u00XX — a raw control byte would make the line invalid JSON. Stops early
 * once `soft_limit` bytes have been used, and never emits a partial escape, so
 * the caller can always append the closing `"}` and newline and stay valid.
 */
static void format_add_json_escaped(lc_format *fmt, const char *msg, size_t len,
                                    size_t soft_limit) {
    static const char HEX[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        /* An escape is at most 6 bytes (\u00XX); stop if it would not leave room
         * under the soft limit, so the fixed closing always fits afterwards. */
        if (fmt->used + 6 > soft_limit) break;
        char c = msg[i];
        switch (c) {
        case '"':  lc_format_add_string(fmt, "\\\"", 2); break;
        case '\\': lc_format_add_string(fmt, "\\\\", 2); break;
        case '\n': lc_format_add_string(fmt, "\\n", 2); break;
        case '\r': lc_format_add_string(fmt, "\\r", 2); break;
        case '\t': lc_format_add_string(fmt, "\\t", 2); break;
        default:
            if ((uint8_t)c < 0x20) {
                char esc[6] = { '\\', 'u', '0', '0', HEX[((uint8_t)c >> 4) & 0xF], HEX[(uint8_t)c & 0xF] };
                lc_format_add_string(fmt, esc, 6);
            } else {
                lc_format_add_char(fmt, c);
            }
            break;
        }
    }
}

/*
 * Write a JSON-formatted log line, built whole in one buffer, with one
 * write: lc_kernel_write_all writes the rest only after a short write.
 * {"ts":<seconds>,"level":"...","msg":"...","tid":<tid>}
 */
static void write_json_line(lc_log_level level, const char *message, size_t msg_len) {
    char buf[1024];
    lc_format fmt = lc_format_start(buf, sizeof(buf));

    /* Get timestamp */
    lc_timespec ts;
    lc_kernel_get_clock(LC_CLOCK_REALTIME, &ts);

    /* Build JSON */
    lc_format_add_text(&fmt, "{\"ts\":");
    lc_format_add_signed(&fmt, ts.seconds);
    lc_format_add_text(&fmt, ",\"level\":\"");
    lc_format_add_string(&fmt, level_names[level], level_name_lens[level]);
    lc_format_add_text(&fmt, "\",\"msg\":\"");
    /* Reserve the tail so the closing `","tid":<n>}` + newline always fits and
     * the line stays valid JSON even when the message is longer than the buffer. */
    format_add_json_escaped(&fmt, message, msg_len,
                            fmt.capacity > 40 ? fmt.capacity - 40 : 0);
    lc_format_add_text(&fmt, "\",\"tid\":");
    lc_format_add_unsigned(&fmt, (uint64_t)(uint32_t)lc_kernel_get_thread_id());
    lc_format_add_text(&fmt, "}\n");

    size_t len = lc_format_finish(&fmt);
    if (len > sizeof(buf) - 1) len = sizeof(buf) - 1;

    lc_kernel_write_all(log_fd, buf, len);
}

/*
 * Write a complete log line: prefix, message and newline in one writev,
 * with no lock. Linux does not mix another thread's or process's bytes
 * into a pipe write of at most PIPE_BUF bytes, or into a write to an
 * O_APPEND regular file. A short write (a terminal, a socket, a full
 * non-blocking pipe, a signal) leaves a tail, written after it, and
 * another line can come in between. An error, or a writev that writes
 * nothing, drops the line.
 */
static void write_log_line(lc_log_level level, const char *message, size_t msg_len) {
    if (log_format == LC_LOG_FORMAT_JSON) {
        write_json_line(level, message, msg_len);
        return;
    }

    /* Build prefix on the stack */
    char prefix[48];
    size_t prefix_len = build_prefix(prefix, sizeof(prefix), level);

    int32_t fd = log_fd;     /* the tail goes where the line went */
    struct iovec line[] = {
        { prefix, prefix_len }, { message, msg_len }, { "\n", 1 },
    };
    size_t parts = sizeof(line) / sizeof(line[0]);
    lc_sysret written;
    do {
        written = lc_syscall3(SYS_writev, fd, (int64_t)line, (int64_t)parts);
    } while (written == -LC_ERR_INTR);
    if (written <= 0) return;

    /* After a short write, write the rest of the part it stopped in and the
     * parts after it, and stop at the first that does not go out whole. */
    size_t done = (size_t)written;
    for (size_t i = 0; i < parts; i++) {
        if (done >= line[i].iov_len) {
            done -= line[i].iov_len;
            continue;
        }
        size_t rest = line[i].iov_len - done;
        if (lc_kernel_write_all(fd, (const char *)line[i].iov_base + done, rest)
            != (lc_sysret)rest)
            return;
        done = 0;
    }
}

/* --- Simple logging --- */

void lc_log(lc_log_level level, const char *message) {
    if (level < log_min_level) return;
    lc_log_string(level, message, lc_string_length(message));
}

void lc_log_string(lc_log_level level, const char *message, size_t length) {
    if (level < log_min_level) return;
    write_log_line(level, message, length);
}

/* --- Formatted logging --- */

void lc_log_begin(lc_log_entry *entry, lc_log_level level) {
    entry->level = level;
    if (level < log_min_level) {
        entry->fmt = lc_format_start(entry->buffer, 0);
    } else {
        entry->fmt = lc_format_start(entry->buffer, LC_LOG_BUFFER_SIZE);
    }
}

void lc_log_commit(lc_log_entry *entry) {
    /* lc_log_begin filtered the entry: its zero-length builder has a NULL
     * buffer and stored nothing, so write nothing, even if the level has
     * been lowered since. */
    if (entry->fmt.buffer == NULL || entry->level < log_min_level) return;

    size_t msg_len = lc_format_finish(&entry->fmt);
    /* Clamp to buffer if overflow */
    if (msg_len > LC_LOG_BUFFER_SIZE - 1) msg_len = LC_LOG_BUFFER_SIZE - 1;

    write_log_line(entry->level, entry->buffer, msg_len);
}
