#ifndef LIGHTC_LOG_H
#define LIGHTC_LOG_H

#include "types.h"
#include "format.h"

/*
 * Thread-safe logging, with no lock.
 *
 * Each log entry is built locally on the stack, then written with a
 * single syscall: a writev for a text line, a write for a JSON one. In an
 * O_APPEND regular file, and in a pipe when the line is at most PIPE_BUF
 * (4096) bytes, lines from other threads and processes never mix into it.
 * Only after a short write (a terminal, a socket, a full non-blocking
 * pipe, a signal) does the rest take a second write, and another line can
 * then come in between. As no lock is held, a child can log after fork.
 *
 * Simple:
 *   lc_log(LC_LOG_INFO, "server started");
 *
 * Formatted (begin/commit pattern):
 *   lc_log_entry entry;
 *   lc_log_begin(&entry, LC_LOG_INFO);
 *   lc_format_add_text(&entry.fmt, "port ");
 *   lc_format_add_unsigned(&entry.fmt, 8080);
 *   lc_log_commit(&entry);
 *
 * Output:
 *   [INFO  tid:1234] server started
 *   [INFO  tid:1234] port 8080
 *
 * Do not copy the entry between begin and commit. lc_log_begin points
 * entry.fmt at entry.buffer, so a copy's fmt still points into the
 * original: text added through it goes to the original's buffer, or to
 * dead stack once the original is out of scope, and committing the copy
 * writes the copy's own buffer, which does not hold that text.
 */

typedef enum {
    LC_LOG_DEBUG,
    LC_LOG_INFO,
    LC_LOG_WARN,
    LC_LOG_ERROR
} lc_log_level;

#define LC_LOG_BUFFER_SIZE 512

/* A log entry being built. Lives on the caller's stack. */
typedef struct {
    lc_log_level level;
    lc_format    fmt;        /* format builder for the message body */
    char         buffer[LC_LOG_BUFFER_SIZE];
} lc_log_entry;

/* --- Configuration --- */

/* Set the minimum log level. Messages below this are discarded. Default: LC_LOG_DEBUG. */
void lc_log_set_level(lc_log_level min_level);

/* Set the output file descriptor. Default: STDERR (fd 2). */
void lc_log_set_output(int32_t fd);

/* --- Simple logging --- */

/* Log a null-terminated message. */
void lc_log(lc_log_level level, const char *message);

/* Log a message with known length. */
void lc_log_string(lc_log_level level, const char *message, size_t length);

/* --- Formatted logging (begin/commit) --- */

/* Initialize a log entry. Call this, then add content with lc_format_add_*,
 * then call lc_log_commit. The entry must be a local variable (stack-allocated). */
void lc_log_begin(lc_log_entry *entry, lc_log_level level);

/* Write the log entry as one line, with one syscall (see above). An entry
 * that lc_log_begin filtered writes nothing, even if the level has been
 * lowered since. */
void lc_log_commit(lc_log_entry *entry);

/* Check if a level would be logged (useful to skip expensive formatting). */
bool lc_log_is_enabled(lc_log_level level);

/* Log output format */
typedef enum {
    LC_LOG_FORMAT_TEXT,    /* default: [LEVEL tid:NNNN] message */
    LC_LOG_FORMAT_JSON     /* {"ts":..., "level":"...", "msg":"...", "tid":...} */
} lc_log_format;

/* Set the output format (default: LC_LOG_FORMAT_TEXT) */
void lc_log_set_format(lc_log_format format);

#endif /* LIGHTC_LOG_H */
