#include <lightc/io.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/heap.h>
#include "kernel.h"          /* struct linux_dirent64, struct statx */

/* ========================================================================
 * Buffered Writer
 * ======================================================================== */

lc_writer lc_writer_create(int32_t fd, size_t buffer_size) {
    lc_writer w;
    w.fd       = fd;
    w.error    = 0;
    lc_result_ptr alloc = lc_heap_allocate(buffer_size);
    w.buffer   = alloc.value;
    w.capacity = w.buffer ? buffer_size : 0;
    w.used     = 0;
    return w;
}

lc_result lc_writer_destroy(lc_writer *writer) {
    lc_result r = lc_writer_flush(writer);
    lc_heap_free(writer->buffer);
    writer->buffer   = NULL;
    writer->capacity = 0;
    writer->used     = 0;
    return r;
}

/* The result of a call that got `count` bytes through before `error`
 * stopped it: unlike lc_err, it keeps the count. */
static lc_result writer_result(size_t count, int32_t error) {
    return (lc_result){ .value = (int64_t)count, .error = error };
}

/*
 * Write `count` bytes of `data` to the fd, retrying EINTR and looping over
 * short writes, and set *written to the number the fd took. Returns 0 when
 * it took them all, LC_ERR_AGAIN when the fd would block, or the error that
 * stopped the write, which becomes the writer's sticky error.
 */
static int32_t writer_write(lc_writer *writer, const uint8_t *data, size_t count, size_t *written) {
    size_t done = 0;
    int32_t error = 0;
    while (done < count) {
        lc_sysret ret = lc_kernel_write_bytes(writer->fd, data + done, count - done);
        if (ret == -LC_ERR_INTR) continue;
        if (ret == -LC_ERR_AGAIN) {
            error = LC_ERR_AGAIN;
            break;
        }
        if (ret <= 0) {
            /* A write that takes no byte would never finish: an I/O error. */
            error = ret < 0 ? (int32_t)(-ret) : LC_ERR_IO;
            writer->error = error;
            break;
        }
        done += (size_t)ret;
    }
    *written = done;
    return error;
}

lc_result lc_writer_flush(lc_writer *writer) {
    if (writer->error) return lc_err(writer->error);

    size_t written;
    int32_t error = writer_write(writer, writer->buffer, writer->used, &written);

    /* Keep the bytes the fd did not take at the front of the buffer: after
     * LC_ERR_AGAIN, the next flush writes them. */
    writer->used -= written;
    if (writer->used > 0) lc_bytes_move(writer->buffer, writer->buffer + written, writer->used);
    return writer_result(written, error);
}

lc_result lc_writer_put_byte(lc_writer *writer, uint8_t byte) {
    if (writer->error == 0 && writer->used < writer->capacity) {
        writer->buffer[writer->used++] = byte;
        return lc_ok(1);
    }
    return lc_writer_put_string(writer, (const char *)&byte, 1);
}

lc_result lc_writer_put_string(lc_writer *writer, const char *str, size_t length) {
    if (writer->error) return lc_err(writer->error);

    const uint8_t *src = (const uint8_t *)str;
    size_t accepted = 0;

    while (accepted < length) {
        size_t remaining = length - accepted;

        /* The buffer is empty and the rest would fill it (always, for an
         * unbuffered writer, whose capacity is 0): write it straight from
         * the caller's memory instead of copying it through. */
        if (writer->used == 0 && remaining >= writer->capacity) {
            size_t written;
            int32_t error = writer_write(writer, src + accepted, remaining, &written);
            return writer_result(accepted + written, error);
        }

        size_t space = writer->capacity - writer->used;
        if (space == 0) {
            lc_result flushed = lc_writer_flush(writer);
            /* Stop on a sticky error, or when the fd would block and took
             * no byte, so the buffer is still full. */
            if (writer->error || writer->used == writer->capacity)
                return writer_result(accepted, flushed.error);
            continue;
        }

        size_t chunk = remaining < space ? remaining : space;
        lc_bytes_copy(writer->buffer + writer->used, src + accepted, chunk);
        writer->used += chunk;
        accepted     += chunk;
    }
    return lc_ok((int64_t)accepted);
}

lc_result lc_writer_put_char(lc_writer *writer, char c) {
    return lc_writer_put_byte(writer, (uint8_t)c);
}

lc_result lc_writer_put_line(lc_writer *writer, const char *str, size_t length) {
    lc_result r = lc_writer_put_string(writer, str, length);
    if (lc_is_err(r)) return r;
    lc_result newline = lc_writer_put_char(writer, '\n');
    newline.value += r.value;
    return newline;
}

/* Write the decimal digits of `value` so that they end at `end`, and return
 * where they start. */
static char *format_decimal(char *end, uint64_t value) {
    do {
        *--end = (char)('0' + value % 10);
        value /= 10;
    } while (value > 0);
    return end;
}

lc_result lc_writer_put_unsigned(lc_writer *writer, uint64_t value) {
    char buf[20];   /* max uint64_t is 20 digits */
    char *p = format_decimal(buf + sizeof(buf), value);
    return lc_writer_put_string(writer, p, (size_t)(buf + sizeof(buf) - p));
}

lc_result lc_writer_put_signed(lc_writer *writer, int64_t value) {
    char buf[20];   /* "-" + up to 19 digits */
    /* Handle INT64_MIN: -(INT64_MIN) overflows, cast first */
    uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1 : (uint64_t)value;
    char *p = format_decimal(buf + sizeof(buf), magnitude);
    if (value < 0) *--p = '-';
    return lc_writer_put_string(writer, p, (size_t)(buf + sizeof(buf) - p));
}

static const char io_hex_digits[] = "0123456789abcdef";

lc_result lc_writer_put_hex(lc_writer *writer, uint64_t value) {
    char buf[18]; /* "0x" + up to 16 hex digits */
    char *p = buf + sizeof(buf);

    if (value == 0) {
        return lc_writer_put_string(writer, "0x0", 3);
    }

    while (value > 0) {
        *--p = io_hex_digits[value & 0xf];
        value >>= 4;
    }

    *--p = 'x';
    *--p = '0';

    return lc_writer_put_string(writer, p, (size_t)(buf + sizeof(buf) - p));
}

lc_result lc_writer_put_newline(lc_writer *writer) {
    return lc_writer_put_char(writer, '\n');
}

/* ========================================================================
 * Buffered Reader
 * ======================================================================== */

lc_reader lc_reader_create(int32_t fd, size_t buffer_size) {
    /* A 0-byte buffer could take no byte, so every read would end the
     * stream at once. */
    if (buffer_size == 0) buffer_size = 4096;

    lc_reader r;
    r.fd          = fd;
    lc_result_ptr alloc = lc_heap_allocate(buffer_size);
    r.buffer      = alloc.value;
    r.capacity    = r.buffer ? buffer_size : 0;
    r.filled      = 0;
    r.position    = 0;
    /* With no buffer the stream ends at once, with the allocation's error. */
    r.end_of_file = r.buffer == NULL;
    r.error       = r.buffer ? 0 : alloc.error;
    return r;
}

void lc_reader_destroy(lc_reader *reader) {
    if (reader->buffer == NULL) return;
    lc_heap_free(reader->buffer);
    reader->buffer   = NULL;
    reader->capacity = 0;
    reader->filled   = 0;
    reader->position = 0;
}

/* Refill the internal buffer from the file descriptor. */
static void reader_refill(lc_reader *reader) {
    if (reader->end_of_file) return;

    reader->position = 0;
    reader->filled   = 0;

    /* Retry EINTR so a signal mid-read is not mistaken for end-of-file. */
    lc_sysret ret = lc_kernel_read_retry(reader->fd, reader->buffer, reader->capacity);
    if (ret > 0) {
        reader->filled = (size_t)ret;
        reader->error  = 0;
        return;
    }
    /* A non-blocking fd with no data yet: not the end of the stream. */
    if (ret == -LC_ERR_AGAIN) {
        reader->error = LC_ERR_AGAIN;
        return;
    }
    /* EOF (0) or a read error ends the stream; error tells them apart. */
    reader->end_of_file = true;
    reader->error       = (int32_t)(-ret);
}

int32_t lc_reader_read_byte(lc_reader *reader) {
    if (reader->buffer == NULL) return -1;
    if (reader->position >= reader->filled) {
        reader_refill(reader);
        if (reader->position >= reader->filled) return -1;
    }
    return (int32_t)reader->buffer[reader->position++];
}

size_t lc_reader_read_bytes(lc_reader *reader, void *buf, size_t count) {
    if (reader->buffer == NULL) return 0;
    uint8_t *dst = (uint8_t *)buf;
    size_t total = 0;

    while (total < count) {
        /* If buffer is exhausted, refill */
        if (reader->position >= reader->filled) {
            reader_refill(reader);
            if (reader->position >= reader->filled) break;
        }

        size_t available = reader->filled - reader->position;
        size_t want      = count - total;
        size_t chunk     = want < available ? want : available;

        lc_bytes_copy(dst + total, reader->buffer + reader->position, chunk);
        reader->position += chunk;
        total            += chunk;
    }

    return total;
}

int64_t lc_reader_read_line(lc_reader *reader, char *buf, size_t buf_size) {
    if (reader->buffer == NULL) return -1;
    if (buf_size == 0) return -1;

    size_t length = 0;
    size_t max    = buf_size - 1; /* leave room for null terminator */

    while (length < max) {
        int32_t byte = lc_reader_read_byte(reader);
        if (byte < 0) {
            /* EOF */
            if (length == 0) {
                buf[0] = '\0';
                return -1;
            }
            break;
        }
        if ((char)byte == '\n') {
            break;
        }
        buf[length++] = (char)byte;
    }

    /* If we stopped because buffer is full, consume rest of line */
    if (length == max) {
        bool consumed = false;
        for (;;) {
            int32_t byte = lc_reader_read_byte(reader);
            if (byte < 0) break;
            consumed = true;
            if ((char)byte == '\n') break;
        }
        /* buf_size 1 stores no byte: only the rest of the line, when
         * there was one, tells a line from the end. */
        if (max == 0 && !consumed) {
            buf[0] = '\0';
            return -1;
        }
    }

    buf[length] = '\0';
    return (int64_t)length;
}

bool lc_reader_is_end(const lc_reader *reader) {
    return reader->end_of_file && reader->position >= reader->filled;
}

/* ========================================================================
 * File Utilities
 * ======================================================================== */

lc_result lc_file_read_all(const char *path, uint8_t **out_data, size_t *out_size) {
    *out_data = NULL;
    *out_size = 0;

    lc_sysret fd_ret = lc_kernel_open_file(path, O_RDONLY | O_CLOEXEC, 0);
    if (fd_ret < 0) return lc_err((int32_t)(-fd_ret));
    int32_t fd = (int32_t)fd_ret;

    /* The size seek to end reports is only a hint for the first allocation:
     * pipes and /proc report 0 or do not support lseek at all, every /sys
     * attribute reports a page whatever it holds, and a file can grow or
     * shrink while it is read. So always read until EOF, into a buffer that
     * grows. One byte more than the hint lets a right hint reach EOF without
     * a reallocation. (A genuinely empty regular file reads 0 bytes
     * immediately and returns the empty result.) */
    lc_sysret end = lc_kernel_seek_position(fd, 0, SEEK_END);
    size_t cap = 4096, len = 0;
    if (end > 0) {
        if (lc_kernel_seek_position(fd, 0, SEEK_SET) < 0) {
            lc_kernel_close_file(fd);
            return lc_err(LC_ERR_IO);
        }
        cap = (size_t)end + 1;
    }

    lc_result_ptr a = lc_heap_allocate(cap);
    if (lc_ptr_is_err(a)) { lc_kernel_close_file(fd); return lc_err(LC_ERR_NOMEM); }
    uint8_t *buf = a.value;
    for (;;) {
        if (len == cap) {
            lc_result_ptr g = lc_heap_reallocate(buf, cap * 2);
            if (lc_ptr_is_err(g)) { lc_heap_free(buf); lc_kernel_close_file(fd); return lc_err(LC_ERR_NOMEM); }
            buf = g.value;
            cap *= 2;
        }
        lc_sysret n = lc_kernel_read_retry(fd, buf + len, cap - len);
        if (n < 0) { lc_heap_free(buf); lc_kernel_close_file(fd); return lc_err((int32_t)(-n)); }
        if (n == 0) break;  /* EOF */
        len += (size_t)n;
    }
    lc_kernel_close_file(fd);
    if (len == 0) { lc_heap_free(buf); return lc_ok(0); }
    *out_data = buf;
    *out_size = len;
    return lc_ok((int64_t)len);
}

lc_result lc_file_write_all(const char *path, const void *data, size_t size) {
    lc_sysret fd_ret = lc_kernel_open_file(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd_ret < 0) return lc_err((int32_t)(-fd_ret));
    int32_t fd = (int32_t)fd_ret;

    /* Write everything, retrying EINTR and looping over partial writes. */
    lc_sysret w = lc_kernel_write_all(fd, data, size);

    lc_kernel_close_file(fd);

    if (w < 0) return lc_err((int32_t)(-w));  /* real error, not an interrupted write */
    if ((size_t)w != size) return lc_err(LC_ERR_IO);
    return lc_ok((int64_t)size);
}

lc_result lc_file_get_size(const char *path) {
    /* statx does not open the file: a FIFO does not block it, it needs no
     * read permission, and a device sees no open. */
    struct statx stx;
    lc_sysret ret = lc_syscall5(SYS_statx, AT_FDCWD, (int64_t)path, 0, STATX_SIZE, (int64_t)&stx);
    if (ret < 0) return lc_err((int32_t)(-ret));
    return lc_ok((int64_t)stx.stx_size);
}

/* ========================================================================
 * Directory Listing
 * ======================================================================== */

/*
 * getdents64 returns entries in the format of struct linux_dirent64
 * (kernel.h). Only its offsets are used: lc_directory_next copies each
 * field out of the record, as it did when the buffer sat at offset 4 and
 * every record was 4 bytes off the struct's alignment (IO-dirent-align).
 * The buffer is aligned to 8 now, as getdents64 aligns its records.
 */
_Static_assert(offsetof(lc_directory, buffer) % 8 == 0, "lc_directory's buffer must be aligned to 8");

lc_result lc_directory_open(lc_directory *dir, const char *path) {
    /* Empty and done before the open, so that one that fails leaves no fd
     * of someone else's to close: the caller's dir may hold anything. */
    dir->fd       = -1;
    dir->error    = 0;
    dir->filled   = 0;
    dir->position = 0;
    dir->done     = true;

    /* O_DIRECTORY makes the kernel reject a non-directory at open (-ENOTDIR),
     * instead of "opening" a regular file and later conflating getdents's
     * -ENOTDIR with end-of-directory. O_CLOEXEC keeps the fd out of exec'd
     * children. */
    lc_sysret fd_ret = lc_kernel_open_file(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    if (fd_ret < 0) {
        dir->error = (int32_t)(-fd_ret);
        return lc_err(dir->error);
    }

    dir->fd   = (int32_t)fd_ret;
    dir->done = false;
    return lc_ok((int64_t)dir->fd);
}

bool lc_directory_next(lc_directory *dir, lc_directory_entry *entry) {
    for (;;) {
        /* If we have data in the buffer, return the next entry */
        if (dir->position < dir->filled) {
            const uint8_t *record = dir->buffer + dir->position;
            uint16_t reclen;
            lc_bytes_copy(&reclen, record + offsetof(struct linux_dirent64, d_reclen), sizeof(reclen));
            /* Guard against a corrupt record: d_reclen == 0 would spin forever,
             * and a record claiming to extend past the filled bytes would read
             * out of bounds. Either means the stream is unusable — stop. */
            if (reclen == 0 || dir->position + reclen > dir->filled) {
                dir->done  = true;
                dir->error = LC_ERR_IO;
                return false;
            }
            entry->name = (const char *)record + offsetof(struct linux_dirent64, d_name);
            entry->type = record[offsetof(struct linux_dirent64, d_type)];
            dir->position += reclen;
            return true;
        }

        /* Buffer exhausted — try to read more */
        if (dir->done) return false;

        lc_sysret ret = lc_kernel_read_directory(dir->fd, dir->buffer, sizeof(dir->buffer));
        if (ret <= 0) {
            /* 0 is the end; a negative return is an error that ends the
             * listing early, which the caller can tell by dir->error. */
            dir->done = true;
            if (ret < 0) dir->error = (int32_t)(-ret);
            return false;
        }

        dir->filled   = (size_t)ret;
        dir->position = 0;
    }
}

void lc_directory_close(lc_directory *dir) {
    if (dir->fd >= 0) {
        lc_kernel_close_file(dir->fd);
        dir->fd = -1;
    }
    /* Empty the buffer too: entries left in it would otherwise come back
     * from a lc_directory_next after the close. */
    dir->filled   = 0;
    dir->position = 0;
    dir->done     = true;
}
