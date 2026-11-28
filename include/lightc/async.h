#ifndef LIGHTC_ASYNC_H
#define LIGHTC_ASYNC_H

#include "types.h"

/*
 * Async I/O — built on io_uring.
 *
 * io_uring uses shared memory ring buffers between user and kernel.
 * After setup, I/O can be submitted and completed with zero syscalls
 * on the fast path.
 *
 *   1. Create a ring: lc_async_ring_create(queue_size)
 *   2. Submit work:   lc_async_submit_read/write(ring, ...)
 *   3. Flush to kernel: lc_async_flush_ex(ring) — or automatic on wait
 *   4. Get results:   lc_async_wait_ex(ring, ...) or lc_async_peek(ring, ...)
 *   5. Destroy:       lc_async_ring_destroy(ring)
 *
 * An entry the kernel does not take, as when io_uring_enter fails with
 * EAGAIN or EBUSY, stays queued, and each later flush or wait offers it
 * again. lc_async_flush_ex and lc_async_wait_ex report the errno of a failed
 * io_uring_enter. lc_async_flush, lc_async_wait and lc_async_peek cannot, so
 * the ring keeps it and the next _ex call reports it.
 */

/* Result of a completed async operation */
typedef struct {
    uint64_t tag;       /* user-provided tag from submit */
    int32_t  result;    /* bytes transferred, or negative errno */
} lc_async_result;

/* Opaque ring structure — see async.c for internals */
typedef struct lc_async_ring lc_async_ring;

/* Create an async I/O ring with the given queue depth.
 * Caller must destroy with lc_async_ring_destroy. Fails with
 * LC_ERR_OPNOTSUPP on a kernel without IORING_FEAT_NODROP (before 5.5),
 * which drops completions when the completion queue is full. */
[[nodiscard]] lc_result_ptr lc_async_ring_create(uint32_t queue_size);

/* Destroy the ring and free all resources. */
void lc_async_ring_destroy(lc_async_ring *ring);

/* Queue a read operation. Does not submit to kernel yet.
 * tag: user-defined value returned in the completion result.
 * offset: file offset (-1 for current position). */
[[nodiscard]] lc_result lc_async_submit_read(lc_async_ring *ring, int32_t fd,
                          void *buf, uint32_t count,
                          uint64_t offset, uint64_t tag);

/* Queue a write operation. Does not submit to kernel yet. */
[[nodiscard]] lc_result lc_async_submit_write(lc_async_ring *ring, int32_t fd,
                           const void *buf, uint32_t count,
                           uint64_t offset, uint64_t tag);

/* Queue a send on a socket. Does not submit to kernel yet. flags are
 * send's; MSG_NOSIGNAL is always added, so a peer that has closed completes
 * with -EPIPE instead of raising a SIGPIPE that would end the process,
 * which a write on a socket does. */
[[nodiscard]] lc_result lc_async_submit_send(lc_async_ring *ring, int32_t fd,
                                             const void *buf, uint32_t count,
                                             int32_t flags, uint64_t tag);

/* Queue a receive on a socket. Does not submit to kernel yet. flags are
 * recv's. */
[[nodiscard]] lc_result lc_async_submit_receive(lc_async_ring *ring, int32_t fd,
                                                void *buf, uint32_t count,
                                                int32_t flags, uint64_t tag);

/* Submit all queued operations to the kernel.
 * Returns number of operations submitted. An error is kept for the next
 * _ex call: lc_async_flush_ex reports it. */
uint32_t lc_async_flush(lc_async_ring *ring);

/* Submit all queued operations to the kernel, and report what failed.
 * value is the number submitted. error is the errno of the io_uring_enter
 * that stopped the submission, or of one an earlier call kept, and 0 when
 * every operation was submitted; EINTR is retried. On EAGAIN or EBUSY the
 * operations not submitted stay queued: reap completions, then flush again. */
[[nodiscard]] lc_result lc_async_flush_ex(lc_async_ring *ring);

/* Submit pending ops and wait for at least one completion.
 * Returns number of completions written to results (up to max_results).
 * EINTR is retried, and another error is kept for the next _ex call. */
uint32_t lc_async_wait(lc_async_ring *ring,
                       lc_async_result *results, uint32_t max_results);

/* Submit pending ops, wait for at least one completion, and report what failed.
 * value is the number of completions written to results (up to max_results).
 * error is the errno of the failed io_uring_enter, or of one an earlier call
 * kept, which is reported at once without waiting; the completions there are
 * are written either way. A signal ends the wait with error 0, and value may
 * then be 0. */
[[nodiscard]] lc_result lc_async_wait_ex(lc_async_ring *ring,
                                         lc_async_result *results, uint32_t max_results);

/* Check for completions without blocking.
 * Returns number of completions written to results (0 if none ready).
 * Moves completions the kernel held back while the completion queue was full
 * into it, as room allows; an error doing so is kept for the next _ex call. */
uint32_t lc_async_peek(lc_async_ring *ring,
                       lc_async_result *results, uint32_t max_results);

/* Get how many submission slots are available. */
uint32_t lc_async_get_free_slots(const lc_async_ring *ring);

/* Submit a raw SQE with explicit opcode and parameters.
 * Used by lightio for accept, timeout, and other operations. */
[[nodiscard]] lc_result lc_async_submit_raw(lc_async_ring *ring, uint8_t opcode, int32_t fd,
                         uint64_t addr, uint32_t len, uint64_t offset,
                         uint32_t op_flags, uint64_t tag);

#endif /* LIGHTC_ASYNC_H */
