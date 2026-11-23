#ifndef LIGHTC_IO_H
#define LIGHTC_IO_H

#include "types.h"

/*
 * Buffered I/O — readers, writers, file utilities, directory listing.
 *
 * Writers buffer output and flush to the kernel in batches.
 * Readers buffer input and refill from the kernel when exhausted.
 * File utilities handle common whole-file operations.
 * Directory listing wraps the getdents64 syscall.
 */

/* ========================================================================
 * Buffered Writer
 *
 * Each put returns an lc_result whose value is the number of bytes it
 * accepted, buffered or written, on error too. Its error is:
 *   0             every byte was accepted.
 *   LC_ERR_AGAIN  the fd is non-blocking and would block: put the bytes
 *                 not accepted again once it is writable. It is not
 *                 sticky. A number put can stop partway through its
 *                 digits, so on a non-blocking fd format a number into
 *                 a string first and put that.
 *   any other     a write failed (ENOSPC, EPIPE, EIO, ...). The error is
 *                 sticky: it stays in `error`, and every later put, flush
 *                 and destroy returns it and writes nothing.
 * On a blocking fd only a sticky error stops a put, so checking the result
 * of the last flush or of destroy is enough there.
 * ======================================================================== */

typedef struct {
    int32_t  fd;
    int32_t  error;        /* sticky write error (errno), 0 = none */
    uint8_t *buffer;
    size_t   capacity;
    size_t   used;
} lc_writer;

/* Create a buffered writer for the given file descriptor.
 * Allocates an internal buffer of `buffer_size` bytes. With a size of 0, or
 * if the allocation fails, the writer is unbuffered: each put writes to the
 * fd at once. */
lc_writer lc_writer_create(int32_t fd, size_t buffer_size);

/* Destroy a writer, flushing any remaining data and freeing the buffer.
 * Returns the result of that flush: on an error, the bytes it did not write
 * are lost. On a non-blocking fd, flush until it succeeds first. */
[[nodiscard]] lc_result lc_writer_destroy(lc_writer *writer);

/* Flush all buffered data to the file descriptor. value = bytes written.
 * On LC_ERR_AGAIN the bytes not written stay buffered: flush again once the
 * fd is writable. */
[[nodiscard]] lc_result lc_writer_flush(lc_writer *writer);

/* Write a single byte. */
lc_result lc_writer_put_byte(lc_writer *writer, uint8_t byte);

/* Write `length` bytes of `str`. */
lc_result lc_writer_put_string(lc_writer *writer, const char *str, size_t length);

/* Write a single character. */
lc_result lc_writer_put_char(lc_writer *writer, char c);

/* Write `length` bytes of `str`, followed by a newline. value counts the
 * newline too. */
lc_result lc_writer_put_line(lc_writer *writer, const char *str, size_t length);

/* Write a signed integer in decimal. */
lc_result lc_writer_put_signed(lc_writer *writer, int64_t value);

/* Write an unsigned integer in decimal. */
lc_result lc_writer_put_unsigned(lc_writer *writer, uint64_t value);

/* Write an unsigned integer in hexadecimal with 0x prefix. */
lc_result lc_writer_put_hex(lc_writer *writer, uint64_t value);

/* Write a newline character. */
lc_result lc_writer_put_newline(lc_writer *writer);

/* ========================================================================
 * Buffered Reader
 *
 * When a read finds no byte to give, `error` says why:
 *   0             end of file: lc_reader_is_end is true.
 *   LC_ERR_AGAIN  the fd is non-blocking and has no data yet. The stream
 *                 has not ended: read again once the fd is readable.
 *   any other     a read failed. That ends the stream, so the error is
 *                 sticky, and lc_reader_is_end is true.
 * ======================================================================== */

typedef struct {
    int32_t  fd;
    uint8_t *buffer;
    size_t   capacity;
    size_t   filled;       /* how many bytes currently in buffer */
    size_t   position;     /* current read position in buffer */
    bool     end_of_file;  /* the stream ended, at EOF or on an error */
    int32_t  error;        /* last read error (errno), 0 = none */
} lc_reader;

/* Create a buffered reader for the given file descriptor.
 * Allocates an internal buffer of `buffer_size` bytes, or of 4096 for a size
 * of 0. If the allocation fails, the stream ends at once, with that error. */
lc_reader lc_reader_create(int32_t fd, size_t buffer_size);

/* Destroy a reader, freeing the buffer. */
void lc_reader_destroy(lc_reader *reader);

/* Read a single byte. Returns -1 when there is none (see `error`). */
int32_t lc_reader_read_byte(lc_reader *reader);

/* Read up to `count` bytes into `buf`. Returns the number of bytes actually
 * read: fewer than `count` when there were no more (see `error`). */
size_t lc_reader_read_bytes(lc_reader *reader, void *buf, size_t count);

/* Read a line (up to '\n' or EOF) into `buf`. Stores at most `buf_size - 1`
 * bytes plus a null terminator, and skips the rest of a longer line. The
 * newline is NOT included.
 * Returns the line length, or -1 if no byte was left (see `error`). A line
 * that LC_ERR_AGAIN cut short returns what came, and the next call the
 * rest. When LC_ERR_AGAIN cuts short the skip of a longer line, the next
 * call returns what is left of it as a line of its own. */
int64_t lc_reader_read_line(lc_reader *reader, char *buf, size_t buf_size);

/* Check if the reader has reached the end of the stream: EOF, or a read
 * error when `error` is not 0. */
bool lc_reader_is_end(const lc_reader *reader);

/* ========================================================================
 * File Utilities
 * ======================================================================== */

/* Read an entire file into a heap-allocated buffer, up to EOF: the size the
 * file reports is not trusted, so /proc and /sys files, and files that grow
 * or shrink meanwhile, read whole.
 * Caller must lc_heap_free(*out_data) when done.
 * value = bytes read on success. */
[[nodiscard]] lc_result lc_file_read_all(const char *path, uint8_t **out_data, size_t *out_size);

/* Write a buffer to a file (creates or truncates).
 * value = bytes written on success. */
[[nodiscard]] lc_result lc_file_write_all(const char *path, const void *data, size_t size);

/* Get the size of a file in bytes, from statx, without opening it: a FIFO
 * does not block it, and it needs no read permission. For a device node it
 * is st_size, usually 0, not the capacity of the device.
 * value = size on success. */
[[nodiscard]] lc_result lc_file_get_size(const char *path);

/* ========================================================================
 * Directory Listing
 * ======================================================================== */

/* Linux directory entry types */
#define LC_DT_UNKNOWN  0
#define LC_DT_DIR      4
#define LC_DT_REG      8
#define LC_DT_LNK      10

typedef struct {
    int32_t fd;
    uint8_t buffer[4096];
    size_t  filled;
    size_t  position;
    bool    done;
} lc_directory;

typedef struct {
    const char *name;
    uint8_t     type;   /* LC_DT_REG, LC_DT_DIR, etc. */
} lc_directory_entry;

/* Open a directory for listing. */
[[nodiscard]] lc_result lc_directory_open(lc_directory *dir, const char *path);

/* Get the next entry. Returns true if an entry was read, false when done.
 * NOTE: entry->name points into the directory's internal buffer and is only
 * valid until the next lc_directory_next or lc_directory_close call — copy it
 * if you need to keep it. */
bool lc_directory_next(lc_directory *dir, lc_directory_entry *entry);

/* Close the directory. */
void lc_directory_close(lc_directory *dir);

#endif /* LIGHTC_IO_H */
