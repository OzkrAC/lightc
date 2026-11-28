#ifndef LIGHTC_IO_H
#define LIGHTC_IO_H

#include "types.h"

/*
 * Buffered I/O — readers, writers, file utilities, directory listing.
 *
 * Writers buffer output and flush to the kernel in batches.
 * Readers buffer input and refill from the kernel when exhausted.
 * File utilities handle common whole-file operations.
 * File status comes from the statx syscall.
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

/* The other types of file, which lc_file_info's type gives too. Each LC_DT_*
 * is a mode's S_IFMT bits shifted right by 12, as getdents64's d_type is. */
#define LC_DT_FIFO     1
#define LC_DT_CHR      2
#define LC_DT_BLK      6
#define LC_DT_SOCK     12

/* A directory being listed. error is 0, or the errno of the read that
 * ended the listing early, as lc_reader's and lc_writer's error is: when
 * lc_directory_next returns false with error 0, every entry was listed.
 * The buffer, which getdents64 fills, is aligned to 8, as the records in it
 * are. */
typedef struct {
    int32_t fd;
    int32_t error;
    _Alignas(8) uint8_t buffer[4096];
    size_t  filled;
    size_t  position;
    bool    done;
} lc_directory;

typedef struct {
    const char *name;
    uint8_t     type;   /* LC_DT_REG, LC_DT_DIR, etc. */
} lc_directory_entry;

/* Open a directory for listing. When it fails, as for a path that is not a
 * directory, *dir has no fd (-1) and is done, so lc_directory_next returns
 * false and lc_directory_close closes nothing; dir->error is the errno. */
[[nodiscard]] lc_result lc_directory_open(lc_directory *dir, const char *path);

/* Get the next entry. Returns true if an entry was read, false when done:
 * at the end, with dir->error 0, or when a read failed, with its errno in
 * dir->error, such as EBADF for a fd closed underneath.
 * NOTE: entry->name points into the directory's internal buffer and is only
 * valid until the next lc_directory_next or lc_directory_close call — copy it
 * if you need to keep it. */
bool lc_directory_next(lc_directory *dir, lc_directory_entry *entry);

/* Close the directory. Its buffer is emptied, so lc_directory_next returns
 * false afterwards; a second close closes nothing. */
void lc_directory_close(lc_directory *dir);

/* ========================================================================
 * File Status
 *
 * lc_file_stat, lc_file_stat_at and lc_file_stat_fd fill an lc_file_info
 * from statx, which opens nothing: a FIFO does not block them, a file needs
 * no read permission, and a device sees no open. They ask statx for the
 * fields of LC_STATX_BASIC_STATS and LC_STATX_MNT_ID.
 *
 * mask has the LC_STATX_* bit of each of those fields that the kernel
 * filled. A filesystem can leave one out, as one that keeps no access
 * times leaves out LC_STATX_ATIME, and that field then holds 0 or a
 * stand-in value. type is LC_DT_UNKNOWN when mask has no LC_STATX_TYPE.
 * The other fields have no bit, as the kernel always fills them:
 * rdev_major and rdev_minor are 0 but for a device node.
 *
 * lc_file_info ends in reserved space, 0 today, so that a later release
 * can add fields, such as the birth time or the subvolume, without
 * changing its size. Each field added there comes with a new LC_STATX_*
 * bit in mask: a caller tells that it was filled by that bit, never by a
 * value that is not 0.
 * ======================================================================== */

/* The bits of lc_file_info's mask, statx's STATX_*: each names the field
 * that the kernel filled. */
#define LC_STATX_TYPE         0x00000001U   /* type */
#define LC_STATX_MODE         0x00000002U   /* mode */
#define LC_STATX_NLINK        0x00000004U   /* links */
#define LC_STATX_UID          0x00000008U   /* uid */
#define LC_STATX_GID          0x00000010U   /* gid */
#define LC_STATX_ATIME        0x00000020U   /* accessed */
#define LC_STATX_MTIME        0x00000040U   /* modified */
#define LC_STATX_CTIME        0x00000080U   /* changed */
#define LC_STATX_INO          0x00000100U   /* inode */
#define LC_STATX_SIZE         0x00000200U   /* size */
#define LC_STATX_BLOCKS       0x00000400U   /* blocks */
#define LC_STATX_BASIC_STATS  0x000007ffU   /* the eleven above */
#define LC_STATX_MNT_ID       0x00001000U   /* mount_id */

/* The bits of lc_file_info's attributes, statx's STATX_ATTR_*. A bit that
 * attributes_mask lacks is one the file's filesystem cannot report, so its
 * being clear in attributes says nothing. */
#define LC_STATX_ATTR_COMPRESSED  0x00000004   /* the filesystem compresses the file */
#define LC_STATX_ATTR_IMMUTABLE   0x00000010   /* the file cannot be changed */
#define LC_STATX_ATTR_APPEND      0x00000020   /* the file can only be appended to */
#define LC_STATX_ATTR_NODUMP      0x00000040   /* backups by dump leave the file out */
#define LC_STATX_ATTR_ENCRYPTED   0x00000800   /* reading the file needs a key */
#define LC_STATX_ATTR_AUTOMOUNT   0x00001000   /* reaching the directory mounts a filesystem */
#define LC_STATX_ATTR_MOUNT_ROOT  0x00002000   /* the root of a mount */
#define LC_STATX_ATTR_VERITY      0x00100000   /* fs-verity guards the file's contents */
#define LC_STATX_ATTR_DAX         0x00200000   /* its I/O bypasses the page cache (DAX) */

/* A time of a file: seconds since the Unix epoch, and the nanoseconds past
 * them, 0 to 999999999. */
typedef struct {
    int64_t seconds;
    int64_t nanoseconds;
} lc_file_time;

/* The status of a file, from statx. type and mode split the file's mode:
 * (uint32_t)type << 12 | mode puts it back together. */
typedef struct {
    uint32_t     mask;             /* the LC_STATX_* of the fields filled */
    uint8_t      type;             /* LC_DT_*, as an lc_directory_entry's */
    uint16_t     mode;             /* the permission bits, 07777 */
    uint32_t     links;            /* the number of hard links */
    uint32_t     uid;              /* the owner */
    uint32_t     gid;              /* the group */
    uint32_t     block_size;       /* the size of I/O the filesystem prefers */
    uint64_t     inode;
    uint64_t     size;             /* in bytes */
    uint64_t     blocks;           /* the 512-byte blocks allocated to it */
    uint64_t     attributes;       /* the LC_STATX_ATTR_* the file has */
    uint64_t     attributes_mask;  /* the LC_STATX_ATTR_* its filesystem reports */
    lc_file_time accessed;         /* the last read */
    lc_file_time modified;         /* the last change of the contents */
    lc_file_time changed;          /* the last change of the contents or the status */
    uint32_t     rdev_major;       /* the device that a device node is */
    uint32_t     rdev_minor;
    uint32_t     dev_major;        /* the device of the filesystem that holds the file */
    uint32_t     dev_minor;
    uint64_t     mount_id;         /* the mount that holds it: the first field of
                                    * its line in /proc/self/mountinfo */
    uint64_t     reserved[4];      /* 0: a later release may fill it, each new
                                    * field with a new LC_STATX_* bit in mask,
                                    * which tells whether it was filled */
} lc_file_info;

/* Get the status of the file at `path` into *info, following a symlink at
 * its end. value = 0 on success. On an error *info is all zero, so its
 * type is LC_DT_UNKNOWN. */
[[nodiscard]] lc_result lc_file_stat(const char *path, lc_file_info *info);

/* Get the status of the file at `path`, relative to the directory `dirfd`
 * (AT_FDCWD for the current one), into *info. flags go to statx unchanged:
 * AT_SYMLINK_NOFOLLOW gives the status of a symlink at the end of path
 * rather than of its target, and statx's other flags, AT_NO_AUTOMOUNT,
 * AT_EMPTY_PATH and the AT_STATX_* sync flags, do what statx(2) says. A
 * flag statx does not take fails with LC_ERR_INVAL, from the kernel.
 * value = 0 on success. On an error *info is all zero. */
[[nodiscard]] lc_result lc_file_stat_at(int32_t dirfd, const char *path, int32_t flags, lc_file_info *info);

/* Get the status of the file that `fd` is open on into *info, from statx
 * with AT_EMPTY_PATH. fd can be any, such as a pipe's, a socket's or one
 * opened O_PATH. value = 0 on success. On an error *info is all zero. */
[[nodiscard]] lc_result lc_file_stat_fd(int32_t fd, lc_file_info *info);

#endif /* LIGHTC_IO_H */
